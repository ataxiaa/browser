// Copyright 2023 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.content.browser.input;

import org.chromium.base.ThreadUtils;
import org.chromium.base.metrics.RecordHistogram;
import org.chromium.blink.mojom.HandwritingGestureResult;
import org.chromium.blink.mojom.StylusWritingGestureData;
import org.chromium.build.annotations.NullMarked;

import java.util.concurrent.Executor;
import java.util.function.IntConsumer;

/**
 * Stores data needed to process and record the result of a gesture, reporting it to Android.
 * Also records how long it took to process the gesture.
 */
@NullMarked
class OngoingGesture {
    private static int sLastId;

    private final int mId;
    private final StylusWritingGestureData mGestureData;
    private final Executor mExecutor;
    private final IntConsumer mConsumer;
    private final long mCreationTimestamp;

    OngoingGesture(StylusWritingGestureData gestureData, Executor executor, IntConsumer consumer) {
        ThreadUtils.assertOnUiThread();
        mId = ++sLastId;
        mGestureData = gestureData;
        mExecutor = executor;
        mConsumer = consumer;
        mCreationTimestamp = System.currentTimeMillis();
    }

    void onGestureHandled(@HandwritingGestureResult.EnumType int result) {
        mExecutor.execute(() -> mConsumer.accept(result));
        logGestureResult(result);

        long timeTaken = System.currentTimeMillis() - mCreationTimestamp;
        assert timeTaken >= 0;
        // Log time taken to handle gesture.
        // Expected range is from 0ms to 1000ms (1 second) with 50 buckets.
        RecordHistogram.recordCustomTimesHistogram(
                "InputMethod.StylusHandwriting.GestureTime2",
                timeTaken,
                /* min= */ 1L,
                /* max= */ 250L,
                /* numBuckets= */ 50);
    }

    int getId() {
        return mId;
    }

    StylusWritingGestureData getGestureData() {
        return mGestureData;
    }

    private static void logGestureResult(@HandwritingGestureResult.EnumType int gestureResult) {
        RecordHistogram.recordEnumeratedHistogram(
                "InputMethod.StylusHandwriting.GestureResult",
                gestureResult,
                HandwritingGestureResult.MAX_VALUE);
    }
}
