// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NGLineBoxFragmentBuilder_h
#define NGLineBoxFragmentBuilder_h

#include "core/layout/ng/geometry/ng_logical_offset.h"
#include "core/layout/ng/inline/ng_inline_break_token.h"
#include "core/layout/ng/inline/ng_line_height_metrics.h"
#include "core/layout/ng/ng_container_fragment_builder.h"
#include "core/layout/ng/ng_positioned_float.h"
#include "platform/wtf/Allocator.h"

namespace blink {

class ComputedStyle;
class NGInlineNode;
class NGPhysicalFragment;

class CORE_EXPORT NGLineBoxFragmentBuilder final
    : public NGContainerFragmentBuilder {
  STACK_ALLOCATED();

 public:
  NGLineBoxFragmentBuilder(NGInlineNode,
                           RefPtr<const ComputedStyle>,
                           NGWritingMode);

  NGLogicalSize Size() const final;

  void SetMetrics(const NGLineHeightMetrics&);
  const NGLineHeightMetrics& Metrics() const { return metrics_; }

  void AddPositionedFloat(const NGPositionedFloat&);

  // Set the break token for the fragment to build.
  // A finished break token will be attached if not set.
  void SetBreakToken(RefPtr<NGInlineBreakToken>);

  // A data struct to keep NGLayoutResult until the box tree structures and
  // child offsets are finalized.
  // Callers can add to the fragment builder in a batch once finalized.
  struct Child {
    RefPtr<NGLayoutResult> layout_result;
    RefPtr<NGPhysicalFragment> fragment;
    NGLogicalOffset offset;

    const NGPhysicalFragment* PhysicalFragment() const;
  };

  class ChildList {
    STACK_ALLOCATED();

   public:
    ChildList() {}
    void operator=(ChildList&& other) {
      children_ = std::move(other.children_);
    }

    Child& operator[](unsigned i) { return children_[i]; }

    unsigned size() const { return children_.size(); }
    bool IsEmpty() const { return children_.IsEmpty(); }
    void ReserveInitialCapacity(unsigned capacity) {
      children_.ReserveInitialCapacity(capacity);
    }

    using iterator = Vector<Child, 16>::iterator;
    using const_iterator = Vector<Child, 16>::const_iterator;
    iterator begin() { return children_.begin(); }
    iterator end() { return children_.end(); }
    const_iterator begin() const { return children_.begin(); }
    const_iterator end() const { return children_.end(); }

    void AddChild(RefPtr<NGLayoutResult>, const NGLogicalOffset&);
    void AddChild(RefPtr<NGPhysicalFragment>, const NGLogicalOffset&);
    // nullptr child is a placeholder until enclosing inline boxes are closed
    // and we know the final box structure and their positions. This variant
    // helps to avoid needing static_cast when adding a nullptr.
    void AddChild(std::nullptr_t, const NGLogicalOffset&);

    void MoveInBlockDirection(LayoutUnit);
    void MoveInBlockDirection(LayoutUnit, unsigned start, unsigned end);

   private:
    Vector<Child, 16> children_;
  };

  void AddChildren(const ChildList&);

  // Creates the fragment. Can only be called once.
  RefPtr<NGLayoutResult> ToLineBoxFragment();

 private:
  NGInlineNode node_;

  NGLineHeightMetrics metrics_;
  Vector<NGPositionedFloat> positioned_floats_;

  RefPtr<NGInlineBreakToken> break_token_;
};

}  // namespace blink

#endif  // NGLineBoxFragmentBuilder
