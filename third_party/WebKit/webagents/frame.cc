// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/WebKit/webagents/frame.h"

#include "core/frame/LocalFrame.h"
#include "services/service_manager/public/cpp/interface_provider.h"

namespace webagents {

Frame::~Frame() {}

Frame::Frame(blink::LocalFrame& frame) : frame_(frame) {}

service_manager::InterfaceProvider& Frame::GetInterfaceProvider() {
  return frame_.GetInterfaceProvider();
}

Document Frame::GetDocument() const {
  blink::Document* doc = frame_.GetDocument();
  DCHECK(doc);
  return Document(*doc);
}

bool Frame::IsMainFrame() const {
  return frame_.IsMainFrame();
}

}  // namespace webagents
