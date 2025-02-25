// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDERER_MEDIA_RECORDER_H264_ENCODER_H_
#define CONTENT_RENDERER_MEDIA_RECORDER_H264_ENCODER_H_

#include "content/public/common/features.h"

#if !BUILDFLAG(RTC_USE_H264)
#error RTC_USE_H264 should be defined.
#endif  // #if BUILDFLAG(RTC_USE_H264)

#include "base/time/time.h"
#include "content/renderer/media_recorder/video_track_recorder.h"
#include "third_party/openh264/src/codec/api/svc/codec_api.h"

namespace content {

// Class encapsulating all openh264 interactions for H264 encoding.
class H264Encoder final : public VideoTrackRecorder::Encoder {
 public:
  struct ISVCEncoderDeleter {
    void operator()(ISVCEncoder* codec);
  };
  typedef std::unique_ptr<ISVCEncoder, ISVCEncoderDeleter> ScopedISVCEncoderPtr;

  static void ShutdownEncoder(std::unique_ptr<base::Thread> encoding_thread,
                              ScopedISVCEncoderPtr encoder);

  H264Encoder(
      const VideoTrackRecorder::OnEncodedVideoCB& on_encoded_video_callback,
      int32_t bits_per_second,
      scoped_refptr<base::SingleThreadTaskRunner> task_runner);

 private:
  // VideoTrackRecorder::Encoder implementation.
  ~H264Encoder() override;
  void EncodeOnEncodingTaskRunner(scoped_refptr<media::VideoFrame> frame,
                                  base::TimeTicks capture_timestamp) override;

  void ConfigureEncoderOnEncodingTaskRunner(const gfx::Size& size);

  // |openh264_encoder_| is a special scoped pointer to guarantee proper
  // destruction, also when reconfiguring due to parameters change. Only used on
  // VideoTrackRecorder::Encoder::encoding_thread_.
  gfx::Size configured_size_;
  ScopedISVCEncoderPtr openh264_encoder_;

  // The |VideoFrame::timestamp()| of the first received frame. Only used on
  // VideoTrackRecorder::Encoder::encoding_thread_.
  base::TimeTicks first_frame_timestamp_;

  DISALLOW_COPY_AND_ASSIGN(H264Encoder);
};

}  // namespace content

#endif  // CONTENT_RENDERER_MEDIA_RECORDER_H264_ENCODER_H_
