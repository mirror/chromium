// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "public/web/WebFrameHandle.h"
#include "core/frame/WebLocalFrameBase.h"
#include "core/frame/WebRemoteFrameBase.h"

namespace blink {

WebFrameHandle::~WebFrameHandle() {
  // TODO(dcheng): Dispose the stuffs.
}

WebFrameHandle::WebFrameHandle(WebFrameHandle&& rhs)
    : local_frame_(rhs.local_frame_.Get()),
      remote_frame_(rhs.remote_frame_.Get()) {
  rhs.local_frame_ = nullptr;
  rhs.remote_frame_ = nullptr;
}

WebFrameHandle& WebFrameHandle::operator=(WebFrameHandle&& rhs) {
  // For simplicity, don't support overwriting non-null values, since this can
  // complicate disposal..
  DCHECK(!local_frame_.Get());
  DCHECK(!remote_frame_.Get());

  local_frame_ = rhs.local_frame_.Get();
  remote_frame_ = rhs.remote_frame_.Get();

  rhs.local_frame_.Reset();
  rhs.remote_frame_.Reset();

  return *this;
}

WebFrameHandle::WebFrameHandle(WebLocalFrameBase& local_frame)
    : local_frame_(&local_frame) {}

WebFrameHandle::WebFrameHandle(WebRemoteFrameBase& remote_frame)
    : remote_frame_(&remote_frame) {}

}  // namespace blink
