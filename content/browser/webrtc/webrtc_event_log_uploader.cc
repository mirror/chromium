// Copyright (c) 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/webrtc/webrtc_event_log_uploader.h"

// TODO: !!! Retry limit?

namespace content {

WebRtcEventLogUploaderImpl::WebRtcEventLogUploaderImpl(
    WebRtcEventLogUploaderObserver* observer,
    base::FilePath path) {}

WebRtcEventLogUploaderImpl::~WebRtcEventLogUploaderImpl() = default;

}  // namespace content
