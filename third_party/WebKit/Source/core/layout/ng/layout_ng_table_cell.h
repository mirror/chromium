// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LayoutNGTableCell_h
#define LayoutNGTableCell_h

#include "core/CoreExport.h"
#include "core/layout/LayoutTableCell.h"
#include "core/layout/ng/layout_ng_data.h"
#include "core/layout/ng/ng_physical_box_fragment.h"

namespace blink {

class NGConstraintSpace;

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

}  // namespace blink

#endif  // LayoutNGTableCell_h
