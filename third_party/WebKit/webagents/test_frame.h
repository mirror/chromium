// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WEBAGENTS_TEST_FRAME_H_
#define WEBAGENTS_TEST_FRAME_H_

#include "third_party/WebKit/webagents/frame.h"
#include "third_party/WebKit/webagents/webagents_export.h"

namespace blink {
class DummyPageHolder;
}

namespace webagents {

class WEBAGENTS_EXPORT TestFrame : public Frame {
 public:
  explicit TestFrame(std::unique_ptr<blink::DummyPageHolder>);
  static std::unique_ptr<Frame> FrameForTesting(int width, int height);

 private:
  std::unique_ptr<blink::DummyPageHolder> dummy_page_holder_;
};

}  // namespace webagents

#endif  // WEBAGENTS_TEST_FRAME_H_
