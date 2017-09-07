// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NGBaseFragmentBuilder_h
#define NGBaseFragmentBuilder_h

#include "core/CoreExport.h"
#include "core/layout/ng/ng_out_of_flow_positioned_descendant.h"
#include "core/layout/ng/ng_writing_mode.h"
#include "core/layout/ng/ng_unpositioned_float.h"
#include "core/style/ComputedStyle.h"
#include "platform/text/TextDirection.h"
#include "platform/wtf/Allocator.h"
#include "platform/wtf/RefPtr.h"

namespace blink {

class NGExclusionSpace;

class CORE_EXPORT NGBaseFragmentBuilder {
  STACK_ALLOCATED();

 public:
  const ComputedStyle& Style() const {
    DCHECK(style_);
    return *style_;
  }
  NGBaseFragmentBuilder& SetStyle(RefPtr<const ComputedStyle>);

  LayoutUnit InlineSize() const { return inline_size_; }
  NGBaseFragmentBuilder& SetInlineSize(LayoutUnit);

  virtual NGLogicalSize Size() const = 0;

  const WTF::Optional<NGBfcOffset>& BfcOffset() const { return bfc_offset_; }
  NGBaseFragmentBuilder& SetBfcOffset(const NGBfcOffset& offset);

  NGWritingMode WritingMode() const { return writing_mode_; }
  TextDirection Direction() const { return direction_; }

  NGBaseFragmentBuilder& SetExclusionSpace(
      std::unique_ptr<const NGExclusionSpace> exclusion_space);

  NGBaseFragmentBuilder& SwapUnpositionedFloats(
      Vector<RefPtr<NGUnpositionedFloat>>* unpositioned_floats) {
    unpositioned_floats_.swap(*unpositioned_floats);
    return *this;
  }

  NGBaseFragmentBuilder& SetEndMarginStrut(const NGMarginStrut& from) {
    end_margin_strut_ = from;
    return *this;
  }

  NGBaseFragmentBuilder& AddOutOfFlowDescendant(NGOutOfFlowPositionedDescendant);

  virtual NGBaseFragmentBuilder& AddChild(RefPtr<NGLayoutResult>, const NGLogicalOffset&);
  virtual NGBaseFragmentBuilder& AddChild(RefPtr<NGPhysicalFragment>,
                                  const NGLogicalOffset&);

  RefPtr<NGLayoutResult> Abort(NGLayoutResult::NGLayoutResultStatus);

 protected:
  // An out-of-flow positioned-candidate is a temporary data structure used
  // within the NGFragmentBuilder.
  //
  // A positioned-candidate can be:
  // 1. A direct out-of-flow positioned child. The child_offset is (0,0).
  // 2. A fragment containing an out-of-flow positioned-descendant. The
  //    child_offset in this case is the containing fragment's offset.
  //
  // The child_offset is stored as a NGLogicalOffset as the physical offset
  // cannot be computed until we know the current fragment's size.
  //
  // When returning the positioned-candidates (from
  // GetAndClearOutOfFlowDescendantCandidates), the NGFragmentBuilder will
  // convert the positioned-candidate to a positioned-descendant using the
  // physical size the fragment builder.
  struct NGOutOfFlowPositionedCandidate {
    NGOutOfFlowPositionedDescendant descendant;
    NGLogicalOffset child_offset;
  };

  NGBaseFragmentBuilder(RefPtr<const ComputedStyle>,
                        NGWritingMode,
                        TextDirection);
  NGBaseFragmentBuilder(NGWritingMode, TextDirection);
  ~NGBaseFragmentBuilder();

  // Floats that need to be positioned by the next in-flow fragment that can
  // determine its block position in space.
  Vector<RefPtr<NGUnpositionedFloat>> unpositioned_floats_;

  Vector<NGOutOfFlowPositionedCandidate> oof_positioned_candidates_;
  Vector<NGOutOfFlowPositionedDescendant> oof_positioned_descendants_;
  std::unique_ptr<const NGExclusionSpace> exclusion_space_;
  NGMarginStrut end_margin_strut_;

  Vector<RefPtr<NGPhysicalFragment>> children_;
  Vector<NGLogicalOffset> offsets_;

  LayoutUnit inline_size_;
  WTF::Optional<NGBfcOffset> bfc_offset_;

 private:
  RefPtr<const ComputedStyle> style_;
  NGWritingMode writing_mode_;
  TextDirection direction_;
};

}  // namespace blink

#endif  // NGBaseFragmentBuilder
