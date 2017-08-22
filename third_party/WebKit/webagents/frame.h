// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WEBAGENTS_FRAME_H_
#define WEBAGENTS_FRAME_H_

#include "third_party/WebKit/webagents/document.h"

namespace blink {
class LocalFrame;
}

namespace service_manager {
class InterfaceProvider;
}

namespace webagents {

class Frame {
 public:
  explicit Frame(blink::LocalFrame&);
  ~Frame() = default;

  static std::unique_ptr<Frame> FrameForTesting(int width, int height);

  service_manager::InterfaceProvider& GetInterfaceProvider();
  // TODO: I don't want to give up ownership of document,
  // but I want others to use it within lifetime of frame.
  Document* GetDocument() const { return document_.get(); }
  bool IsMainFrame() const;

 private:
  blink::LocalFrame& frame_;
  std::unique_ptr<Document> document_;
};

}  // namespace webagents

#endif  // WEBAGENTS_FRAME_H_
