// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/WebKit/webagents/test_frame.h"

#include "core/testing/DummyPageHolder.h"

namespace webagents {

TestFrame::TestFrame(std::unique_ptr<blink::DummyPageHolder> dummy_page_holder)
    : Frame(dummy_page_holder->GetFrame()),
      dummy_page_holder_(std::move(dummy_page_holder)) {}

TestFrame::TestFrame(int width, int height)
    : TestFrame(blink::DummyPageHolder::Create(blink::IntSize(width, height))) {}

std::unique_ptr<Frame> TestFrame::FrameForTesting(int width, int height) {
  return std::make_unique<TestFrame>(width, height);
}

}  // namespace webagents
