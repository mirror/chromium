// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ScrollIntoViewParams_h
#define ScrollIntoViewParams_h

#include "platform/scroll/ScrollAlignment.h"
#include "platform/scroll/ScrollTypes.h"
#include "platform/wtf/Allocator.h"

namespace blink {

struct ScrollIntoViewParams {
  STACK_ALLOCATED();

  ScrollAlignment align_x = ScrollAlignment::kAlignCenterIfNeeded;
  ScrollAlignment align_y = ScrollAlignment::kAlignCenterIfNeeded;
  ScrollType scroll_type = kProgrammaticScroll;
  bool make_visible_in_visual_viewport = true;
  ScrollBehavior scroll_behavior = kScrollBehaviorAuto;
  bool is_for_scroll_sequence = false;
};

}  // namespace blink

#endif  // ScrollIntoViewParams_h
