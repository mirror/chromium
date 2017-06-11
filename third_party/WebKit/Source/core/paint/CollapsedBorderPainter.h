// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CollapsedBorderPainter_h
#define CollapsedBorderPainter_h

#include "core/layout/LayoutTable.h"
#include "core/layout/LayoutTableCell.h"
#include "platform/wtf/Allocator.h"

namespace blink {

struct PaintInfo;
class LayoutPoint;

class CollapsedBorderPainter {
  STACK_ALLOCATED();

 public:
  CollapsedBorderPainter(const LayoutTableCell& cell)
      : cell_(cell), table_(*cell.Table()) {}

  void PaintCollapsedBorders(const PaintInfo&, const LayoutPoint&);

 private:
  const DisplayItemClient& DisplayItemClientForBorders() const;

  void SetupBorders();
  void AdjustCorners();
  void AdjustForWritingModeAndDirection();

  const LayoutTableCell& cell_;
  const LayoutTable& table_;

  struct Border {
    // This will be set to null if we don't need to paint this border.
    const CollapsedBorderValue* value;
    int inner_width;
    int outer_width;
    // We may paint the border not in full length if the corner is covered by
    // another higher priority border. The following are the outsets from the
    // border box of the begin and end points of the painted segment.
    int begin_outset;
    int end_outset;
  };
  Border start_;
  Border end_;
  Border before_;
  Border after_;

  const LayoutTableCell* cell_preceding_;
  const LayoutTableCell* cell_following_;
  const LayoutTableCell* cell_above_;
  const LayoutTableCell* cell_below_;
};

}  // namespace blink

#endif  // CollapsedBorderPainter_h
