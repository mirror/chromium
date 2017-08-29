// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/WebKit/webagents/frame.h"

#include "core/frame/LocalFrame.h"
#include "services/service_manager/public/cpp/interface_provider.h"

namespace webagents {

Frame::Frame(blink::LocalFrame& frame) : frame_(frame) {}

Frame::~Frame() {}

service_manager::InterfaceProvider& Frame::GetInterfaceProvider() {
  return frame_.GetInterfaceProvider();
}

std::unique_ptr<Document> Frame::GetDocument() const {
  if (!frame_.GetDocument())
    return nullptr;
  return std::make_unique<Document>(frame_.GetDocument());
}

bool Frame::IsMainFrame() const {
  return frame_.IsMainFrame();
}

}  // namespace webagents
