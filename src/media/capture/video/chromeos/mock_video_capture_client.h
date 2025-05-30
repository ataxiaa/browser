// Copyright 2017 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_CAPTURE_VIDEO_CHROMEOS_MOCK_VIDEO_CAPTURE_CLIENT_H_
#define MEDIA_CAPTURE_VIDEO_CHROMEOS_MOCK_VIDEO_CAPTURE_CLIENT_H_

#include "media/capture/video/video_capture_device.h"
#include "testing/gmock/include/gmock/gmock.h"

// TODO(crbug.com/41386548):
// Consolidate the MockVideoCaptureClient implementations

namespace media {
namespace unittest_internal {

class MockVideoCaptureClient : public VideoCaptureDevice::Client {
 public:
  MOCK_METHOD0(DoReserveOutputBuffer, void(void));
  MOCK_METHOD0(DoOnIncomingCapturedBuffer, void(void));
  MOCK_METHOD0(DoOnIncomingCapturedVideoFrame, void(void));
  MOCK_METHOD0(OnCaptureConfigurationChanged, void(void));
  MOCK_METHOD3(OnError,
               void(media::VideoCaptureError error,
                    const base::Location& from_here,
                    const std::string& reason));
  MOCK_METHOD1(OnFrameDropped, void(media::VideoCaptureFrameDropReason reason));
  MOCK_CONST_METHOD0(GetBufferPoolUtilization, double(void));
  MOCK_METHOD0(OnStarted, void(void));

  explicit MockVideoCaptureClient();

  ~MockVideoCaptureClient() override;

  void SetFrameCb(base::OnceClosure frame_cb);

  void SetQuitCb(base::OnceClosure quit_cb);

  void DumpError(media::VideoCaptureError error,
                 const base::Location& location,
                 const std::string& message);

  void OnIncomingCapturedData(
      const uint8_t* data,
      int length,
      const VideoCaptureFormat& format,
      const gfx::ColorSpace& color_space,
      int rotation,
      bool flip_y,
      base::TimeTicks reference_time,
      base::TimeDelta timestamp,
      std::optional<base::TimeTicks> capture_begin_time,
      const std::optional<media::VideoFrameMetadata>& metadata,
      int frame_feedback_id) override;
  void OnIncomingCapturedGfxBuffer(
      gfx::GpuMemoryBuffer* buffer,
      const VideoCaptureFormat& frame_format,
      int clockwise_rotation,
      base::TimeTicks reference_time,
      base::TimeDelta timestamp,
      std::optional<base::TimeTicks> capture_begin_time,
      const std::optional<media::VideoFrameMetadata>& metadata,
      int frame_feedback_id) override;
  void OnIncomingCapturedExternalBuffer(
      CapturedExternalVideoBuffer buffer,
      base::TimeTicks reference_time,
      base::TimeDelta timestamp,
      std::optional<base::TimeTicks> capture_begin_time,
      const gfx::Rect& visible_rect,
      const std::optional<media::VideoFrameMetadata>& metadata) override;
  // Trampoline methods to workaround GMOCK problems with std::unique_ptr<>.
  ReserveResult ReserveOutputBuffer(const gfx::Size& dimensions,
                                    VideoPixelFormat format,
                                    int frame_feedback_id,
                                    Buffer* buffer,
                                    int* require_new_buffer_id,
                                    int* retire_old_buffer_id) override;
  void OnIncomingCapturedBuffer(
      Buffer buffer,
      const VideoCaptureFormat& format,
      base::TimeTicks reference_time,
      base::TimeDelta timestamp,
      std::optional<base::TimeTicks> capture_begin_time,
      const std::optional<media::VideoFrameMetadata>& metadata) override;
  void OnIncomingCapturedBufferExt(
      Buffer buffer,
      const VideoCaptureFormat& format,
      const gfx::ColorSpace& color_space,
      base::TimeTicks reference_time,
      base::TimeDelta timestamp,
      std::optional<base::TimeTicks> capture_begin_time,
      gfx::Rect visible_rect,
      const std::optional<VideoFrameMetadata>& additional_metadata) override;

 private:
  base::OnceClosure frame_cb_;
  base::OnceClosure quit_cb_;
};

using NiceMockVideoCaptureClient = ::testing::NiceMock<MockVideoCaptureClient>;

}  // namespace unittest_internal
}  // namespace media

#endif  // MEDIA_CAPTURE_VIDEO_CHROMEOS_MOCK_VIDEO_CAPTURE_CLIENT_H_
