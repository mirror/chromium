// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "remoting/protocol/webrtc_frame_scheduler.h"

#include <algorithm>
#include <memory>

#include "base/location.h"
#include "base/logging.h"
#include "base/single_thread_task_runner.h"
#include "base/threading/thread_task_runner_handle.h"
#include "remoting/base/constants.h"
#include "remoting/proto/video.pb.h"

namespace remoting {
namespace protocol {

namespace {

// Default target bitrate in kbps
const int kDefaultTargetBitrateKbps = 1000;

}  // namespace

// The frame scheduler currently uses a simple polling technique
// at 30 FPS to capture, encode and send frames over webrtc transport.
// An improved solution will use target bitrate feedback to pace out
// the capture rate.
WebrtcFrameScheduler::WebrtcFrameScheduler(
    scoped_refptr<base::SingleThreadTaskRunner> encode_task_runner,
    std::unique_ptr<webrtc::DesktopCapturer> capturer,
    WebrtcTransport* webrtc_transport,
    std::unique_ptr<VideoEncoder> encoder)
    : target_bitrate_kbps_(kDefaultTargetBitrateKbps),
      main_task_runner_(base::ThreadTaskRunnerHandle::Get()),
      encode_task_runner_(encode_task_runner),
      capturer_(std::move(capturer)),
      webrtc_transport_(webrtc_transport),
      encoder_(std::move(encoder)),
      weak_factory_(this) {
  DCHECK(encode_task_runner_);
  DCHECK(capturer_);
  DCHECK(webrtc_transport_);
  DCHECK(encoder_);
  // Does not really start anything. Registers callback on this class.
  capturer_->Start(this);
  capture_timer_.reset(new base::RepeatingTimer());
}

WebrtcFrameScheduler::~WebrtcFrameScheduler() {
  encode_task_runner_->DeleteSoon(FROM_HERE, encoder_.release());
}

void WebrtcFrameScheduler::Start() {
  // Register for PLI requests.
  webrtc_transport_->video_encoder_factory()->SetKeyFrameRequestCallback(
      base::Bind(&WebrtcFrameScheduler::SetKeyFrameRequest,
                 base::Unretained(this)));
  // Register for target bitrate notifications.
  webrtc_transport_->video_encoder_factory()->SetTargetBitrateCallback(
      base::Bind(&WebrtcFrameScheduler::SetTargetBitrate,
                 base::Unretained(this)));
}

void WebrtcFrameScheduler::Stop() {
  // Clear PLI request callback.
  webrtc_transport_->video_encoder_factory()->SetKeyFrameRequestCallback(
      base::Closure());
  webrtc_transport_->video_encoder_factory()->SetTargetBitrateCallback(
      TargetBitrateCallback());
  // Cancel any pending encode.
  task_tracker_.TryCancelAll();
  capture_timer_->Stop();
}

void WebrtcFrameScheduler::Pause(bool pause) {
  if (pause) {
    Stop();
  } else {
    Start();
  }
}

void WebrtcFrameScheduler::SetSizeCallback(
    const VideoStream::SizeCallback& callback) {
  size_callback_ = callback;
}

void WebrtcFrameScheduler::SetKeyFrameRequest() {
  VLOG(1) << "Request key frame";
  base::AutoLock lock(lock_);
  key_frame_request_ = true;
  if (!received_first_frame_request_) {
    received_first_frame_request_ = true;
    main_task_runner_->PostTask(
        FROM_HERE, base::Bind(&WebrtcFrameScheduler::StartCaptureTimer,
                              weak_factory_.GetWeakPtr()));
  }
}

void WebrtcFrameScheduler::StartCaptureTimer() {
  capture_timer_->Start(FROM_HERE, base::TimeDelta::FromSeconds(1) / 30, this,
                        &WebrtcFrameScheduler::CaptureNextFrame);
}

void WebrtcFrameScheduler::SetTargetBitrate(int target_bitrate_kbps) {
  VLOG(1) << "Set Target bitrate " << target_bitrate_kbps;
  base::AutoLock lock(lock_);
  target_bitrate_kbps_ = target_bitrate_kbps;
}

bool WebrtcFrameScheduler::ClearAndGetKeyFrameRequest() {
  base::AutoLock lock(lock_);
  bool key_frame_request = key_frame_request_;
  key_frame_request_ = false;
  return key_frame_request;
}

void WebrtcFrameScheduler::OnCaptureCompleted(webrtc::DesktopFrame* frame) {
  DCHECK(thread_checker_.CalledOnValidThread());

  base::TimeTicks captured_ticks = base::TimeTicks::Now();
  int64_t capture_timestamp_ms =
      (captured_ticks - base::TimeTicks()).InMilliseconds();
  capture_pending_ = false;

  std::unique_ptr<webrtc::DesktopFrame> owned_frame(frame);

  if (encode_pending_) {
    // TODO(isheriff): consider queuing here
    VLOG(1) << "Dropping captured frame since encoder is still busy";
    return;
  }

  last_capture_completed_ticks_ = captured_ticks;

  webrtc::DesktopVector dpi =
      frame->dpi().is_zero() ? webrtc::DesktopVector(kDefaultDpi, kDefaultDpi)
                             : frame->dpi();

  if (!frame_size_.equals(frame->size()) || !frame_dpi_.equals(dpi)) {
    frame_size_ = frame->size();
    frame_dpi_ = dpi;
    if (!size_callback_.is_null())
      size_callback_.Run(frame_size_, frame_dpi_);
  }
  encode_pending_ = true;
  task_tracker_.PostTaskAndReplyWithResult(
      encode_task_runner_.get(), FROM_HERE,
      base::Bind(&WebrtcFrameScheduler::EncodeFrame, encoder_.get(),
                 base::Passed(std::move(owned_frame)), target_bitrate_kbps_,
                 ClearAndGetKeyFrameRequest(), capture_timestamp_ms),
      base::Bind(&WebrtcFrameScheduler::OnFrameEncoded,
                 weak_factory_.GetWeakPtr()));
}

void WebrtcFrameScheduler::CaptureNextFrame() {
  DCHECK(thread_checker_.CalledOnValidThread());

  if (capture_pending_ || encode_pending_) {
    VLOG(1) << "Capture/encode still pending..";
    return;
  }

  capture_pending_ = true;
  VLOG(1) << "Capture next frame after "
          << (base::TimeTicks::Now() - last_capture_started_ticks_)
                 .InMilliseconds();
  last_capture_started_ticks_ = base::TimeTicks::Now();
  capturer_->Capture(webrtc::DesktopRegion());
}

// static
std::unique_ptr<VideoPacket> WebrtcFrameScheduler::EncodeFrame(
    VideoEncoder* encoder,
    std::unique_ptr<webrtc::DesktopFrame> frame,
    uint32_t target_bitrate_kbps,
    bool key_frame_request,
    int64_t capture_time_ms) {
  uint32_t flags = 0;
  if (key_frame_request)
    flags |= VideoEncoder::REQUEST_KEY_FRAME;

  base::TimeTicks current = base::TimeTicks::Now();
  encoder->UpdateTargetBitrate(target_bitrate_kbps);
  std::unique_ptr<VideoPacket> packet = encoder->Encode(*frame, flags);
  if (!packet)
    return nullptr;
  // TODO(isheriff): Note that while VideoPacket capture time is supposed
  // to be capture duration, we (ab)use it for capture timestamp here. This
  // will go away when we move away from VideoPacket.
  packet->set_capture_time_ms(capture_time_ms);

  VLOG(1) << "Encode duration "
          << (base::TimeTicks::Now() - current).InMilliseconds()
          << " payload size " << packet->data().size();
  return packet;
}

void WebrtcFrameScheduler::OnFrameEncoded(std::unique_ptr<VideoPacket> packet) {
  DCHECK(thread_checker_.CalledOnValidThread());
  encode_pending_ = false;
  if (!packet)
    return;
  base::TimeTicks current = base::TimeTicks::Now();
  float encoded_bits = packet->data().size() * 8.0;

  // Simplistic adaptation of frame polling in the range 5 FPS to 30 FPS.
  uint32_t next_sched_ms = std::max(
      33, std::min(static_cast<int>(encoded_bits / target_bitrate_kbps_), 200));
  if (webrtc_transport_->video_encoder_factory()->SendEncodedFrame(
          std::move(packet)) >= 0) {
    VLOG(1) << " Send duration "
            << (base::TimeTicks::Now() - current).InMilliseconds()
            << "next sched " << next_sched_ms;
  } else {
    LOG(ERROR) << "SendEncodedFrame() failed";
  }
  capture_timer_->Start(FROM_HERE,
                        base::TimeDelta::FromMilliseconds(next_sched_ms), this,
                        &WebrtcFrameScheduler::CaptureNextFrame);
}

}  // namespace protocol
}  // namespace remoting
