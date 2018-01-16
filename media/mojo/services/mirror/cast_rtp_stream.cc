// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/mojo/services/mirror/cast_rtp_stream.h"
#include "media/mojo/services/mirror/mirror_service.h"

#include <stdint.h>
#include <algorithm>
#include <memory>
#include <utility>

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/callback_helpers.h"
#include "base/command_line.h"
#include "base/logging.h"
#include "base/macros.h"
#include "base/threading/thread_task_runner_handle.h"
#include "base/trace_event/trace_event.h"
#include "base/values.h"
#include "content/public/renderer/video_encode_accelerator.h"
#include "media/base/audio_bus.h"
#include "media/base/audio_converter.h"
#include "media/base/audio_parameters.h"
#include "media/base/bind_to_current_loop.h"
#include "media/base/media_switches.h"
#include "media/base/video_frame.h"
#include "media/base/video_util.h"
#include "media/cast/cast_config.h"
#include "media/cast/cast_sender.h"
#include "media/cast/net/cast_transport_config.h"
#include "media/video/video_encode_accelerator.h"

using media::cast::FrameSenderConfig;

namespace media {

namespace {

// The maximum number of milliseconds that should elapse since the last video
// frame was received from the video source, before requesting refresh frames.
constexpr int kRefreshIntervalMilliseconds = 250;

// The maximum number of refresh video frames to request/receive.  After this
// limit (60 * 250ms = 15 seconds), refresh frame requests will stop being made.
constexpr int kMaxConsecutiveRefreshFrames = 60;

FrameSenderConfig DefaultOpusConfig() {
  FrameSenderConfig config;
  config.rtp_payload_type = media::cast::RtpPayloadType::AUDIO_OPUS;
  config.sender_ssrc = 1;
  config.receiver_ssrc = 2;
  config.rtp_timebase = media::cast::kDefaultAudioSamplingRate;
  config.channels = 2;
  config.min_bitrate = config.max_bitrate = config.start_bitrate =
      media::cast::kDefaultAudioEncoderBitrate;
  config.max_frame_rate = 100;  // 10 ms audio frames
  config.codec = media::cast::CODEC_AUDIO_OPUS;
  return config;
}

FrameSenderConfig DefaultVp8Config() {
  FrameSenderConfig config;
  config.rtp_payload_type = media::cast::RtpPayloadType::VIDEO_VP8;
  config.sender_ssrc = 11;
  config.receiver_ssrc = 12;
  config.rtp_timebase = media::cast::kVideoFrequency;
  config.channels = 1;
  config.max_bitrate = media::cast::kDefaultMaxVideoBitrate;
  config.min_bitrate = media::cast::kDefaultMinVideoBitrate;
  config.max_frame_rate = media::cast::kDefaultMaxFrameRate;
  config.codec = media::cast::CODEC_VIDEO_VP8;
  return config;
}

FrameSenderConfig DefaultH264Config() {
  FrameSenderConfig config;
  config.rtp_payload_type = media::cast::RtpPayloadType::VIDEO_H264;
  config.sender_ssrc = 11;
  config.receiver_ssrc = 12;
  config.rtp_timebase = media::cast::kVideoFrequency;
  config.channels = 1;
  config.max_bitrate = media::cast::kDefaultMaxVideoBitrate;
  config.min_bitrate = media::cast::kDefaultMinVideoBitrate;
  config.max_frame_rate = media::cast::kDefaultMaxFrameRate;
  config.codec = media::cast::CODEC_VIDEO_H264;
  return config;
}

FrameSenderConfig DefaultRemotingAudioConfig() {
  FrameSenderConfig config;
  config.rtp_payload_type = media::cast::RtpPayloadType::REMOTE_AUDIO;
  config.sender_ssrc = 3;
  config.receiver_ssrc = 4;
  config.codec = media::cast::CODEC_AUDIO_REMOTE;
  config.rtp_timebase = media::cast::kRemotingRtpTimebase;
  config.max_bitrate = 1000000;
  config.min_bitrate = 0;
  config.channels = 2;
  config.max_frame_rate = 100;  // 10 ms audio frames

  return config;
}

FrameSenderConfig DefaultRemotingVideoConfig() {
  FrameSenderConfig config;
  config.rtp_payload_type = media::cast::RtpPayloadType::REMOTE_VIDEO;
  config.sender_ssrc = 13;
  config.receiver_ssrc = 14;
  config.codec = media::cast::CODEC_VIDEO_REMOTE;
  config.rtp_timebase = media::cast::kRemotingRtpTimebase;
  config.max_bitrate = 10000000;
  config.min_bitrate = 0;
  config.channels = 1;
  config.max_frame_rate = media::cast::kDefaultMaxFrameRate;
  return config;
}

std::vector<FrameSenderConfig> SupportedAudioConfigs(bool for_remoting_stream) {
  if (for_remoting_stream)
    return {DefaultRemotingAudioConfig()};
  else
    return {DefaultOpusConfig()};
}

std::vector<FrameSenderConfig> SupportedVideoConfigs(bool for_remoting_stream) {
  if (for_remoting_stream)
    return {DefaultRemotingVideoConfig()};

  std::vector<FrameSenderConfig> supported_configs;
  // Prefer VP8 over H.264 for hardware encoder.
  if (CastRtpStream::IsHardwareVP8EncodingSupported())
    supported_configs.push_back(DefaultVp8Config());
  if (CastRtpStream::IsHardwareH264EncodingSupported())
    supported_configs.push_back(DefaultH264Config());

  // Propose the default software VP8 encoder, if no hardware encoders are
  // available.
  if (supported_configs.empty())
    supported_configs.push_back(DefaultVp8Config());

  return supported_configs;
}

}  // namespace

CastVideoSink::CastVideoSink(const ErrorCallback& error_callback,
                             MirrorService* service)
    : deliverer_(new Deliverer(error_callback)),
      service_(service),
      consecutive_refresh_count_(0),
      expecting_a_refresh_frame_(false),
      weak_factory_(this) {}

CastVideoSink::~CastVideoSink() {}

void CastVideoSink::AddToTrack(
    const scoped_refptr<media::cast::VideoFrameInput>& frame_input) {
  DCHECK(deliverer_);
  deliverer_->WillConnectToTrack(weak_factory_.GetWeakPtr(), frame_input);
  refresh_timer_.Start(
      FROM_HERE,
      base::TimeDelta::FromMilliseconds(kRefreshIntervalMilliseconds),
      base::Bind(&CastVideoSink::OnRefreshTimerFired, base::Unretained(this)));
}

CastVideoSink::Deliverer::Deliverer(const ErrorCallback& error_callback)
    : error_callback_(error_callback) {}

void CastVideoSink::Deliverer::WillConnectToTrack(
    base::WeakPtr<CastVideoSink> sink,
    scoped_refptr<media::cast::VideoFrameInput> frame_input) {
  sink_ = sink;
  frame_input_ = std::move(frame_input);
}

void CastVideoSink::Deliverer::OnVideoFrame(
    const scoped_refptr<media::VideoFrame>& video_frame,
    base::TimeTicks estimated_capture_time) {
  if (!sink_)
    return;

  sink_->DidReceiveFrame();

  const base::TimeTicks timestamp = estimated_capture_time.is_null()
                                        ? base::TimeTicks::Now()
                                        : estimated_capture_time;

  if (!(video_frame->format() == media::PIXEL_FORMAT_I420 ||
        video_frame->format() == media::PIXEL_FORMAT_YV12 ||
        video_frame->format() == media::PIXEL_FORMAT_YV12A)) {
    error_callback_.Run("Incompatible video frame format.");
    return;
  }
  scoped_refptr<media::VideoFrame> frame = video_frame;
  // Drop alpha channel since we do not support it yet.
  if (frame->format() == media::PIXEL_FORMAT_YV12A)
    frame = media::WrapAsI420VideoFrame(video_frame);

  // Used by chrome/browser/extension/api/cast_streaming/performance_test.cc
  TRACE_EVENT_INSTANT2("cast_perf_test", "ConsumeVideoFrame",
                       TRACE_EVENT_SCOPE_THREAD, "timestamp",
                       (timestamp - base::TimeTicks()).InMicroseconds(),
                       "time_delta", frame->timestamp().InMicroseconds());
  frame_input_->InsertRawVideoFrame(frame, timestamp);
}

CastVideoSink::Deliverer::~Deliverer() {}

void CastVideoSink::OnRefreshTimerFired() {
  ++consecutive_refresh_count_;
  if (consecutive_refresh_count_ >= kMaxConsecutiveRefreshFrames)
    refresh_timer_.Stop();  // Stop timer until receiving a non-refresh frame.

  DVLOG(1) << "CastVideoSink is requesting another refresh frame "
              "(consecutive count="
           << consecutive_refresh_count_ << ").";
  expecting_a_refresh_frame_ = true;
  service_->RequestRefreshFrame();
}

void CastVideoSink::DidReceiveFrame() {
  if (expecting_a_refresh_frame_) {
    // There is uncertainty as to whether the video frame was in response to a
    // refresh request.  However, if it was not, more video frames will soon
    // follow, and before the refresh timer can fire again.  Thus, the
    // behavior resulting from this logic will be correct.
    expecting_a_refresh_frame_ = false;
  } else {
    consecutive_refresh_count_ = 0;
    // The following re-starts the timer, scheduling it to fire at
    // kRefreshIntervalMilliseconds from now.
    refresh_timer_.Reset();
  }
}

// Receives audio data from a MediaStreamTrack. Data is submitted to
// media::cast::FrameInput.
//
// Threading: Audio frames are received on the real-time audio thread.
// Note that RemoveFromAudioTrack() is synchronous and we have
// gurantee that there will be no more audio data after calling it.
class CastAudioSink : public media::AudioConverter::InputCallback {
 public:
  // |track| provides data for this sink.
  CastAudioSink(int output_channels, int output_sample_rate)
      : output_channels_(output_channels),
        output_sample_rate_(output_sample_rate),
        current_input_bus_(nullptr),
        sample_frames_in_(0),
        sample_frames_out_(0) {}

  ~CastAudioSink() override {}

  // Add this sink. Data received will be submitted to |frame_input|.
  void AddToTrack(
      const scoped_refptr<media::cast::AudioFrameInput>& frame_input) {
    DCHECK(frame_input.get());
    DCHECK(!frame_input_.get());
    // This member is written here and then accessed on the IO thread
    // We will not get data until AddToAudioTrack is called so it is
    // safe to access this member now.
    frame_input_ = frame_input;
  }

  // Called on real-time audio thread.
  void OnData(const media::AudioBus& input_bus,
              base::TimeTicks estimated_capture_time) {
    // LOG(INFO) << __func__;
    ++num_on_data_;
    DCHECK(input_params_.IsValid());
    DCHECK_EQ(input_bus.channels(), input_params_.channels());
    DCHECK_EQ(input_bus.frames(), input_params_.frames_per_buffer());
    DCHECK(!estimated_capture_time.is_null());
    DCHECK(converter_.get());

    // Determine the duration of the audio signal enqueued within |converter_|.
    const base::TimeDelta signal_duration_already_buffered =
        (sample_frames_in_ * base::TimeDelta::FromSeconds(1) /
         input_params_.sample_rate()) -
        (sample_frames_out_ * base::TimeDelta::FromSeconds(1) /
         output_sample_rate_);
    DVLOG(2) << "Audio reference time adjustment: -("
             << signal_duration_already_buffered.InMicroseconds() << " us)";
    const base::TimeTicks capture_time_of_first_converted_sample =
        estimated_capture_time - signal_duration_already_buffered;

    // Convert the entire input signal.  AudioConverter is efficient in that no
    // additional copying or conversion will occur if the input signal is in the
    // same format as the output.  Note that, while the number of sample frames
    // provided as input is always the same, the chunk size (and the size of the
    // |audio_bus| here) can be variable.  This is not an issue since
    // media::cast::AudioFrameInput can handle variable-sized AudioBuses.
    std::unique_ptr<media::AudioBus> audio_bus =
        media::AudioBus::Create(output_channels_, converter_->ChunkSize());
    // AudioConverter will call ProvideInput() to fetch from |current_data_|.
    current_input_bus_ = &input_bus;
    converter_->Convert(audio_bus.get());
    DCHECK(!current_input_bus_);  // ProvideInput() called exactly once?

    sample_frames_in_ += input_params_.frames_per_buffer();
    sample_frames_out_ += audio_bus->frames();

    frame_input_->InsertAudio(std::move(audio_bus),
                              capture_time_of_first_converted_sample);
  }

  // Called on real-time audio thread.
  void OnSetFormat(const media::AudioParameters& params) {
    if (input_params_.Equals(params))
      return;
    input_params_ = params;

    DVLOG(1) << "Setting up audio resampling: {" << input_params_.channels()
             << " channels, " << input_params_.sample_rate() << " Hz} --> {"
             << output_channels_ << " channels, " << output_sample_rate_
             << " Hz}";
    const media::AudioParameters output_params(
        media::AudioParameters::AUDIO_PCM_LOW_LATENCY,
        media::GuessChannelLayout(output_channels_), output_sample_rate_, 32,
        output_sample_rate_ * input_params_.frames_per_buffer() /
            input_params_.sample_rate());
    LOG(INFO) << "output sample_rate=" << output_params.sample_rate()
              << " output frames_per_buffer="
              << output_params.frames_per_buffer()
              << " input sample_rate=" << input_params_.sample_rate()
              << " input frames_per_buffer="
              << input_params_.frames_per_buffer();
    converter_.reset(
        new media::AudioConverter(input_params_, output_params, false));
    converter_->AddInput(this);
    sample_frames_in_ = 0;
    sample_frames_out_ = 0;
  }

  // AudioConverter::InputCallback implementation.
  // Called on real-time audio thread.
  double ProvideInput(media::AudioBus* audio_bus,
                      uint32_t frames_delayed) override {
    // LOG(INFO) << __func__ << " number=" << num_on_data_;
    DCHECK(current_input_bus_);
    current_input_bus_->CopyTo(audio_bus);
    current_input_bus_ = nullptr;
    return 1.0;
  }

 private:
  const int output_channels_;
  const int output_sample_rate_;

  // This must be set before the real-time audio thread starts calling OnData(),
  // and remain unchanged until after the thread will stop calling OnData().
  scoped_refptr<media::cast::AudioFrameInput> frame_input_;

  // These members are accessed on the real-time audio time only.
  media::AudioParameters input_params_;
  std::unique_ptr<media::AudioConverter> converter_;
  const media::AudioBus* current_input_bus_;
  int64_t sample_frames_in_;
  int64_t sample_frames_out_;
  uint64_t num_on_data_ = 0;

  DISALLOW_COPY_AND_ASSIGN(CastAudioSink);
};

bool CastRtpStream::IsHardwareVP8EncodingSupported() {
  const base::CommandLine* cmd_line = base::CommandLine::ForCurrentProcess();
  if (cmd_line->HasSwitch(switches::kDisableCastStreamingHWEncoding)) {
    DVLOG(1) << "Disabled hardware VP8 support for Cast Streaming.";
    return false;
  }

  // Query for hardware VP8 encoder support.
  const std::vector<media::VideoEncodeAccelerator::SupportedProfile>
      vea_profiles =
          MirrorService::GetSupportedVideoEncodeAcceleratorProfiles();
  for (const auto& vea_profile : vea_profiles) {
    if (vea_profile.profile >= media::VP8PROFILE_MIN &&
        vea_profile.profile <= media::VP8PROFILE_MAX) {
      return true;
    }
  }
  return false;
}

bool CastRtpStream::IsHardwareH264EncodingSupported() {
  const base::CommandLine* cmd_line = base::CommandLine::ForCurrentProcess();
  if (cmd_line->HasSwitch(switches::kDisableCastStreamingHWEncoding)) {
    DVLOG(1) << "Disabled hardware h264 support for Cast Streaming.";
    return false;
  }

// Query for hardware H.264 encoder support.
//
// TODO(miu): Look into why H.264 hardware encoder on MacOS is broken.
// http://crbug.com/596674
// TODO(emircan): Look into HW encoder initialization issues on Win.
// https://crbug.com/636064
#if !defined(OS_MACOSX) && !defined(OS_WIN)
  const std::vector<media::VideoEncodeAccelerator::SupportedProfile>
      vea_profiles =
          MirrorService::GetSupportedVideoEncodeAcceleratorProfiles();
  for (const auto& vea_profile : vea_profiles) {
    if (vea_profile.profile >= media::H264PROFILE_MIN &&
        vea_profile.profile <= media::H264PROFILE_MAX) {
      return true;
    }
  }
#endif  // !defined(OS_MACOSX) && !defined(OS_WIN)
  return false;
}

CastRtpStream::CastRtpStream(bool is_audio,
                             cast::CastSender* cast_sender,
                             MirrorService* service)
    : is_audio_(is_audio),
      cast_sender_(cast_sender),
      service_(service),
      weak_factory_(this) {}

CastRtpStream::~CastRtpStream() {}

// static
std::vector<FrameSenderConfig> CastRtpStream::GetSupportedConfigs(
    bool is_audio,
    bool is_remoting) {
  if (is_audio)
    return SupportedAudioConfigs(is_remoting);
  else
    return SupportedVideoConfigs(is_remoting);
}

void CastRtpStream::Start(const FrameSenderConfig& config) {
  LOG(INFO) << __func__ << ": is_audio: " << is_audio_;
  DCHECK(cast_sender_);
  if (is_audio_) {
    audio_sink_.reset(new CastAudioSink(config.channels, config.rtp_timebase));
    cast_sender_->InitializeAudio(
        config,
        BindToCurrentLoop(base::Bind(&MirrorService::OnOperationalStatusChange,
                                     base::Unretained(service_), is_audio_)));
  } else {
    video_sink_.reset(new CastVideoSink(
        BindToCurrentLoop(base::Bind(&MirrorService::OnErrorMessage,
                                     base::Unretained(service_))),
        service_));
    cast_sender_->InitializeVideo(
        config,
        BindToCurrentLoop(base::Bind(&MirrorService::OnOperationalStatusChange,
                                     base::Unretained(service_), is_audio_)),
        BindToCurrentLoop(
            base::Bind(&MirrorService::CreateVideoEncodeAccelerator,
                       base::Unretained(service_))),
        BindToCurrentLoop(base::Bind(&MirrorService::CreateVideoEncodeMemory,
                                     base::Unretained(service_))));
  }
}

void CastRtpStream::Stop() {
  LOG(INFO) << __func__;
  video_sink_.reset();
}

void CastRtpStream::StartDeliver() {
  LOG(INFO) << __func__;
  if (is_audio_) {
    DCHECK(audio_sink_);
    audio_sink_->AddToTrack(cast_sender_->audio_frame_input());
  } else {
    DCHECK(video_sink_);
    video_sink_->AddToTrack(cast_sender_->video_frame_input());
  }
}

scoped_refptr<CastVideoSink::Deliverer> CastRtpStream::GetVideoDeliverer() {
  DCHECK(!is_audio_);
  DCHECK(video_sink_);
  return video_sink_->deliverer();
}

void CastRtpStream::OnAudioData(const media::AudioBus& input_bus,
                                base::TimeTicks estimated_capture_time) {
  DCHECK(is_audio_);
  DCHECK(audio_sink_);
  audio_sink_->OnData(input_bus, estimated_capture_time);
}

void CastRtpStream::SetAudioParameters(const media::AudioParameters& params) {
  DCHECK(is_audio_);
  DCHECK(audio_sink_);
  // webrtc::AudioProcessing requires a 10 ms chunk size. We use this native
  // size when processing is enabled. When disabled we use the same size as
  // the source if less than 10 ms.
  //
  // TODO(ajm): This conditional buffer size appears to be assuming knowledge of
  // the sink based on the source parameters. PeerConnection sinks seem to want
  // 10 ms chunks regardless, while WebAudio sinks want less, and we're assuming
  // we can identify WebAudio sinks by the input chunk size. Less fragile would
  // be to have the sink actually tell us how much it wants (as in the above
  // todo).

  // int processing_frames = params.sample_rate() / 100;
  // int output_frames = params.sample_rate() / 100;
  // if (params.frames_per_buffer() < output_frames) {
  //   processing_frames = params.frames_per_buffer();
  //   output_frames = processing_frames;
  // }

  // const AudioParameters output_format(
  //     media::AudioParameters::AUDIO_PCM_LOW_LATENCY, params.channel_layout(),
  //     params.sample_rate(), 16, output_frames);

  // audio_sink_->OnSetFormat(output_format);

  audio_sink_->OnSetFormat(params);
}

}  // namespace media
