// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WEBAGENTS_AGENT_H_
#define WEBAGENTS_AGENT_H_

#include "third_party/WebKit/public/web/WebLocalFrame.h"
#include "third_party/WebKit/webagents/frame.h"

namespace webagents {

class Agent {
 public:
  explicit Agent(blink::WebLocalFrame&);
  virtual ~Agent() = default;

 protected:
  Frame& GetFrame() const { return frame_; }

 private:
  Frame& frame_;
};

}  // namespace webagents

#endif  // WEBAGENTS_AGENT_H_
