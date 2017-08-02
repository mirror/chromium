// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "remoting/codec/video_encoder_verbatim.h"

#include <stddef.h>
#include <stdint.h>

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/logging.h"
#include "base/memory/ptr_util.h"
#include "base/sequenced_task_runner.h"
#include "base/stl_util.h"
#include "base/task_scheduler/post_task.h"
#include "base/task_scheduler/task_traits.h"
#include "remoting/base/util.h"
#include "remoting/codec/video_encoder_webrtc_video_encoder_gpu.h"
#include "remoting/proto/video.pb.h"
#include "third_party/webrtc/modules/desktop_capture/desktop_frame.h"
#include "third_party/webrtc/modules/desktop_capture/desktop_geometry.h"
#include "third_party/webrtc/modules/desktop_capture/desktop_region.h"

namespace remoting {

VideoEncoderWebrtcVideoEncoderGpu::VideoEncoderWebrtcVideoEncoderGpu()
    : encoder_(WebrtcVideoEncoderGpu::CreateForH264()),
      task_runner_(
          base::CreateSequencedTaskRunnerWithTraits(base::TaskTraits())),
      gpu_encode_thread_("gpu_encode") {
  gpu_encode_thread_.Start();
};
VideoEncoderWebrtcVideoEncoderGpu::~VideoEncoderWebrtcVideoEncoderGpu() {}

std::unique_ptr<VideoPacket> VideoEncoderWebrtcVideoEncoderGpu::Encode(
    const webrtc::DesktopFrame& frame) {
  base::WaitableEvent event(base::WaitableEvent::ResetPolicy::AUTOMATIC,
                            base::WaitableEvent::InitialState::NOT_SIGNALED);
  gpu_encode_thread_.task_runner()->PostTask(
      FROM_HERE,
      base::Bind(&VideoEncoderWebrtcVideoEncoderGpu::DoEncode,
                 base::Unretained(this), base::ConstRef(frame), &event));
  event.Wait();

  // can we just do this?
  return base::MakeUnique<VideoPacket>();
}

void VideoEncoderWebrtcVideoEncoderGpu::DoEncode(
    const webrtc::DesktopFrame& frame,
    base::WaitableEvent* event) {
  encoder_->Encode(base::WrapUnique(webrtc::BasicDesktopFrame::CopyOf(frame)),
                   WebrtcVideoEncoder::FrameParams(),
                   base::Bind(&VideoEncoderWebrtcVideoEncoderGpu::Callback,
                              base::Unretained(this), event));
}

void VideoEncoderWebrtcVideoEncoderGpu::Callback(
    base::WaitableEvent* event,
    std::unique_ptr<WebrtcVideoEncoder::EncodedFrame> frame) {
  encoded_frame_ = std::move(frame);
  event->Signal();
}

}  // namespace remoting
