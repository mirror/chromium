// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NGLineBoxFragmentBuilder_h
#define NGLineBoxFragmentBuilder_h

#include "core/layout/ng/geometry/ng_logical_offset.h"
#include "core/layout/ng/inline/ng_inline_break_token.h"
#include "core/layout/ng/inline/ng_line_height_metrics.h"
#include "core/layout/ng/ng_base_fragment_builder.h"
#include "platform/wtf/Allocator.h"

namespace blink {

class NGInlineNode;
class NGPhysicalFragment;
class NGPhysicalLineBoxFragment;

class CORE_EXPORT NGLineBoxFragmentBuilder final
    : public NGBaseFragmentBuilder {
  STACK_ALLOCATED();

 public:
  NGLineBoxFragmentBuilder(NGInlineNode,
                           scoped_refptr<const ComputedStyle>,
                           NGWritingMode);

  NGLineBoxFragmentBuilder& SetInlineSize(LayoutUnit);

  NGLineBoxFragmentBuilder& AddChild(scoped_refptr<NGPhysicalFragment>,
                                     const NGLogicalOffset&);
  NGLineBoxFragmentBuilder& AddChild(scoped_refptr<NGLayoutResult>,
                                     const NGLogicalOffset&);
  NGLineBoxFragmentBuilder& AddChild(std::nullptr_t, const NGLogicalOffset&);
  void MoveChildrenInBlockDirection(LayoutUnit);
  void MoveChildrenInBlockDirection(LayoutUnit, unsigned start, unsigned end);

  const Vector<scoped_refptr<NGPhysicalFragment>>& Children() const {
    return children_;
  }
  Vector<scoped_refptr<NGPhysicalFragment>>& MutableChildren() {
    return children_;
  }
  const Vector<NGLogicalOffset>& Offsets() const { return offsets_; }
  Vector<NGLogicalOffset>& MutableOffsets() { return offsets_; }

  void SetMetrics(const NGLineHeightMetrics&);
  const NGLineHeightMetrics& Metrics() const { return metrics_; }

  // Set the break token for the fragment to build.
  // A finished break token will be attached if not set.
  void SetBreakToken(scoped_refptr<NGInlineBreakToken>);

  // Creates the fragment. Can only be called once.
  scoped_refptr<NGPhysicalLineBoxFragment> ToLineBoxFragment();

 private:
  NGInlineNode node_;

  LayoutUnit inline_size_;

  Vector<scoped_refptr<NGPhysicalFragment>> children_;
  Vector<NGLogicalOffset> offsets_;

  NGLineHeightMetrics metrics_;

  scoped_refptr<NGInlineBreakToken> break_token_;
};

}  // namespace blink

#endif  // NGLineBoxFragmentBuilder
