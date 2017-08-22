// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/WebKit/webagents/agent.h"

#include "third_party/WebKit/Source/core/frame/LocalFrame.h"
#include "third_party/WebKit/Source/core/frame/WebLocalFrameImpl.h"

namespace webagents {

Agent::Agent(blink::WebLocalFrame& frame)
    : frame_(*new Frame(*ToWebLocalFrameImpl(frame).GetFrame())) {}

}  // namespace webagents
