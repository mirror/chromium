// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/WebKit/webagents/frame.h"

#include "services/service_manager/public/cpp/interface_provider.h"
#include "third_party/WebKit/Source/core/frame/LocalFrame.h"
#include "third_party/WebKit/Source/core/testing/DummyPageHolder.h"

namespace webagents {

Frame::Frame(blink::LocalFrame& frame) : frame_(frame) {}

class TestFrame : public Frame {
 public:
  TestFrame(std::unique_ptr<blink::DummyPageHolder> dummy_page_holder)
      : Frame(dummy_page_holder->GetFrame()) {
    dummy_page_holder_ = std::move(dummy_page_holder);
  }
  std::unique_ptr<blink::DummyPageHolder> dummy_page_holder_;
};

Frame& Frame::FrameForTesting(int width, int height) {
  return *new TestFrame(
      blink::DummyPageHolder::Create(blink::IntSize(width, height)));
}

service_manager::InterfaceProvider& Frame::GetInterfaceProvider() {
  return frame_.GetInterfaceProvider();
}

Document* Frame::GetDocument() const {
  return Document::Create(frame_.GetDocument());
}

bool Frame::IsMainFrame() const {
  return frame_.IsMainFrame();
}

}  // namespace webagents
