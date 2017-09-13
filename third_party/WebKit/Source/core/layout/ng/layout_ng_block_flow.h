// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LayoutNGBlockFlow_h
#define LayoutNGBlockFlow_h

#include "core/CoreExport.h"
#include "core/layout/LayoutBlockFlow.h"
#include "core/layout/ng/layout_ng_data.h"
#include "core/layout/ng/layout_ng_table_cell.h"
#include "core/layout/ng/ng_physical_box_fragment.h"

namespace blink {

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
