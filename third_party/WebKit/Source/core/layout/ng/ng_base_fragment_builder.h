// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NGBaseFragmentBuilder_h
#define NGBaseFragmentBuilder_h

#include "core/CoreExport.h"
#include "core/layout/ng/geometry/ng_bfc_offset.h"
#include "core/layout/ng/geometry/ng_logical_size.h"
#include "core/layout/ng/geometry/ng_margin_strut.h"
#include "core/layout/ng/ng_layout_result.h"
#include "core/layout/ng/ng_physical_fragment.h"
#include "core/layout/ng/ng_writing_mode.h"
#include "core/style/ComputedStyle.h"
#include "platform/text/TextDirection.h"
#include "platform/wtf/Allocator.h"
#include "platform/wtf/RefPtr.h"

namespace blink {

class NGExclusionSpace;
struct NGUnpositionedFloat;

class CORE_EXPORT NGBaseFragmentBuilder {
  STACK_ALLOCATED();
 public:
  virtual ~NGBaseFragmentBuilder();

  const ComputedStyle& Style() const {
    DCHECK(style_);
    return *style_;
  }
  NGBaseFragmentBuilder& SetStyle(RefPtr<const ComputedStyle>);

  LayoutUnit InlineSize() const { return inline_size_; }
  NGBaseFragmentBuilder& SetInlineSize(LayoutUnit);

  virtual NGLogicalSize Size() const = 0;

  // The NGBfcOffset is where this fragment was positioned within the BFC. If
  // it is not set, this fragment may be placed anywhere within the BFC.
  const WTF::Optional<NGBfcOffset>& BfcOffset() const { return bfc_offset_; }
  NGBaseFragmentBuilder& SetBfcOffset(const NGBfcOffset&);

  NGBaseFragmentBuilder& SetEndMarginStrut(const NGMarginStrut&);

  NGBaseFragmentBuilder& SetExclusionSpace(
      std::unique_ptr<const NGExclusionSpace> exclusion_space);

  NGBaseFragmentBuilder& SwapUnpositionedFloats(
      Vector<RefPtr<NGUnpositionedFloat>>*);

  virtual NGBaseFragmentBuilder& AddChild(RefPtr<NGLayoutResult>,
                                          const NGLogicalOffset&);
  virtual NGBaseFragmentBuilder& AddChild(RefPtr<NGPhysicalFragment>,
                                          const NGLogicalOffset&);

  // TODO FIXME comment why we have this.
  virtual NGBaseFragmentBuilder& AddChild(std::nullptr_t,
                                          const NGLogicalOffset&);

  const Vector<RefPtr<NGPhysicalFragment>>& Children() const {
    return children_;
  }

  NGBaseFragmentBuilder& AddOutOfFlowDescendant(
      NGOutOfFlowPositionedDescendant);

  NGWritingMode WritingMode() const { return writing_mode_; }
  TextDirection Direction() const { return direction_; }

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

  LayoutUnit inline_size_;
  WTF::Optional<NGBfcOffset> bfc_offset_;
  NGMarginStrut end_margin_strut_;
  std::unique_ptr<const NGExclusionSpace> exclusion_space_;

  // Floats that need to be positioned by the next in-flow fragment that can
  // determine its block position in space.
  Vector<RefPtr<NGUnpositionedFloat>> unpositioned_floats_;

  Vector<NGOutOfFlowPositionedCandidate> oof_positioned_candidates_;
  Vector<NGOutOfFlowPositionedDescendant> oof_positioned_descendants_;

  Vector<RefPtr<NGPhysicalFragment>> children_;
  Vector<NGLogicalOffset> offsets_;

 private:
  RefPtr<const ComputedStyle> style_;
  NGWritingMode writing_mode_;
  TextDirection direction_;
};

}  // namespace blink

#endif  // NGBaseFragmentBuilder
