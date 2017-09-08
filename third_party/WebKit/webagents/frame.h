// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WEBAGENTS_FRAME_H_
#define WEBAGENTS_FRAME_H_

#include "third_party/WebKit/webagents/document.h"
#include "third_party/WebKit/webagents/webagents_export.h"

namespace blink {
class LocalFrame;
}

namespace service_manager {
class InterfaceProvider;
}

namespace webagents {

class WEBAGENTS_EXPORT Frame {
 public:
  virtual ~Frame();
  explicit Frame(blink::LocalFrame&);

  service_manager::InterfaceProvider& GetInterfaceProvider();
  Document GetDocument() const;
  bool IsMainFrame() const;

 private:
  blink::LocalFrame& frame_;
};

}  // namespace webagents

#endif  // WEBAGENTS_FRAME_H_
