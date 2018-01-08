// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NGCaretRect_h
#define NGCaretRect_h

#include "core/editing/Forward.h"
#include "core/layout/ng/geometry/ng_physical_offset_rect.h"
#include "core/layout/ng/ng_physical_fragment.h"
#include "platform/wtf/Forward.h"
#include "platform/wtf/Optional.h"

namespace blink {

// This file provides utility functions for computing caret rect in LayoutNG.

class LayoutBlockFlow;
struct LocalCaretRect;

// Given an inline formatting context and a position in the context, returns the
// caret rect if a caret should be placed at the position, with the given
// affinity. The caret rect location is local to the given formatting context.
LocalCaretRect ComputeNGLocalCaretRect(const LayoutBlockFlow&,
                                       const PositionWithAffinity&);

// Converts the PositionInFlatTree into Position (in DOM) and then calls the
// above function. This is a shorthand helping template functions in editing
// code, so that they don't need to do the conversion by themselves.
LocalCaretRect ComputeNGLocalCaretRect(const LayoutBlockFlow&,
                                       const PositionInFlatTreeWithAffinity&);

// An NGCaretPosition indicates a caret position relative to an inline
// NGPhysicalFragment:
// - When |fragment| is box, |position_type| is either |kBeforeBox| or
// |kAfterBox|, indicating either of the two caret positions by the box sides;
// |text_offset| is |nullopt| in this case.
// - When |fragment| is text, |position_type| is |kAtTextOffset|, and
// |text_offset| is in the text offset range of the fragment.
// - |offset_to_container_box| caches the (geometric) offset of |fragment| to
// its inline formatting context, since |fragment| itself doesn't store it.
//
// TODO(xiaochengh): Try not to store |offset_to_container_box| without
// performance regression.
// TODO(xiaochengh): Support "in empty container" caret type

enum class NGCaretPositionType { kBeforeBox, kAfterBox, kAtTextOffset };
struct NGCaretPosition {
  DISALLOW_NEW_EXCEPT_PLACEMENT_NEW();

  scoped_refptr<const NGPhysicalFragment> fragment;
  NGPhysicalOffset offset_to_container_box;
  NGCaretPositionType position_type;
  Optional<unsigned> text_offset;
};

// Given an inline formatting context, a text offset in the context and a text
// affinity, returns the corresponding NGCaretPosition, or null if not found.
CORE_EXPORT NGCaretPosition ComputeNGCaretPosition(const LayoutBlockFlow&,
                                                   unsigned,
                                                   TextAffinity);

}  // namespace blink

#endif  // NGCaretRect_h
