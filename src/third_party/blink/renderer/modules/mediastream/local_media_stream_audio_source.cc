// Copyright 2016 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/modules/mediastream/local_media_stream_audio_source.h"

#include <utility>

#include "base/strings/stringprintf.h"
#include "base/task/single_thread_task_runner.h"
#include "media/audio/audio_source_parameters.h"
#include "third_party/blink/public/mojom/mediastream/media_stream.mojom-blink.h"
#include "third_party/blink/public/platform/modules/webrtc/webrtc_logging.h"
#include "third_party/blink/public/platform/platform.h"
#include "third_party/blink/public/web/web_local_frame.h"
#include "third_party/blink/renderer/core/frame/local_frame.h"

namespace blink {

LocalMediaStreamAudioSource::LocalMediaStreamAudioSource(
    LocalFrame* consumer_frame,
    const MediaStreamDevice& device,
    const int* requested_buffer_size,
    bool disable_local_echo,
    bool enable_system_echo_cancellation,
    ConstraintsRepeatingCallback started_callback,
    scoped_refptr<base::SingleThreadTaskRunner> task_runner)
    : MediaStreamAudioSource(std::move(task_runner),
                             true /* is_local_source */,
                             disable_local_echo),
      consumer_frame_(consumer_frame),
      started_callback_(std::move(started_callback)) {
  DVLOG(1) << "LocalMediaStreamAudioSource::LocalMediaStreamAudioSource("
              "device.input="
           << device.input.AsHumanReadableString()
           << " requested_buffer_size=" << requested_buffer_size
           << " enable_system_echo_cancellation="
           << (enable_system_echo_cancellation ? "true" : "false") << ")"
           << " system AEC available: "
           << (!!(device.input.effects() &
                  media::AudioParameters::ECHO_CANCELLER)
                   ? "YES"
                   : "NO");
  MediaStreamDevice device_to_request(device);
  if (enable_system_echo_cancellation) {
    // System echo cancellation may only be requested if supported by the
    // device, otherwise a different MediaStreamSource implementation should be
    // used.
    DCHECK_NE(device_to_request.input.effects() &
                  media::AudioParameters::ECHO_CANCELLER,
              0);
  } else {
    // No need for system echo cancellation, clearing the bit if it's set.
    device_to_request.input.set_effects(
        device_to_request.input.effects() &
        ~media::AudioParameters::ECHO_CANCELLER);
  }
  SetDevice(device_to_request);

  int frames_per_buffer = device.input.frames_per_buffer();
  if (requested_buffer_size)
    frames_per_buffer = *requested_buffer_size;

  // If the device buffer size was not provided, use a default.
  if (frames_per_buffer <= 0) {
    frames_per_buffer =
        (device.input.sample_rate() * kFallbackAudioLatencyMs) / 1000;
  }

  // Set audio format and take into account the special case where a discrete
  // channel layout is reported since it will result in an invalid channel
  // count (=0) if only default constructions is used.
  media::AudioParameters params(media::AudioParameters::AUDIO_PCM_LOW_LATENCY,
                                device.input.channel_layout_config(),
                                device.input.sample_rate(), frames_per_buffer);
  if (device.input.channel_layout() == media::CHANNEL_LAYOUT_DISCRETE) {
    DCHECK_LE(device.input.channels(), 2);
  }
  params.set_effects(device_to_request.input.effects());
  SetFormat(params);
}

LocalMediaStreamAudioSource::~LocalMediaStreamAudioSource() {
  DVLOG(1) << "LocalMediaStreamAudioSource::~LocalMediaStreamAudioSource()";
  EnsureSourceIsStopped();
}

bool LocalMediaStreamAudioSource::EnsureSourceIsStarted() {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);

  if (source_)
    return true;

  std::string str = base::StringPrintf(
      "LocalMediaStreamAudioSource::EnsureSourceIsStarted."
      " channel_layout=%d, sample_rate=%d, buffer_size=%d"
      ", session_id=%s, effects=%d. ",
      device().input.channel_layout(), device().input.sample_rate(),
      device().input.frames_per_buffer(),
      device().session_id().ToString().c_str(), device().input.effects());
  WebRtcLogMessage(str);
  DVLOG(1) << str;

  // Sanity-check that the consuming WebLocalFrame still exists.
  // This is required by AudioDeviceFactory.
  if (!consumer_frame_)
    return false;

  VLOG(1) << "Starting local audio input device (session_id="
          << device().session_id() << ") with audio parameters={"
          << GetAudioParameters().AsHumanReadableString() << "}.";

  auto* web_frame =
      static_cast<WebLocalFrame*>(WebFrame::FromCoreFrame(consumer_frame_));
  source_ = Platform::Current()->NewAudioCapturerSource(
      web_frame, media::AudioSourceParameters(device().session_id()));
  source_->Initialize(GetAudioParameters(), this);
  source_->Start();
  return true;
}

void LocalMediaStreamAudioSource::EnsureSourceIsStopped() {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);

  if (!source_)
    return;

  source_->Stop();
  source_ = nullptr;

  VLOG(1) << "Stopped local audio input device (session_id="
          << device().session_id() << ") with audio parameters={"
          << GetAudioParameters().AsHumanReadableString() << "}.";
}

void LocalMediaStreamAudioSource::OnCaptureStarted() {
  started_callback_.Run(this, mojom::MediaStreamRequestResult::OK, "");
}

void LocalMediaStreamAudioSource::Capture(
    const media::AudioBus* audio_bus,
    base::TimeTicks audio_capture_time,
    const media::AudioGlitchInfo& glitch_info,
    double volume) {
  DCHECK(audio_bus);
  DeliverDataToTracks(*audio_bus, audio_capture_time, glitch_info);
}

void LocalMediaStreamAudioSource::OnCaptureError(
    media::AudioCapturerSource::ErrorCode code,
    const std::string& why) {
  WebRtcLogMessage(
      base::StringPrintf("LocalMediaStreamAudioSource::OnCaptureError: %d, %s",
                         static_cast<int>(code), why.c_str()));

  StopSourceOnError(code, why);
}

void LocalMediaStreamAudioSource::OnCaptureMuted(bool is_muted) {
  SetMutedState(is_muted);
}

void LocalMediaStreamAudioSource::ChangeSourceImpl(
    const MediaStreamDevice& new_device) {
  WebRtcLogMessage(
      "LocalMediaStreamAudioSource::ChangeSourceImpl(new_device = " +
      new_device.id + ")");
  EnsureSourceIsStopped();
  SetDevice(new_device);
  EnsureSourceIsStarted();
}

using EchoCancellationType =
    blink::AudioProcessingProperties::EchoCancellationType;

std::optional<blink::AudioProcessingProperties>
LocalMediaStreamAudioSource::GetAudioProcessingProperties() const {
  blink::AudioProcessingProperties properties;
  properties.DisableDefaultProperties();

  if (device().input.effects() & media::AudioParameters::ECHO_CANCELLER) {
    properties.echo_cancellation_type =
        EchoCancellationType::kEchoCancellationSystem;
  }

  return properties;
}

}  // namespace blink
