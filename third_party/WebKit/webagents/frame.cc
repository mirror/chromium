// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/WebKit/webagents/frame.h"

#include "services/service_manager/public/cpp/interface_provider.h"
#include "third_party/WebKit/Source/core/frame/LocalFrame.h"

namespace webagents {

Frame::Frame(blink::LocalFrame* frame) : frame_(frame) {}

Frame::~Frame() {}

service_manager::InterfaceProvider& Frame::GetInterfaceProvider() {
  return frame_->GetInterfaceProvider();
}

Document* Frame::GetDocument() const {
  return Document::Create(frame_->GetDocument());
}
}  // namespace webagents
