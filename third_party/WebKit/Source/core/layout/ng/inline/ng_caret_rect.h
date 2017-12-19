// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NGCaretRect_h
#define NGCaretRect_h

#include "core/editing/Forward.h"
#include "core/layout/ng/geometry/ng_physical_offset_rect.h"
#include "platform/wtf/Optional.h"

namespace blink {

// This file provides utility functions for computing caret rect in LayoutNG.

class LayoutBlockFlow;
struct LocalCaretRect;
class NGPhysicalFragment;

// TODO(xiaochengh): Documentation.
LocalCaretRect ComputeNGLocalCaretRect(const LayoutBlockFlow&,
                                       const PositionWithAffinity&);
LocalCaretRect ComputeNGLocalCaretRect(const LayoutBlockFlow&,
                                       const PositionInFlatTreeWithAffinity&);

// Below are exposed to tests only
enum class CaretPositionType { kBeforeBox, kAfterBox, kAtTextOffset };
struct CaretPosition {
  const NGPhysicalFragment* fragment = nullptr;
  NGPhysicalOffset offset_to_container_box;
  CaretPositionType position_type;
  Optional<unsigned> text_offset;
};

// Exposed to unit tests only
CORE_EXPORT CaretPosition ComputeNGCaretPosition(const LayoutBlockFlow&,
                                                 unsigned,
                                                 TextAffinity);

}  // namespace blink

#endif  // NGCaretRect_h
