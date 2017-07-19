// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NGIntrinsicSize_h
#define NGIntrinsicSize_h

#include "core/CoreExport.h"
#include "core/layout/ng/geometry/ng_logical_size.h"
#include "platform/LayoutUnit.h"

namespace blink {

// Represents intrinsic size information for replaced elements.
struct CORE_EXPORT NGIntrinsicSize {
  NGLogicalSize default_size;
  Optional<LayoutUnit> computed_block_size;
  Optional<LayoutUnit> computed_inline_size;
  NGLogicalSize aspect_ratio;
};

}  // namespace blink

#endif  // NGIntrinsicSize_h
