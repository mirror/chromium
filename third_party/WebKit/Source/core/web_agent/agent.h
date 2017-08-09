// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WEB_AGENT_AGENT_H_
#define WEB_AGENT_AGENT_H_

#include "core/frame/LocalFrame.h"
#include "core/web_agent/api/frame.h"
#include "platform/heap/Handle.h"
#include "public/web/WebMeaningfulLayout.h"

namespace web {

class Agent : public blink::GarbageCollectedFinalized<Agent> {
 public:
  explicit Agent(Frame* frame) : frame_(frame) {}
  virtual ~Agent() = default;

  DEFINE_INLINE_TRACE() { visitor->Trace(frame_); }

  virtual void DidMeaningfulLayout(blink::WebMeaningfulLayout) {}

 protected:
  Frame* GetFrame() const { return frame_; }

 private:
  blink::Member<Frame> frame_;
};

}  // namespace web
#endif  // WEB_AGENT_AGENT_H_
