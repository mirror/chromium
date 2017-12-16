// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NGLineBoxFragmentBuilder_h
#define NGLineBoxFragmentBuilder_h

#include "core/layout/ng/geometry/ng_logical_offset.h"
#include "core/layout/ng/inline/ng_inline_node.h"
#include "core/layout/ng/inline/ng_line_height_metrics.h"
#include "core/layout/ng/ng_container_fragment_builder.h"
#include "core/layout/ng/ng_positioned_float.h"
#include "platform/wtf/Allocator.h"

namespace blink {

class ComputedStyle;
class NGInlineBreakToken;
class NGInlineNode;
class NGPhysicalFragment;

class CORE_EXPORT NGLineBoxFragmentBuilder final
    : public NGContainerFragmentBuilder {
  STACK_ALLOCATED();
 public:
  NGLineBoxFragmentBuilder(NGInlineNode,
                           scoped_refptr<const ComputedStyle>,
                           WritingMode,
                           TextDirection);
  ~NGLineBoxFragmentBuilder() override;

  void Reset();

  NGLogicalSize Size() const final;
  LayoutUnit ComputeBlockSize() const;

  void SetMetrics(const NGLineHeightMetrics&);
  const NGLineHeightMetrics& Metrics() const { return metrics_; }

  void SwapPositionedFloats(Vector<NGPositionedFloat>*);

  // Set the break token for the fragment to build.
  // A finished break token will be attached if not set.
  void SetBreakToken(scoped_refptr<NGInlineBreakToken>);

  // A data struct to keep NGLayoutResult or fragment until the box tree
  // structures and child offsets are finalized.
  struct Child {
    DISALLOW_NEW_EXCEPT_PLACEMENT_NEW();

    scoped_refptr<NGLayoutResult> layout_result;
    scoped_refptr<NGPhysicalFragment> fragment;
    LayoutObject* out_of_flow_positioned_box = nullptr;
    LayoutObject* out_of_flow_containing_box = nullptr;
    NGLogicalOffset offset;
    LayoutUnit inline_size;
    unsigned box_data_index = 0;
    UBiDiLevel bidi_level;

    Child(NGLogicalOffset offset) : offset(offset), bidi_level(0xff) {}
    Child(UBiDiLevel bidi_level) : bidi_level(bidi_level) {}
    Child(scoped_refptr<NGLayoutResult> layout_result,
          NGLogicalOffset offset,
          LayoutUnit inline_size,
          UBiDiLevel bidi_level)
        : layout_result(std::move(layout_result)),
          offset(offset),
          inline_size(inline_size),
          bidi_level(bidi_level) {}
    Child(scoped_refptr<NGPhysicalFragment> fragment,
          LayoutUnit block_offset,
          LayoutUnit inline_size,
          UBiDiLevel bidi_level)
        : fragment(std::move(fragment)),
          offset({LayoutUnit(), block_offset}),
          inline_size(inline_size),
          bidi_level(bidi_level) {}
    Child(LayoutObject* out_of_flow_positioned_box,
          LayoutObject* out_of_flow_containing_box,
          UBiDiLevel bidi_level)
        : out_of_flow_positioned_box(out_of_flow_positioned_box),
          out_of_flow_containing_box(out_of_flow_containing_box),
          bidi_level(bidi_level) {}

    bool HasFragment() const { return layout_result || fragment; }
    bool HasBidiLevel() const { return bidi_level != 0xff; }
    const NGPhysicalFragment* PhysicalFragment() const;
  };

  // A vector of Child.
  // Unlike the fragment builder, chlidren are mutable.
  // Callers can add to the fragment builder in a batch once finalized.
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
    void clear() { children_.clear(); }

    using iterator = Vector<Child, 16>::iterator;
    iterator begin() { return children_.begin(); }
    iterator end() { return children_.end(); }
    using const_iterator = Vector<Child, 16>::const_iterator;
    const_iterator begin() const { return children_.begin(); }
    const_iterator end() const { return children_.end(); }

    void AddChild(Child&& child) { children_.push_back(std::move(child)); }
    void AddChild(scoped_refptr<NGLayoutResult>,
                  const NGLogicalOffset&,
                  LayoutUnit inline_size,
                  UBiDiLevel);
    void AddChild(scoped_refptr<NGPhysicalFragment>,
                  LayoutUnit block_offset,
                  LayoutUnit inline_size,
                  UBiDiLevel);
    void AddChild(LayoutObject* out_of_flow_positioned_box,
                  LayoutObject* out_of_flow_containing_box,
                  UBiDiLevel);
    // nullptr child is a placeholder until enclosing inline boxes are closed
    // and we know the final box structure and their positions. This variant
    // helps to avoid needing static_cast when adding a nullptr.
    void AddChild(const NGLogicalOffset&);
    void AddChild(UBiDiLevel);

    void MoveInBlockDirection(LayoutUnit);
    void MoveInBlockDirection(LayoutUnit, unsigned start, unsigned end);

   private:
    Vector<Child, 16> children_;
  };

  // Add all items in ChildList. Skips null Child if any.
  void AddChildren(ChildList&);

  // Creates the fragment. Can only be called once.
  scoped_refptr<NGLayoutResult> ToLineBoxFragment();

 private:
  NGInlineNode node_;

  NGLineHeightMetrics metrics_;
  Vector<NGPositionedFloat> positioned_floats_;

  scoped_refptr<NGInlineBreakToken> break_token_;
};

}  // namespace blink

#endif  // NGLineBoxFragmentBuilder
