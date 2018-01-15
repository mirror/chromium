// Copyright (c) 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BaselineAlignment_h
#define BaselineAlignment_h

#include "core/layout/GridLayoutUtils.h"
#include "core/layout/LayoutBox.h"

namespace blink {

class GridTrackSizingAlgorithm;

// These classes are used to implement the Baseline Alignment logic, as
// described in the CSS Box Alignment specification.
// https://drafts.csswg.org/css-align/#baseline-terms

// A baseline-sharing group is composed of boxes that participate in
// baseline alignment together. This is possible only if they:
//
//   * Share an alignment context along an axis perpendicular to their
//   baseline alignment axis.
//   * Have compatible baseline alignment preferences (i.e., the baselines
//   that want to align are on the same side of the alignment context).
class BaselineGroup {
 public:
  void Update(const LayoutBox&, LayoutUnit ascent, LayoutUnit descent);
  LayoutUnit MaxAscent() const { return max_ascent_; }
  LayoutUnit MaxDescent() const { return max_descent_; }
  int size() const { return items_.size(); }

 private:
  friend class BaselineContext;
  BaselineGroup(WritingMode block_flow, ItemPosition child_preference);
  bool IsCompatible(WritingMode, ItemPosition) const;
  bool IsOppositeBlockFlow(WritingMode block_flow) const;
  bool IsOrthogonalBlockFlow(WritingMode block_flow) const;

  WritingMode block_flow_;
  ItemPosition preference_;
  LayoutUnit max_ascent_;
  LayoutUnit max_descent_;
  HashSet<const LayoutBox*> items_;
};

// Boxes share an alignment context along a particular axis when they
// are:
//
//  * table cells in the same row, along the table's row (inline) axis
//  * table cells in the same column, along the table's column (block)
//  axis
//  * grid items in the same row, along the grid's row (inline) axis
//  * grid items in the same column, along the grid's colum (block) axis
//  * flex items in the same flex line, along the flex container's main
//  axis
class BaselineContext {
 public:
  BaselineContext(const LayoutBox& child,
                  ItemPosition preference,
                  LayoutUnit ascent,
                  LayoutUnit descent);
  Vector<BaselineGroup>& SharedGroups() { return shared_groups_; }
  const BaselineGroup& GetSharedGroup(const LayoutBox& child,
                                      ItemPosition preference) const;
  void UpdateSharedGroup(const LayoutBox& child,
                         ItemPosition preference,
                         LayoutUnit ascent,
                         LayoutUnit descent);

 private:
  // TODO Properly implement baseline-group compatibility
  // See https://github.com/w3c/csswg-drafts/issues/721
  BaselineGroup& FindCompatibleSharedGroup(const LayoutBox& child,
                                           ItemPosition preference);

  Vector<BaselineGroup> shared_groups_;
};

static inline bool IsBaselinePosition(ItemPosition position) {
  return position == ItemPosition::kBaseline ||
         position == ItemPosition::kLastBaseline;
}

class BaselineAlignment {
 public:
  void UpdateBaselineAlignmentContextIfNeeded(ItemPosition,
                                              unsigned shared_ontext,
                                              LayoutBox&,
                                              GridAxis);
  LayoutUnit BaselineOffsetForChild(ItemPosition,
                                    unsigned shared_ontext,
                                    const LayoutBox&,
                                    GridAxis) const;
  Optional<LayoutUnit> ExtentForBaselineAlignment(ItemPosition,
                                                  unsigned shared_ontext,
                                                  const LayoutBox&,
                                                  GridAxis) const;
  bool BaselineMayAffectIntrinsicSize(const GridTrackSizingAlgorithm&,
                                      GridTrackSizingDirection) const;
  void SetBlockFlow(WritingMode block_flow) { block_flow_ = block_flow; };
  void Clear();

 private:
  const BaselineGroup& GetBaselineGroupForChild(ItemPosition,
                                                unsigned shared_ontext,
                                                const LayoutBox&,
                                                GridAxis) const;
  LayoutUnit MarginOverForChild(const LayoutBox&, GridAxis) const;
  LayoutUnit MarginUnderForChild(const LayoutBox&, GridAxis) const;
  LayoutUnit LogicalAscentForChild(const LayoutBox&, GridAxis) const;
  LayoutUnit AscentForChild(const LayoutBox&, GridAxis) const;
  LayoutUnit DescentForChild(const LayoutBox&, LayoutUnit, GridAxis) const;
  bool IsDescentBaselineForChild(const LayoutBox&, GridAxis) const;
  bool IsBaselineContextComputed(GridAxis) const;
  bool IsHorizontalBaselineAxis(GridAxis) const;
  bool IsOrthogonalChildForBaseline(const LayoutBox&) const;
  bool IsParallelToBaselineAxisForChild(const LayoutBox&, GridAxis) const;

  typedef HashMap<unsigned,
                  std::unique_ptr<BaselineContext>,
                  DefaultHash<unsigned>::Hash,
                  WTF::UnsignedWithZeroKeyHashTraits<unsigned>>
      BaselineContextsMap;

  WritingMode block_flow_;
  BaselineContextsMap row_axis_alignment_context_;
  BaselineContextsMap col_axis_alignment_context_;
};

}  // namespace blink

#endif  // BaselineContext_h
