// Copyright 2025 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/viz/service/input/android_state_transfer_handler.h"

#include <android/input.h>

#include <utility>
#include <vector>

#include "base/android/build_info.h"
#include "base/android/jni_android.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

// Must come after all headers that specialize FromJniType() / ToJniType().
#include "ui/events/motionevent_jni_headers/MotionEvent_jni.h"

namespace viz {

using ::testing::_;

namespace {

constexpr int kAndroidActionDown = AMOTION_EVENT_ACTION_DOWN;
constexpr int kAndroidActionMove = AMOTION_EVENT_ACTION_MOVE;
constexpr int kAndroidActionUp = AMOTION_EVENT_ACTION_UP;
constexpr FrameSinkId kRootCompositorFrameSinkId = FrameSinkId(1, 1);
constexpr FrameSinkId kRootWidgetFrameSinkId = FrameSinkId(2, 3);

base::android::ScopedInputEvent GetInputEvent(jlong down_time_ms,
                                              jlong event_time_ms,
                                              int action,
                                              float x,
                                              float y) {
  // Java_MotionEvent_obtain expects timestamps(down time, event time) obtained
  // from |SystemClock#uptimeMillis()|.
  JNIEnv* env = base::android::AttachCurrentThread();
  base::android::ScopedJavaLocalRef<jobject> java_motion_event =
      JNI_MotionEvent::Java_MotionEvent_obtain(env, down_time_ms, event_time_ms,
                                               action, x, y,
                                               /*metaState=*/0);
  const AInputEvent* native_event = nullptr;
  if (__builtin_available(android 31, *)) {
    native_event = AMotionEvent_fromJava(env, java_motion_event.obj());
  }
  CHECK(native_event);

  return base::android::ScopedInputEvent(native_event);
}

struct TestInputStream {
  base::TimeTicks down_time_ms;
  std::vector<base::android::ScopedInputEvent> events;
};

TestInputStream GenerateEventsForSequence(int num_moves,
                                          bool include_touch_up) {
  static base::TimeTicks event_time =
      base::TimeTicks::Now() - base::Milliseconds(100);
  static float x = 100;
  static float y = 200;

  TestInputStream event_stream;

  event_time += base::Milliseconds(8);
  x += 5;
  y += 5;

  jlong down_time = event_time.ToUptimeMillis();
  event_stream.down_time_ms = base::TimeTicks::FromUptimeMillis(down_time);
  event_stream.events.push_back(GetInputEvent(
      down_time, event_time.ToUptimeMillis(), kAndroidActionDown, x, y));

  for (int i = 1; i <= num_moves; i++) {
    event_time += base::Milliseconds(8);
    x += 5;
    y += 5;
    event_stream.events.push_back(GetInputEvent(
        down_time, event_time.ToUptimeMillis(), kAndroidActionMove, x, y));
  }
  if (include_touch_up) {
    event_time += base::Milliseconds(8);
    event_stream.events.push_back(GetInputEvent(
        down_time, event_time.ToUptimeMillis(), kAndroidActionUp, x, y));
  }
  return event_stream;
}

}  // namespace

class MockRenderInputRouterSupportAndroid
    : public RenderInputRouterSupportAndroidInterface {
 public:
  virtual ~MockRenderInputRouterSupportAndroid() = default;

  MOCK_METHOD((bool),
              OnTouchEvent,
              (const ui::MotionEventAndroid&, bool),
              (override));

  base::WeakPtr<RenderInputRouterSupportAndroidInterface> GetWeakPtr() {
    return weak_factory_.GetWeakPtr();
  }

  void Destroy() { weak_factory_.InvalidateWeakPtrs(); }

 private:
  base::WeakPtrFactory<RenderInputRouterSupportAndroidInterface> weak_factory_{
      this};
};

class AndroidStateTransferHandlerTest : public testing::Test {
 public:
  void SetUp() override {
    if (base::android::BuildInfo::GetInstance()->sdk_int() <
        base::android::SDK_VERSION_V) {
      GTEST_SKIP()
          << "AndroidStateTransferHandlerTest is used only when InputOnViz "
             "is enabled i.e. on Android V+";
    }
  }

 protected:
  MockRenderInputRouterSupportAndroid mock_rir_support_;
  AndroidStateTransferHandler handler_;
};

// The order of events received:
// Down1 -> Move1 -> StateTransfer(for Down1)
TEST_F(AndroidStateTransferHandlerTest, EventsProcessedOnStateTransfer) {
  TestInputStream event_stream = GenerateEventsForSequence(
      /*num_moves*/ 1,
      /*include_touch_up*/ false);

  for (auto& event : event_stream.events) {
    handler_.OnMotionEvent(std::move(event), kRootCompositorFrameSinkId);
  }
  EXPECT_EQ(handler_.GetEventsBufferSizeForTesting(), 2u);

  auto state = input::mojom::TouchTransferState::New();
  state->down_time_ms = event_stream.down_time_ms;
  state->root_widget_frame_sink_id = kRootWidgetFrameSinkId;

  // Both the events should be dequeued and processed.
  EXPECT_CALL(mock_rir_support_, OnTouchEvent(_, _)).Times(2);
  handler_.StateOnTouchTransfer(std::move(state),
                                mock_rir_support_.GetWeakPtr());
  EXPECT_EQ(handler_.GetEventsBufferSizeForTesting(), 0u);
}

// The order of events received:
// Down1 -> Move1 -> StateTransfer(for Down1)
// The rir_support for widget referred in state is already destroyed.
TEST_F(AndroidStateTransferHandlerTest, RIRSupportNullOnStateTransfer) {
  TestInputStream event_stream = GenerateEventsForSequence(
      /*num_moves*/ 1,
      /*include_touch_up*/ false);

  for (auto& event : event_stream.events) {
    handler_.OnMotionEvent(std::move(event), kRootCompositorFrameSinkId);
  }
  EXPECT_EQ(handler_.GetEventsBufferSizeForTesting(), 2u);

  auto state = input::mojom::TouchTransferState::New();
  state->down_time_ms = event_stream.down_time_ms;
  state->root_widget_frame_sink_id = kRootWidgetFrameSinkId;

  EXPECT_CALL(mock_rir_support_, OnTouchEvent(_, _)).Times(0);
  handler_.StateOnTouchTransfer(std::move(state),
                                /* rir_support= */ nullptr);
  EXPECT_EQ(handler_.GetEventsBufferSizeForTesting(), 0u);
}

// The order of events received:
// StateTransfer(for Down1) -> Down1 -> Move1 -> OnDestroyedCompositorFrameSink
// -> Move3.
// Move3 ends up getting dropped.
TEST_F(AndroidStateTransferHandlerTest,
       InputReceivingCompositorFrameSinkDestroyedMidSequence) {
  TestInputStream event_stream = GenerateEventsForSequence(
      /*num_moves*/ 2,
      /*include_touch_up*/ false);

  auto state = input::mojom::TouchTransferState::New();
  state->down_time_ms = event_stream.down_time_ms;
  state->root_widget_frame_sink_id = kRootWidgetFrameSinkId;

  handler_.StateOnTouchTransfer(std::move(state),
                                mock_rir_support_.GetWeakPtr());
  handler_.OnMotionEvent(std::move(event_stream.events[0]),
                         kRootCompositorFrameSinkId);
  handler_.OnMotionEvent(std::move(event_stream.events[1]),
                         kRootCompositorFrameSinkId);

  EXPECT_CALL(mock_rir_support_, OnTouchEvent(_, _)).Times(0);
  mock_rir_support_.Destroy();

  //  The events are dropped after the render input router support is
  //  destroyed.
  handler_.OnMotionEvent(std::move(event_stream.events[2]),
                         kRootCompositorFrameSinkId);
  EXPECT_EQ(handler_.GetEventsBufferSizeForTesting(), 0u);
}

// The order of events received:
// StateTransfer(for Down1) -> Down1 -> Move1
// Down1 and Move1 can be immediately processed.
TEST_F(AndroidStateTransferHandlerTest, StateReceivedBeforeTouchDownOnViz) {
  TestInputStream event_stream = GenerateEventsForSequence(
      /*num_moves*/ 2,
      /*include_touch_up*/ false);

  auto state = input::mojom::TouchTransferState::New();
  state->down_time_ms = event_stream.down_time_ms;
  state->root_widget_frame_sink_id = kRootWidgetFrameSinkId;
  handler_.StateOnTouchTransfer(std::move(state),
                                mock_rir_support_.GetWeakPtr());

  EXPECT_CALL(mock_rir_support_, OnTouchEvent(_, _)).Times(1);
  handler_.OnMotionEvent(std::move(event_stream.events[0]),
                         kRootCompositorFrameSinkId);
  EXPECT_EQ(handler_.GetEventsBufferSizeForTesting(), 0u);

  EXPECT_CALL(mock_rir_support_, OnTouchEvent(_, _)).Times(1);
  handler_.OnMotionEvent(std::move(event_stream.events[1]),
                         kRootCompositorFrameSinkId);
  EXPECT_EQ(handler_.GetEventsBufferSizeForTesting(), 0u);
}

// The order of events received:
// StateTransfer(for Down1) -> Down1 -> Move1 -> StateTransfer(for Down2) -> Up1
// -> Down2 Down1 and move1 can be immediately processed.
TEST_F(AndroidStateTransferHandlerTest, NewStateReceivedMidSequence) {
  TestInputStream event_stream_1 =
      GenerateEventsForSequence(/*num_moves*/ 1,
                                /*include_touch_up*/ true);

  TestInputStream event_stream_2 =
      GenerateEventsForSequence(/*num_moves*/ 0,
                                /*include_touch_up*/ false);

  // State is received before receiving any touch events directly on Viz
  auto state1 = input::mojom::TouchTransferState::New();
  state1->down_time_ms = event_stream_1.down_time_ms;

  handler_.StateOnTouchTransfer(std::move(state1),
                                mock_rir_support_.GetWeakPtr());

  EXPECT_CALL(mock_rir_support_, OnTouchEvent(_, _)).Times(3);
  // Down1 is received.
  handler_.OnMotionEvent(std::move(event_stream_1.events[0]),
                         kRootCompositorFrameSinkId);
  EXPECT_EQ(handler_.GetEventsBufferSizeForTesting(), 0u);

  // Move1 is received.
  handler_.OnMotionEvent(std::move(event_stream_1.events[1]),
                         kRootCompositorFrameSinkId);
  EXPECT_EQ(handler_.GetEventsBufferSizeForTesting(), 0u);

  // State for second sequence is received.
  MockRenderInputRouterSupportAndroid mock_rir_support2;
  auto state2 = input::mojom::TouchTransferState::New();
  state2->down_time_ms = event_stream_2.down_time_ms;
  handler_.StateOnTouchTransfer(std::move(state2),
                                mock_rir_support2.GetWeakPtr());

  // Up1 is received.
  handler_.OnMotionEvent(std::move(event_stream_1.events[2]),
                         kRootCompositorFrameSinkId);
  EXPECT_EQ(handler_.GetEventsBufferSizeForTesting(), 0u);

  // Down1 is received from second sequence, on different
  // RenderInputRouterSupport.
  EXPECT_CALL(mock_rir_support2, OnTouchEvent(_, _)).Times(1);
  handler_.OnMotionEvent(std::move(event_stream_2.events[0]),
                         kRootCompositorFrameSinkId);
  EXPECT_EQ(handler_.GetEventsBufferSizeForTesting(), 0u);
}

// The order of events received:
// Down1 -> Move1 -> Up1 -> Down2 -> StateTransfer(for Down1)
// Down1 and move1 can be immediately processed.
TEST_F(AndroidStateTransferHandlerTest,
       FullSequenceReceivedBeforeStateTransfer) {
  TestInputStream event_stream_1 =
      GenerateEventsForSequence(/*num_moves*/ 1,
                                /*include_touch_up*/ true);

  TestInputStream event_stream_2 =
      GenerateEventsForSequence(/*num_moves*/ 0,
                                /*include_touch_up*/ false);

  for (auto& event : event_stream_1.events) {
    handler_.OnMotionEvent(std::move(event), kRootCompositorFrameSinkId);
  }
  EXPECT_EQ(handler_.GetEventsBufferSizeForTesting(), 3u);

  for (auto& event : event_stream_2.events) {
    handler_.OnMotionEvent(std::move(event), kRootCompositorFrameSinkId);
  }
  EXPECT_EQ(handler_.GetEventsBufferSizeForTesting(), 4u);

  // State is received before receiving any touch events directly on Viz
  EXPECT_CALL(mock_rir_support_, OnTouchEvent(_, _)).Times(3);
  auto state1 = input::mojom::TouchTransferState::New();
  state1->down_time_ms = event_stream_1.down_time_ms;
  handler_.StateOnTouchTransfer(std::move(state1),
                                mock_rir_support_.GetWeakPtr());
  EXPECT_EQ(handler_.GetEventsBufferSizeForTesting(), 1u);
}

// The order of events received:
// StateTransfer(Down2) -> Down1 -> Up1 -> Down2
TEST_F(AndroidStateTransferHandlerTest,
       InputEventsAreNotQueuedForDroppedStateTransfer) {
  TestInputStream event_stream_1 =
      GenerateEventsForSequence(/*num_moves*/ 0,
                                /*include_touch_up*/ true);

  TestInputStream event_stream_2 =
      GenerateEventsForSequence(/*num_moves*/ 0,
                                /*include_touch_up*/ false);

  auto state2 = input::mojom::TouchTransferState::New();
  state2->down_time_ms = event_stream_2.down_time_ms;
  handler_.StateOnTouchTransfer(std::move(state2),
                                mock_rir_support_.GetWeakPtr());

  for (auto& event : event_stream_1.events) {
    handler_.OnMotionEvent(std::move(event), kRootCompositorFrameSinkId);
  }
  EXPECT_EQ(handler_.GetEventsBufferSizeForTesting(), 0u);

  EXPECT_CALL(mock_rir_support_, OnTouchEvent(_, _)).Times(1);
  for (auto& event : event_stream_2.events) {
    handler_.OnMotionEvent(std::move(event), kRootCompositorFrameSinkId);
  }
  EXPECT_EQ(handler_.GetEventsBufferSizeForTesting(), 0u);
}

// The order of events received:
// Down1 -> Up1 -> Down2 -> Up2 -> Down3 -> StateTransfer(Down3)
TEST_F(AndroidStateTransferHandlerTest,
       QueuedInputEventsDroppedUponLaterStateTransfer) {
  TestInputStream event_stream_1 =
      GenerateEventsForSequence(/*num_moves*/ 0,
                                /*include_touch_up*/ true);

  TestInputStream event_stream_2 =
      GenerateEventsForSequence(/*num_moves*/ 0,
                                /*include_touch_up*/ true);

  TestInputStream event_stream_3 =
      GenerateEventsForSequence(/*num_moves*/ 0,
                                /*include_touch_up*/ false);

  for (auto& event : event_stream_1.events) {
    handler_.OnMotionEvent(std::move(event), kRootCompositorFrameSinkId);
  }
  for (auto& event : event_stream_2.events) {
    handler_.OnMotionEvent(std::move(event), kRootCompositorFrameSinkId);
  }
  for (auto& event : event_stream_3.events) {
    handler_.OnMotionEvent(std::move(event), kRootCompositorFrameSinkId);
  }
  EXPECT_EQ(handler_.GetEventsBufferSizeForTesting(),
            event_stream_1.events.size() + event_stream_2.events.size() +
                event_stream_3.events.size());

  EXPECT_CALL(mock_rir_support_, OnTouchEvent(_, _)).Times(1);
  auto state3 = input::mojom::TouchTransferState::New();
  state3->down_time_ms = event_stream_3.down_time_ms;
  handler_.StateOnTouchTransfer(std::move(state3),
                                mock_rir_support_.GetWeakPtr());
}

// The order of events received:
// StateTransfer(Down1) -> Down1 -> Move1 -> OnDestroyedCompositorFrameSink ->
// Up1 -> Down2 -> StateTransfer(Down2)
TEST_F(AndroidStateTransferHandlerTest, RirNullOnLastInputInSequence) {
  TestInputStream event_stream_1 =
      GenerateEventsForSequence(/*num_moves*/ 1,
                                /*include_touch_up*/ true);

  TestInputStream event_stream_2 =
      GenerateEventsForSequence(/*num_moves*/ 0,
                                /*include_touch_up*/ false);

  auto state1 = input::mojom::TouchTransferState::New();
  state1->down_time_ms = event_stream_1.down_time_ms;
  handler_.StateOnTouchTransfer(std::move(state1),
                                mock_rir_support_.GetWeakPtr());

  EXPECT_CALL(mock_rir_support_, OnTouchEvent(_, _)).Times(2);
  handler_.OnMotionEvent(std::move(event_stream_1.events[0]),
                         kRootCompositorFrameSinkId);
  handler_.OnMotionEvent(std::move(event_stream_1.events[1]),
                         kRootCompositorFrameSinkId);

  mock_rir_support_.Destroy();

  EXPECT_CALL(mock_rir_support_, OnTouchEvent(_, _)).Times(0);
  handler_.OnMotionEvent(std::move(event_stream_1.events[2]),
                         kRootCompositorFrameSinkId);
  EXPECT_EQ(handler_.GetEventsBufferSizeForTesting(), 0u);

  for (auto& event : event_stream_2.events) {
    handler_.OnMotionEvent(std::move(event), kRootCompositorFrameSinkId);
  }
  EXPECT_EQ(handler_.GetEventsBufferSizeForTesting(), 1u);

  EXPECT_CALL(mock_rir_support_, OnTouchEvent(_, _)).Times(1);
  auto state2 = input::mojom::TouchTransferState::New();
  state2->down_time_ms = event_stream_2.down_time_ms;
  handler_.StateOnTouchTransfer(std::move(state2),
                                mock_rir_support_.GetWeakPtr());
}

}  // namespace viz
