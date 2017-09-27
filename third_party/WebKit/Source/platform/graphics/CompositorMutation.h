// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CompositorMutation_h
#define CompositorMutation_h

#include <memory>
#include "platform/wtf/HashMap.h"
#include "third_party/skia/include/core/SkMatrix44.h"

namespace blink {

class CompositorMutation {
 public:
  bool IsOpacityMutated() const { return mutated_flags_ & 4; }
  bool IsScrollLeftMutated() const { return mutated_flags_ & 1; }
  bool IsScrollTopMutated() const { return mutated_flags_ & 2; }
  bool IsTransformMutated() const { return mutated_flags_ & 8; }

  float Opacity() const { return opacity_; }
  float ScrollLeft() const { return scroll_left_; }
  float ScrollTop() const { return scroll_top_; }
  SkMatrix44 Transform() const { return transform_; }

 private:
  uint32_t mutated_flags_ = 0;
  float opacity_ = 0;
  float scroll_left_ = 0;
  float scroll_top_ = 0;
  SkMatrix44 transform_;
};

struct CompositorMutations {
  HashMap<uint64_t, std::unique_ptr<CompositorMutation>> map;
};

}  // namespace blink

#endif  // CompositorMutation_h
