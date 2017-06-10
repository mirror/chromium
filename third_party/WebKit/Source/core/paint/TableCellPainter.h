// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef TableCellPainter_h
#define TableCellPainter_h

#include "core/layout/LayoutTable.h"
#include "core/layout/LayoutTableCell.h"
#include "platform/graphics/paint/DisplayItem.h"
#include "platform/wtf/Allocator.h"

namespace blink {

struct PaintInfo;
class LayoutObject;
class LayoutPoint;
class LayoutRect;

class TableCellPainter {
  STACK_ALLOCATED();

 public:
  TableCellPainter(const LayoutTableCell& cell)
      : cell_(cell), table_(*cell.Table()) {}

  void Paint(const PaintInfo&, const LayoutPoint&);

  void PaintCollapsedBorders(const PaintInfo&, const LayoutPoint&);
  void PaintContainerBackgroundBehindCell(
      const PaintInfo&,
      const LayoutPoint&,
      const LayoutObject& background_object);
  void PaintBoxDecorationBackground(const PaintInfo&,
                                    const LayoutPoint& paint_offset);
  void PaintMask(const PaintInfo&, const LayoutPoint& paint_offset);

 private:
  const DisplayItemClient& DisplayItemClientForBorders() const;
  LayoutRect PaintRectNotIncludingVisualOverflow(
      const LayoutPoint& paint_offset);
  void PaintBackground(const PaintInfo&,
                       const LayoutRect&,
                       const LayoutObject& background_object);

  struct CollapsedBorderPaintInfo;
  void AdjustBorderSegments(CollapsedBorderPaintInfo&,
                            CollapsedBorderPaintInfo&,
                            CollapsedBorderPaintInfo&,
                            CollapsedBorderPaintInfo&);
  void AdjustEndBorderWithCellFollowing(CollapsedBorderPaintInfo&);
  void AdjustAfterBorderWithCellBelow(CollapsedBorderPaintInfo&);
  void AdjustForWritingModeAndDirection(CollapsedBorderPaintInfo&,
                                        CollapsedBorderPaintInfo&,
                                        CollapsedBorderPaintInfo&,
                                        CollapsedBorderPaintInfo&);

  const LayoutTableCell* CellPreceding() { return table_.CellPreceding(cell_); }
  const LayoutTableCell* CellFollowing() { return table_.CellFollowing(cell_); }
  const LayoutTableCell* CellAbove() { return table_.CellAbove(cell_); }
  const LayoutTableCell* CellBelow() { return table_.CellBelow(cell_); }

  const LayoutTableCell& cell_;
  const LayoutTable& table_;
};

}  // namespace blink

#endif  // TableCellPainter_h
