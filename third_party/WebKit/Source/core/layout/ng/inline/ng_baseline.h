// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NGBaseline_h
#define NGBaseline_h

#include "platform/LayoutUnit.h"
#include "platform/fonts/FontBaseline.h"
#include "platform/wtf/Allocator.h"

namespace blink {

class LayoutBox;
class NGLayoutInputNode;

enum class NGBaselineAlgorithmType {
  kAtomicInline,
  kAtomicInlineForFirstLine,
  kFirstLine
};

struct NGBaselineRequest {
  NGBaselineAlgorithmType algorithm_type;
  FontBaseline baseline_type;
};

struct NGBaseline {
  NGBaselineAlgorithmType algorithm_type;
  FontBaseline baseline_type;
  LayoutUnit offset;

  static bool ShouldPropagateBaselines(const NGLayoutInputNode);
  static bool ShouldPropagateBaselines(const LayoutBox*);
};

}  // namespace blink

#endif  // NGBaseline_h
