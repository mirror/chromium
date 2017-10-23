// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/public/test/test_runner_support.h"

#include "content/common/unique_name_helper.h"
#include "content/renderer/render_frame_impl.h"
#include "third_party/WebKit/public/web/WebLocalFrame.h"

namespace content {

std::string GetLayoutTestsNameForFrame(blink::WebLocalFrame* frame) {
  std::string unique_name = RenderFrameImpl::FromWebFrame(frame)->unique_name();

  // Layout tests require stable names in test output - strip the dynamic frame
  // suffix to achieve this.
  return UniqueNameHelper::RemoveDynamicFrameSuffixForTesting(unique_name);
}

}  // namespace content
