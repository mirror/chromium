// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/layout/ng/inline/ng_caret_rect.h"

#include "core/layout/LayoutBlockFlow.h"
#include "core/layout/ng/ng_layout_test.h"

// Test cases:
//
// A/B. Character types before/after offset
//  1. Normal rendered text
//  2. Rendered text in first-letter
//  3. Unrendered whitespace due to line wrapping
//  4. Hard linebreak
//  5. Generated text
//  6. Object replacement character for inline block
//  7. Object replacement character for other block layout roots (float or oof)
// We ignore Generated BiDi control for now since it's for mixed BiDi handling
//
// C. Position relative to fragments
//  1. In the middle of a fragment
//  2. At fragment begin
//  3. At fragment end
//  4. Not in any fragment (is this really possible)
//
// D. TextAffinity
//  1. Downstream
//  2. Upstream

namespace blink {

class NGCaretRectTest : public NGLayoutTest,
                        private ScopedLayoutNGPaintFragmentsForTest {
 public:
  NGCaretRectTest()
      : NGLayoutTest(), ScopedLayoutNGPaintFragmentsForTest(true) {}

 protected:
  void SetInlineFormattingContext(const char* id, const char* html) {
    SetBodyInnerHTML(html);
    context_ = ToLayoutBlockFlow(GetLayoutObjectByElementId(id));
    DCHECK(context_);
    DCHECK(context_->IsLayoutNGMixin());
  }

  CaretPosition ComputeNGCaretPosition(unsigned offset,
                                       TextAffinity affinity) const {
    return blink::ComputeNGCaretPosition(*context_, offset, affinity);
  }

  const LayoutBlockFlow* context_;
};

TEST_F(NGCaretRectTest, A1B1C1) {
  SetInlineFormattingContext("t", "<div id=t>foo</div>");
  auto downstream_position =
      ComputeNGCaretPosition(1, TextAffinity::kDownstream);
  auto upstream_position = ComputeNGCaretPosition(1, TextAffinity::kUpstream);

  EXPECT_EQ(downstream_position.fragment, upstream_position.fragment);
}

}  // namespace blink
