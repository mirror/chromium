// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LayoutNGMixin_h
#define LayoutNGMixin_h

#include "core/layout/LayoutBoxModelObject.h"
#include "core/layout/ng/inline/ng_inline_node_data.h"
#include "core/layout/ng/ng_constraint_space.h"
#include "core/layout/ng/ng_physical_box_fragment.h"
#include "core/paint/ng/ng_paint_fragment.h"

namespace blink {

class NGBreakToken;
class NGLayoutResult;

// This mixin holds code shared between LayoutNG subclasses of
// LayoutBlockFlow.

template <typename Base>
class LayoutNGMixin : public Base {
 public:
  explicit LayoutNGMixin(Element* element) : Base(element) {}
  ~LayoutNGMixin() override;

  NGInlineNodeData* GetNGInlineNodeData() const override;
  void ResetNGInlineNodeData() override;
  bool HasNGInlineNodeData() const override {
    return ng_inline_node_data_.get();
  }

  LayoutUnit FirstLineBoxBaseline() const override;
  LayoutUnit InlineBlockBaseline(LineDirectionMode) const override;

  void PaintObject(const PaintInfo&, const LayoutPoint&) const override;

  bool NodeAtPoint(HitTestResult&,
                   const HitTestLocation& location_in_container,
                   const LayoutPoint& accumulated_offset,
                   HitTestAction) override;

  // Returns the last layout result for this block flow with the given
  // constraint space and break token, or null if it is not up-to-date or
  // otherwise unavailable.
  RefPtr<NGLayoutResult> CachedLayoutResult(const NGConstraintSpace&,
                                            NGBreakToken*) const override;

  void SetCachedLayoutResult(const NGConstraintSpace&,
                             NGBreakToken*,
                             RefPtr<NGLayoutResult>) override;

  const NGPaintFragment* PaintFragment() const override {
    return paint_fragment_.get();
  }
  void SetPaintFragment(RefPtr<const NGPhysicalFragment>) override;

  static bool LocalVisualRectFor(const LayoutObject*, NGPhysicalOffsetRect*);

 protected:
  bool IsOfType(LayoutObject::LayoutObjectType) const override;

  void AddOverflowFromChildren() override;

  const NGPhysicalBoxFragment* CurrentFragment() const override;

  const NGBaseline* FragmentBaseline(NGBaselineAlgorithmType) const;

  void UpdateMargins(const NGConstraintSpace&);

  std::unique_ptr<NGInlineNodeData> ng_inline_node_data_;

  RefPtr<NGLayoutResult> cached_result_;
  RefPtr<const NGConstraintSpace> cached_constraint_space_;
  std::unique_ptr<const NGPaintFragment> paint_fragment_;

  friend class NGBaseLayoutAlgorithmTest;
};

}  // namespace blink

#endif  // LayoutNGMixin_h
