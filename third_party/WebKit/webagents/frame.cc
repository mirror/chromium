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
      : Frame(dummy_page_holder->GetFrame()),
        dummy_page_holder_(std::move(dummy_page_holder)) {}

 private:
  std::unique_ptr<blink::DummyPageHolder> dummy_page_holder_;
};

std::unique_ptr<Frame> Frame::FrameForTesting(int width, int height) {
  return std::make_unique<TestFrame>(
      blink::DummyPageHolder::Create(blink::IntSize(width, height)));
}

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
