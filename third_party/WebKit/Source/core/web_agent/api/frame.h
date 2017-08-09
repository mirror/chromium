// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WEB_AGENT_API_FRAME_H_
#define WEB_AGENT_API_FRAME_H_

#include "core/web_agent/api/document.h"
#include "platform/heap/Handle.h"

namespace blink {
class LocalFrame;
}

namespace service_manager {
class InterfaceProvider;
}

namespace web {

class Frame : public blink::GarbageCollected<Frame> {
 public:
  explicit Frame(blink::LocalFrame*);
  ~Frame();

  DECLARE_TRACE();

  service_manager::InterfaceProvider& GetInterfaceProvider();
  Document* GetDocument() const;

 private:
  blink::Member<blink::LocalFrame> frame_;
};

}  // namespace web
#endif  // WEB_AGENT_API_FRAME_H_
