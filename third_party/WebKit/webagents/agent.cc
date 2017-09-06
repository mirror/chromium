// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/WebKit/webagents/agent.h"
#include "third_party/WebKit/webagents/frame.h"

#include "core/frame/LocalFrame.h"
#include "core/frame/WebLocalFrameImpl.h"

namespace webagents {

// Note: WebLocalFrameImpl::GetFrame may not exist when webagent is created, so
// we must keep reference to WebLocalFrame and then create webagents::Frame
// lazily.
Agent::Agent(blink::WebLocalFrame& web_local_frame)
    : web_local_frame_(web_local_frame) {}

Agent::~Agent() {}

Frame& Agent::GetFrame() {
  if (!frame_) {
    blink::LocalFrame* local_frame =
        ToWebLocalFrameImpl(web_local_frame_).GetFrame();
    DCHECK(local_frame);
    frame_ = std::make_unique<Frame>(*local_frame);
  }
  return *frame_;
}

}  // namespace webagents
