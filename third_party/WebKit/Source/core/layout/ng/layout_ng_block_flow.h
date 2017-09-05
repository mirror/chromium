// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LayoutNGBlockFlow_h
#define LayoutNGBlockFlow_h

#include "core/CoreExport.h"
#include "core/layout/LayoutBlockFlow.h"
#include "core/layout/LayoutTableCell.h"
#include "core/layout/ng/ng_physical_box_fragment.h"

namespace blink {

class NGBreakToken;
class NGConstraintSpace;
struct NGInlineNodeData;
class NGLayoutResult;

class CORE_EXPORT LayoutNGData {
 public:
  NGInlineNodeData* GetNGInlineNodeData() const;
  void ResetNGInlineNodeData();
  bool HasNGInlineNodeData() const { return ng_inline_node_data_.get(); }

  // Returns the last layout result for this block flow with the given
  // constraint space and break token, or null if it is not up-to-date or
  // otherwise unavailable.
  RefPtr<NGLayoutResult> CachedLayoutResult(LayoutBox* box,
                                            const NGConstraintSpace&,
                                            NGBreakToken*) const;

  void SetCachedLayoutResult(const NGConstraintSpace&,
                             NGBreakToken*,
                             RefPtr<NGLayoutResult>);

 private:
  std::unique_ptr<NGInlineNodeData> ng_inline_node_data_;

  RefPtr<NGLayoutResult> cached_result_;
  RefPtr<const NGConstraintSpace> cached_constraint_space_;
};

// This overrides the default layout block algorithm to use Layout NG.
class CORE_EXPORT LayoutNGBlockFlow final : public LayoutBlockFlow {
 public:
  explicit LayoutNGBlockFlow(Element*);
  ~LayoutNGBlockFlow() override;

  void UpdateBlockLayout(bool relayout_children) override;

  const char* GetName() const override { return "LayoutNGBlockFlow"; }

  LayoutUnit FirstLineBoxBaseline() const override;
  LayoutUnit InlineBlockBaseline(LineDirectionMode) const override;

  void PaintObject(const PaintInfo&, const LayoutPoint&) const override;

  LayoutNGData* GetNGData() { return &layout_ng_data_; }

  RefPtr<const NGPhysicalBoxFragment> RootFragment() const {
    return physical_root_fragment_;
  }

 private:
  bool IsOfType(LayoutObjectType) const override;

  void UpdateMargins(const NGConstraintSpace&);

  LayoutNGData layout_ng_data_;
  RefPtr<const NGPhysicalBoxFragment> physical_root_fragment_;
};

DEFINE_LAYOUT_OBJECT_TYPE_CASTS(LayoutNGBlockFlow, IsLayoutNGBlockFlow());

class CORE_EXPORT LayoutNGTableCell final : public LayoutTableCell {
 public:
  explicit LayoutNGTableCell(Element*);
  ~LayoutNGTableCell() override;

  void UpdateBlockLayout(bool relayout_children) override;

  const char* GetName() const override { return "LayoutNGTableCell"; }

  LayoutUnit FirstLineBoxBaseline() const override;
  LayoutUnit InlineBlockBaseline(LineDirectionMode) const override;

  void PaintObject(const PaintInfo&, const LayoutPoint&) const override;

  LayoutNGData* GetNGData() { return &layout_ng_data_; }

  RefPtr<const NGPhysicalBoxFragment> RootFragment() const {
    return physical_root_fragment_;
  }

 private:
  bool IsOfType(LayoutObjectType) const override;

  void UpdateMargins(const NGConstraintSpace&);

  LayoutNGData layout_ng_data_;
  RefPtr<const NGPhysicalBoxFragment> physical_root_fragment_;
};

DEFINE_LAYOUT_OBJECT_TYPE_CASTS(LayoutNGTableCell, IsLayoutNGTableCell());

inline RefPtr<const NGPhysicalBoxFragment> GetRootFragment(
    const LayoutBlockFlow& block) {
  DCHECK(block.IsLayoutNGTableCell() || block.IsLayoutNGBlockFlow());

  if (block.IsLayoutNGBlockFlow())
    return ToLayoutNGBlockFlow(&block)->RootFragment();
  else
    return ToLayoutNGTableCell(&block)->RootFragment();
}

inline LayoutNGData* GetNGData(LayoutBox* box) {
  return box->IsLayoutNGBlockFlow() ? ToLayoutNGBlockFlow(box)->GetNGData()
                                    : ToLayoutNGTableCell(box)->GetNGData();
}

}  // namespace blink

#endif  // LayoutNGBlockFlow_h
