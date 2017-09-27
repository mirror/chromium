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
  bool IsOpacityMutated() const { return false; }
  bool IsScrollLeftMutated() const { return false; }
  bool IsScrollTopMutated() const { return false; }
  bool IsTransformMutated() const { return false; }

  float Opacity() const { return 0.0f; }
  float ScrollLeft() const { return 0.0f; }
  float ScrollTop() const { return 0.0f; }
  SkMatrix44 Transform() const {
    return SkMatrix44(SkMatrix44::kIdentity_Constructor);
  }
};

struct CompositorMutations {
  HashMap<uint64_t, std::unique_ptr<CompositorMutation>> map;
};

}  // namespace blink

#endif  // CompositorMutation_h
