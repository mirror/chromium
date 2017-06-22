// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/paint/TableCellPaintInvalidator.h"

#include "core/layout/LayoutTable.h"
#include "core/layout/LayoutTableCell.h"
#include "core/layout/LayoutTableCol.h"
#include "core/layout/LayoutTableRow.h"
#include "core/layout/LayoutTableSection.h"
#include "core/paint/BlockPaintInvalidator.h"
#include "core/paint/ObjectPaintInvalidator.h"
#include "core/paint/PaintInvalidator.h"
#include "core/paint/PaintLayer.h"

namespace blink {

void TableCellPaintInvalidator::InvalidateContainerForCellGeometryChange(
    const LayoutObject& container,
    const PaintInvalidatorContext* container_context) {
  DCHECK(container.GetPaintInvalidationReason() ==
         PaintInvalidationReason::kNone);

  ObjectPaintInvalidator invalidator(container);
  if (!container_context) {
    DCHECK(!RuntimeEnabledFeatures::SlimmingPaintInvalidationEnabled());
    ObjectPaintInvalidator(container).InvalidatePaintRectangle(
        container.LocalVisualRect(), nullptr);
    return;
  }

  container_context->painting_layer->SetNeedsRepaint();
  container.InvalidateDisplayItemClients(PaintInvalidationReason::kGeometry);
  if (context_.paint_invalidation_container !=
      container_context->paint_invalidation_container) {
    ObjectPaintInvalidatorWithContext(container, *container_context)
        .InvalidatePaintRectangleWithContext(
            container.VisualRect(), PaintInvalidationReason::kGeometry);
  }
}

PaintInvalidationReason TableCellPaintInvalidator::InvalidatePaint() {
  // The cell's containing row and section paint backgrounds behind the cell,
  // and the row or table paints collapsed borders. If the cell's geometry
  // changed, invalidate the display items painting backgrounds and/or collapsed
  // borders.
  if (context_.old_location != context_.new_location ||
      cell_.Size() != cell_.PreviousSize()) {
    const auto& row = *cell_.Row();
    const auto& section = *row.Section();
    const auto& table = *section.Table();
    if (row.GetPaintInvalidationReason() == PaintInvalidationReason::kNone &&
        (row.StyleRef().HasBackground() ||
         (table.HasCollapsedBorders() &&
          !table.ShouldPaintCollapsedBordersInWhole()))) {
      InvalidateContainerForCellGeometryChange(
          row, RuntimeEnabledFeatures::SlimmingPaintInvalidationEnabled()
                   ? context_.parent_context
                   : nullptr);
    }

    if (table.GetPaintInvalidationReason() == PaintInvalidationReason::kNone &&
        table.HasCollapsedBorders() &&
        table.ShouldPaintCollapsedBordersInWhole()) {
      InvalidateContainerForCellGeometryChange(
          table, RuntimeEnabledFeatures::SlimmingPaintInvalidationEnabled()
                     ? context_.parent_context->parent_context->parent_context
                     : nullptr);
    }

    if (section.GetPaintInvalidationReason() ==
        PaintInvalidationReason::kNone) {
      bool section_paints_background = section.StyleRef().HasBackground();
      if (!section_paints_background) {
        auto col_and_colgroup = section.Table()->ColElementAtAbsoluteColumn(
            cell_.AbsoluteColumnIndex());
        if ((col_and_colgroup.col &&
             col_and_colgroup.col->StyleRef().HasBackground()) ||
            (col_and_colgroup.colgroup &&
             col_and_colgroup.colgroup->StyleRef().HasBackground()))
          section_paints_background = true;
      }
      if (section_paints_background) {
        InvalidateContainerForCellGeometryChange(
            section, RuntimeEnabledFeatures::SlimmingPaintInvalidationEnabled()
                         ? context_.parent_context->parent_context
                         : nullptr);
      }
    }
  }

  return BlockPaintInvalidator(cell_).InvalidatePaint(context_);
}

}  // namespace blink
