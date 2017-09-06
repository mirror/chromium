// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WEBAGENTS_AGENT_H_
#define WEBAGENTS_AGENT_H_

#include "third_party/WebKit/public/web/WebLocalFrame.h"
#include "third_party/WebKit/webagents/webagents_export.h"

namespace webagents {

class Frame;

class WEBAGENTS_EXPORT Agent {
 public:
  virtual ~Agent();
  Frame& GetFrame();

 protected:
  explicit Agent(blink::WebLocalFrame&);

 private:
  blink::WebLocalFrame& web_local_frame_;
  std::unique_ptr<Frame> frame_;
};

}  // namespace webagents

#endif  // WEBAGENTS_AGENT_H_
