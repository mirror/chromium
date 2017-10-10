// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "remoting/host/ipc_video_frame_capturer.h"

#include "base/logging.h"
#include "base/time/time.h"
#include "remoting/host/desktop_session_proxy.h"
#include "third_party/webrtc/modules/desktop_capture/desktop_frame.h"

namespace remoting {

IpcVideoFrameCapturer::IpcVideoFrameCapturer(
    scoped_refptr<DesktopSessionProxy> desktop_session_proxy)
    : callback_(nullptr),
      desktop_session_proxy_(desktop_session_proxy),
      capture_pending_(false),
      weak_factory_(this) {
}

IpcVideoFrameCapturer::~IpcVideoFrameCapturer() {
}

void IpcVideoFrameCapturer::Start(Callback* callback) {
  DCHECK(!callback_);
  DCHECK(callback);
  callback_ = callback;
  desktop_session_proxy_->SetVideoCapturer(weak_factory_.GetWeakPtr());
}

void IpcVideoFrameCapturer::CaptureFrame() {
  DCHECK(!capture_pending_);
  capture_pending_ = true;
  capture_start_ = base::Time::Now();
  desktop_session_proxy_->CaptureFrame();
}

void IpcVideoFrameCapturer::OnCaptureResult(
    webrtc::DesktopCapturer::Result result,
    std::unique_ptr<webrtc::DesktopFrame> frame) {
  DCHECK(capture_pending_);
  capture_pending_ = false;
  const base::Time capture_finish = base::Time::Now();
  if (capture_finish - capture_start_ >= base::TimeDelta::FromMilliseconds(500)) {
    LOG(ERROR) << "Debugging: IpcVideoFrameCapturer takes longer than expected "
               << capture_finish - capture_start_;
  }
  callback_->OnCaptureResult(result, std::move(frame));
}

bool IpcVideoFrameCapturer::GetSourceList(SourceList* sources) {
  NOTIMPLEMENTED();
  return false;
}

bool IpcVideoFrameCapturer::SelectSource(SourceId id) {
  NOTIMPLEMENTED();
  return false;
}

}  // namespace remoting
