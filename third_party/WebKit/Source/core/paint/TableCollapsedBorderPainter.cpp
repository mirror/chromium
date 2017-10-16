// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#include "core/paint/TableCollapsedBorderPainter.h"
#include <algorithm>
#include "core/layout/LayoutTableCell.h"
#include "core/layout/LayoutTableCol.h"
#include "core/paint/BlockPainter.h"
#include "core/paint/BoxPainter.h"
#include "core/paint/LayoutObjectDrawingRecorder.h"
#include "core/paint/ObjectPainter.h"
#include "core/paint/PaintInfo.h"
#include "platform/graphics/GraphicsContextStateSaver.h"

namespace blink {
// Comparator: returns
// -1 => e1 <  e2
//  0 => e1 == e2
//  1 => e1 >  e2
static int CompareEdges(const TableCollapsedBorderPainter::EdgeRecord& e1,
                        const TableCollapsedBorderPainter::EdgeRecord& e2) {
  // if (!e1.border_) {
  //     if (!e2.border_)
  //         return 0;
  //     else
  //         return -1;
  // }
  // if (!e2.border_)
  //     return 1;
  EBorderStyle e1_style = e1.border_.Style();
  EBorderStyle e2_style = e2.border_.Style();
  // RULES: https://www.w3.org/TR/CSS2/tables.html#border-conflict-resolution
  // 1) Hidden edges win
  // 2) None edges lose
  // 3) Wider border-width win
  // 4) Louder border-style win
  // 5) Higher precedence wins
  // Rule #1
  if (e1_style == EBorderStyle::kHidden) {
    if (e2_style == EBorderStyle::kHidden)
      return 0;
    // else
    return 1;
  }
  if (e2_style == EBorderStyle::kHidden)
    return -1;
  // Rule #2
  if (e1_style == EBorderStyle::kNone) {
    if (e2_style == EBorderStyle::kNone)
      return 0;
    // else
    return -1;
  }
  if (e2_style == EBorderStyle::kNone)
    return 1;
  // Rule #3
  if (e1.border_.Width() < e2.border_.Width())
    return -1;
  // else
  if (e2.border_.Width() < e1.border_.Width())
    return 1;
  // Rule #4
  if (e1_style > e2_style)
    return 1;
  // else
  if (e2_style > e1_style)
    return -1;
  // Rule #5
  if (e1.precedence_ != e2.precedence_)
    return e1.precedence_ < e2.precedence_ ? -1 : 1;
  return 0;
}

// Like CompareEdges, except that HIDDEN edges lose
static int CompareIntersectionEdges(
    const TableCollapsedBorderPainter::EdgeRecord& e1,
    const TableCollapsedBorderPainter::EdgeRecord& e2) {
  EBorderStyle e1_style = e1.border_.Style();
  EBorderStyle e2_style = e2.border_.Style();
  // Rule #1
  if (e1_style == EBorderStyle::kHidden) {
    if (e2_style == EBorderStyle::kHidden)
      return 0;
    // else
    return -1;
  }
  if (e2_style == EBorderStyle::kHidden)
    return 1;
  return CompareEdges(e1, e2);
}

static std::string DirectionToStr(
    TableCollapsedBorderPainter::EdgeDirection dir) {
  switch (dir) {
    case TableCollapsedBorderPainter::North:
      return "North";
    case TableCollapsedBorderPainter::West:
      return "West";
    case TableCollapsedBorderPainter::East:
      return "East";
    case TableCollapsedBorderPainter::South:
      return "South";
    case TableCollapsedBorderPainter::None:
      return "None";
  }
}

static std::string PrecedenceToStr(EBorderPrecedence p) {
  switch (p) {
    case kBorderPrecedenceOff:
      return "OFF";
    case kBorderPrecedenceTable:
      return "TABLE";
    case kBorderPrecedenceColumnGroup:
      return "COLGROUP";
    case kBorderPrecedenceColumn:
      return "COL";
    case kBorderPrecedenceRowGroup:
      return "ROWGROUP";
    case kBorderPrecedenceRow:
      return "ROW";
    case kBorderPrecedenceCell:
      return "CELL";
  }
}

void TableCollapsedBorderPainter::ShowEdge(unsigned row,
                                           unsigned col,
                                           EdgeDirection direction,
                                           EBorderPrecedence precedence,
                                           const BorderValue& border,
                                           Color resolved_color) const {
  Color c = border.GetColor().Resolve(resolved_color);
  LOG(INFO) << "[" << row << " " << col << " " << DirectionToStr(direction)
            << "] " << PrecedenceToStr(precedence) << " " << border.Width()
            << " " << c.SerializedAsCSSComponentValue().Ascii().data();
}

void TableCollapsedBorderPainter::ShowEdges(bool show_hidden) const {
  LOG(INFO) << "edges";
  for (auto r = start_visible_row_; r <= end_visible_row_; r++) {
    LOG(INFO) << "row:" << r;
    for (auto c = start_visible_column_; c < end_visible_column_; c++) {
      auto edgeIdx = EdgeToIndex(r, c, East);
      if (edgeIdx != npos) {
        EdgeRecord edge = edges_[edgeIdx];
        if (show_hidden || edge.precedence_ != kBorderPrecedenceOff) {
          ShowEdge(r, c, East, edge.precedence_, edge.border_,
                   edge.resolved_color_);
        }
      }
      edgeIdx = EdgeToIndex(r, c, South);
      if (edgeIdx != npos) {
        EdgeRecord edge = edges_[edgeIdx];
        if (show_hidden || edge.precedence_ != kBorderPrecedenceOff) {
          ShowEdge(r, c, South, edge.precedence_, edge.border_,
                   edge.resolved_color_);
        }
      }
    }
  }
}

void TableCollapsedBorderPainter::ShowVisibleEdges(
    const VisibleEdgeContainer& edges) const {
  LOG(INFO) << "visedges";
  for (auto edge = edges.begin(); edge < edges.end(); edge++) {
    ShowEdge((*edge).m_row, (*edge).m_column, (*edge).direction_,
             (*edge).precedence_, (*edge).border_, (*edge).resolved_color_);
  }
}

void TableCollapsedBorderPainter::ShowIntersection(unsigned row,
                                                   unsigned col) const {
  auto i = IntersectionToIndex(row, col);
  if (i != npos) {
    const Intersection in = intersections_[i];
    if (in.direction_ != None) {
      LOG(INFO) << "[" << row << ", " << col << "] " << in.width_.ToInt() << " "
                << in.height_.ToInt()
                << " Win:" << DirectionToStr(in.direction_);
    }
  }
}

void TableCollapsedBorderPainter::ShowIntersections() const {
  LOG(INFO) << "intersections";
  for (auto r = start_visible_row_; r <= end_visible_row_; r++) {
    for (auto c = start_visible_column_; c < end_visible_column_; c++) {
      ShowIntersection(r, c);
    }
  }
}

void TableCollapsedBorderPainter::PaintBorders(
    const PaintInfo& paint_info,
    const LayoutPoint& paint_offset,
    const CellSpan& dirty_rows,
    const CellSpan& dirty_columns,
    const TableCollapsedBorderPainter& previous_painter) {
  InitEdges(dirty_rows, dirty_columns, previous_painter);
  // draw all the edges
  // initial algorithm: just draw everything
  // later algorithms: draw continuous lines if possible
  InitIntersections();
  PaintEdges(paint_info, paint_offset, dirty_rows, dirty_columns);
}

void TableCollapsedBorderPainter::InitEdges(
    const CellSpan& dirty_rows,
    const CellSpan& dirty_columns,
    const TableCollapsedBorderPainter& previous_painter) {
  // Initialize sizes
  // number of rows/cols in the section
  num_rows_ = layout_table_section_->NumRows();
  num_effective_columns_ =
      layout_table_section_->Table()->NumEffectiveColumns();
  // Coordinates of intersections whose edges we'd like to measure.
  // Measurement area is dirty_rows x dirtyCols + single-cell wide border zone
  // above/left of dirty area.
  //
  // Border zone is needed to measure intersections inside dirty area correctly.
  //
  // Number of intersections inside n-rows is n+1, inside m-cols is m+1
  // start and end. End points to one beyond, equivalent to iterator's .End()
  start_visible_row_ = dirty_rows.Start() == 0 ? 0 : dirty_rows.Start() - 1;
  start_visible_column_ =
      dirty_columns.Start() == 0 ? 0 : dirty_columns.Start() - 1;
  end_visible_row_ = dirty_rows.End() + 1;
  end_visible_column_ = dirty_columns.End() + 1;
  unsigned edgeRowCount = end_visible_row_ - start_visible_row_;
  unsigned edgeColumnCount = end_visible_column_ - start_visible_column_;
  edges_.resize(2 * edgeRowCount * edgeColumnCount);
  PopulateEdges(dirty_rows, dirty_columns, previous_painter);
  // ShowEdges();
}

void TableCollapsedBorderPainter::InitIntersections() {
  max_intersection_width_ = 0;
  max_intersection_height_ = 0;
  intersections_.resize((end_visible_row_ - start_visible_row_) *
                        (end_visible_column_ - start_visible_column_));
  for (unsigned r = start_visible_row_; r < end_visible_row_; r++) {
    for (unsigned c = start_visible_column_; c < end_visible_column_; c++) {
      InitIntersection(r, c);
    }
  }
  // ShowIntersections();
}

void TableCollapsedBorderPainter::InitIntersection(unsigned row, unsigned col) {
  EdgeRecord edges[None];
  for (int d = North; d < None; d++) {
    unsigned index = EdgeToIndex(row, col, (EdgeDirection)d);
    if (index != npos)
      edges[d] = edges_[index];
  }
  EdgeDirection winner = North;
  for (int d = West; d < None; d++) {
    if (CompareIntersectionEdges(edges[winner], edges[d]) == -1)
      winner = (EdgeDirection)d;
  }
  unsigned index = IntersectionToIndex(row, col);
  // only set if our winner is really an edge
  if (edges[winner].precedence_ != kBorderPrecedenceOff) {
    unsigned width = 0;
    unsigned height = 0;
    if (winner == North || winner == South) {
      width = edges[winner].border_.Width();
      if (CompareIntersectionEdges(edges[East], edges[West]) < 1)  // West won
        height = edges[West].border_.Width();
      else
        height = edges[East].border_.Width();
    } else {  // winner == East || West
      height = edges[winner].border_.Width();
      if (CompareIntersectionEdges(edges[North], edges[South]) <
          1)  // South won
        width = edges[South].border_.Width();
      else
        width = edges[North].border_.Width();
    }
    max_intersection_height_ = std::max(max_intersection_height_, height);
    max_intersection_width_ = std::max(max_intersection_width_, width);
    intersections_[index] =
        Intersection(LayoutUnit(width), LayoutUnit(height), winner);
    // LOG(INFO) << "In: " << row << " " << col << ": " << width << "px " <<
    // height << "px " << DirectionToStr(winner);
  }
}

void TableCollapsedBorderPainter::PopulateEdges(
    const CellSpan& dirty_rows,
    const CellSpan& dirty_columns,
    const TableCollapsedBorderPainter& previous_painter) {
  VisibleEdgeContainer edges;
  // Iterate everything in reverse priority order
  // populate adjacent edges from adjacent section
  GetVisibleEdgesSiblingSection(edges, previous_painter);
  MergeVisibleEdges(edges);
  LayoutTable* table = layout_table_section_->Table();
  // populate <table>
  GetVisibleEdgesTable(edges, layout_table_section_->Table());
  MergeVisibleEdges(edges);
  // populate <colgroup>
  TextDirection direction = table->Style()->Direction();
  Vector<const LayoutTableCol*> colgroups;
  for (LayoutTableCol* colgroup = table->FirstColumn(); colgroup;
       colgroup = colgroup->NextColumn())
    colgroups.push_back(colgroup);
  for (auto col_group = colgroups.rbegin(); col_group != colgroups.rend();
       col_group++) {
    if ((*col_group)->IsTableColumnGroup()) {
      GetVisibleEdgesColgroup(edges, *col_group);
      MergeVisibleEdges(edges);
    }
  }
  // populate <col>
  for (auto col = colgroups.rbegin(); col != colgroups.rend(); col++) {
    if ((*col)->IsTableColumn()) {
      GetVisibleEdgesCol(edges, *col);
      MergeVisibleEdges(edges);
    }
  }
  // populate <tbody>
  GetVisibleEdgesSection(edges);
  MergeVisibleEdges(edges);
  // populate <tr>
  for (int r = dirty_rows.End() - 1; r >= (int)dirty_rows.Start(); r--) {
    const LayoutTableRow* row = layout_table_section_->RowLayoutObjectAt(r);
    if (row) {
      GetVisibleEdgesRow(edges, row);
      MergeVisibleEdges(edges);
    }
  }
  // populate <td>, iterate in reverse
  for (int r = dirty_rows.End() - 1; r >= (int)dirty_rows.Start(); r--) {
    for (int c = dirty_columns.End() - 1; c >= (int)dirty_columns.Start();
         c--) {
      // HashSet for primary cell to eliminate duplicates
      const LayoutTableCell* cell = layout_table_section_->PrimaryCellAt(r, c);
      if (cell) {
        GetVisibleEdgesCell(edges, cell, direction);
        MergeVisibleEdges(edges);
      }
    }
  }
  // ShowEdges();
  // ShowVisibleEdges(edges);
}

// Gets edges of the neighboring section
void TableCollapsedBorderPainter::GetVisibleEdgesSiblingSection(
    VisibleEdgeContainer& edges,
    const TableCollapsedBorderPainter& previous_painter) {
  edges.clear();
  bool is_above =
      previous_painter.layout_table_section_ ==
      layout_table_section_->Table()->SectionAbove(layout_table_section_);
  bool is_below =
      previous_painter.layout_table_section_ ==
      layout_table_section_->Table()->SectionBelow(layout_table_section_);
  if (!is_above && !is_below) {
    // Happens if we are the first section
    // LOG(INFO) << "not sure where sibling section is";
    return;
  }
  unsigned siblingRow = is_above ? previous_painter.num_rows_ : 0;
  unsigned my_row = is_above ? 0 : num_rows_;
  for (unsigned c = previous_painter.start_visible_column_;
       c <= previous_painter.end_visible_column_; c++) {
    auto sibling_index = previous_painter.EdgeToIndex(siblingRow, c, East);
    auto my_index = EdgeToIndex(my_row, c, East);
    if (sibling_index != npos && my_index != npos) {
      EdgeRecord sibling_edge = previous_painter.edges_[sibling_index];
      edges.push_back(VisibleEdgeRecord(my_row, c, East, sibling_edge.border_,
                                        sibling_edge.precedence_,
                                        sibling_edge.resolved_color_));
    }
  }
}

void TableCollapsedBorderPainter::GetVisibleEdgesTable(
    VisibleEdgeContainer& edges,
    const LayoutTable* table) {
  edges.clear();
  Color resolved_color = table->ResolveColor(CSSPropertyColor);
  const ComputedStyle* border_style = table->Style();
  const BorderValue top_border_value = border_style->BorderTop();
  const BorderValue* top_border = &top_border_value;
  const BorderValue right_border_value = border_style->BorderRight();
  const BorderValue* right_border = &right_border_value;
  const BorderValue bottom_border_value = border_style->BorderBottom();
  const BorderValue* bottom_border = &bottom_border_value;
  const BorderValue left_border_value = border_style->BorderLeft();
  const BorderValue* left_border = &left_border_value;
  RotateBorders(TextDirection::kLtr, table->Style()->GetWritingMode(),
                &top_border, &right_border, &bottom_border, &left_border);
  // top row, only if we abut table top
  if (this->layout_table_section_ == table->TopNonEmptySection()) {
    FillHorizontalEdges(edges, 0, 0, num_effective_columns_, *top_border,
                        kBorderPrecedenceTable, resolved_color);
  }
  // bottom row, only if we abut table bottom
  if (this->layout_table_section_ == table->BottomNonEmptySection()) {
    FillHorizontalEdges(edges, num_rows_, 0, num_effective_columns_,
                        *bottom_border, kBorderPrecedenceTable, resolved_color);
  }
  FillVerticalEdges(edges, 0, 0, num_rows_, *left_border,
                    kBorderPrecedenceTable, resolved_color);
  FillVerticalEdges(edges, num_effective_columns_, 0, num_rows_, *right_border,
                    kBorderPrecedenceTable, resolved_color);
}

void TableCollapsedBorderPainter::GetVisibleEdgesSection(
    VisibleEdgeContainer& edges) {
  edges.clear();
  Color resolved_color = layout_table_section_->ResolveColor(CSSPropertyColor);
  // Traverse all the section edges, and assign them
  FillVisibleRect(edges, 0, 0, num_rows_, num_effective_columns_,
                  layout_table_section_->Style(), TextDirection::kLtr,
                  layout_table_section_->Table()->Style()->GetWritingMode(),
                  resolved_color, kBorderPrecedenceRowGroup);
}

void TableCollapsedBorderPainter::GetVisibleEdgesColgroup(
    VisibleEdgeContainer& edges,
    const LayoutTableCol* colgroup) {
  edges.clear();
  Color resolved_color = colgroup->ResolveColor(CSSPropertyColor);
  Vector<unsigned> indexes = colgroup->GetEffectiveColumnIndexes();
  if (indexes.size() < 1)
    return;
  LayoutTable* table = layout_table_section_->Table();
  TextDirection direction = table->Style()->Direction();
  unsigned start_col_index = indexes[0];
  unsigned end_col_index = indexes.back() + 1;
  if (direction == TextDirection::kRtl) {
    unsigned cols = table->EffectiveColumns().size();
    unsigned old_start_col_index = start_col_index;
    start_col_index = cols - end_col_index;
    end_col_index = cols - old_start_col_index;
  }
  FillVisibleRect(edges, 0, start_col_index, num_rows_, end_col_index,
                  colgroup->Style(), direction,
                  table->Style()->GetWritingMode(), resolved_color,
                  kBorderPrecedenceColumnGroup);
}

void TableCollapsedBorderPainter::GetVisibleEdgesCol(
    VisibleEdgeContainer& edges,
    const LayoutTableCol* col) {
  edges.clear();
  Color resolved_color = col->ResolveColor(CSSPropertyColor);
  LayoutTable* table = layout_table_section_->Table();
  if (!table)
    return;
  Vector<unsigned> indexes = col->GetEffectiveColumnIndexes();
  if (indexes.size() > 0) {
    // fill multiple virtual columns
    for (auto idx = indexes.rbegin(); idx != indexes.rend(); ++idx) {
      FillVisibleRect(edges, 0, *idx, num_rows_, *idx + 1, col->Style(),
                      TextDirection::kLtr, table->Style()->GetWritingMode(),
                      resolved_color, kBorderPrecedenceColumn);
    }
  }
  // unsigned startAbsoluteColIndex = table->colElementToAbsoluteColumn(col);
  // unsigned endAbsoluteColIndex = startAbsoluteColIndex + col->span();
  // unsigned start_col_index =
  // table->absolute_columnToEffectiveColumn(startAbsoluteColIndex); unsigned
  // end_col_index =
  // table->absolute_columnToEffectiveColumn(endAbsoluteColIndex);
  // // We replicat column span times. Weird, but that is the standard:
  // // "For the purposes of the CSS table model, the col element is expected to
  // be treated as if it was present as many times as its span attribute
  // specifies." for (auto tmpEnd = end_col_index; tmpEnd > start_col_index;
  // tmpEnd--) {
  //     FillVisibleRect(edges, 0, tmpEnd - 1, num_rows_, tmpEnd, col->Style(),
  //         col->Style()->Direction(), col->Style()->GetWritingMode(),
  //         resolved_color, BorderPrecedenceColumn);
  // }
}

void TableCollapsedBorderPainter::GetVisibleEdgesRow(
    VisibleEdgeContainer& edges,
    const LayoutTableRow* row) {
  edges.clear();
  Color resolved_color = row->ResolveColor(CSSPropertyColor);
  unsigned rowIndex = row->RowIndex();
  // We get direction from section because row direction only applies to cells,
  // not the entire row.
  FillVisibleRect(edges, rowIndex, 0, rowIndex + 1, num_effective_columns_,
                  row->Style(), TextDirection::kLtr,
                  row->Table()->Style()->GetWritingMode(), resolved_color,
                  kBorderPrecedenceRow);
}

void TableCollapsedBorderPainter::GetVisibleEdgesCell(
    VisibleEdgeContainer& edges,
    const LayoutTableCell* cell,
    TextDirection direction) {
  edges.clear();
  Color resolved_color = cell->ResolveColor(CSSPropertyColor);
  if (!cell->Parent()) {
    LOG(INFO) << "DETACHED CELL";
    return;
  }
  const LayoutTable* table = layout_table_section_->Table();
  unsigned start_row = cell->Parent() ? cell->RowIndex() : 65000;
  unsigned absolute_col = cell->AbsoluteColumnIndex();
  unsigned eff_start_col = table->AbsoluteColumnToEffectiveColumn(absolute_col);
  unsigned eff_end_col =
      table->AbsoluteColumnToEffectiveColumn(absolute_col + cell->ColSpan());
  if (direction == TextDirection::kRtl) {
    unsigned cols = table->EffectiveColumns().size();
    unsigned old_eff_start_col = eff_start_col;
    eff_start_col = cols - eff_end_col;
    eff_end_col = cols - old_eff_start_col;
  }
  unsigned end_row = std::min(start_row + cell->RowSpan(), num_rows_);
  // LOG(INFO) << "m_columns: " << table->effective_columns().size()
  //      << " start_row: " << start_row << " end_row: " << end_row
  //      << " startCol: " << eff_start_col << " endCol:" << eff_end_col
  //      << " abscol:" << absolute_col;
  // Fill border
  // cell's direction is LTR because we do not rotate cell's borders
  FillVisibleRect(edges, start_row, eff_start_col, end_row, eff_end_col,
                  cell->Style(), direction, table->Style()->GetWritingMode(),
                  resolved_color, kBorderPrecedenceCell);
  // Hide row inner edges
  for (unsigned row = start_row + 1; row < end_row; row++) {
    if (row >= start_visible_row_ && row < end_visible_row_) {
      for (unsigned effCol = eff_start_col; effCol < eff_end_col; effCol++) {
        if (effCol >= start_visible_column_ && effCol < end_visible_column_) {
          edges.push_back(VisibleEdgeRecord(
              row, effCol, direction == TextDirection::kLtr ? East : West,
              hidden_border_, kBorderPrecedenceCell, resolved_color, true));
        }
      }
    }
  }
  // Hide col inner edges
  for (unsigned effCol = eff_start_col + 1; effCol < eff_end_col; effCol++) {
    if (effCol >= start_visible_column_ && effCol < end_visible_column_) {
      for (unsigned row = start_row; row < end_row; row++) {
        if (row >= start_visible_row_ && row < end_visible_row_) {
          edges.push_back(VisibleEdgeRecord(row, effCol, South, hidden_border_,
                                            kBorderPrecedenceCell,
                                            resolved_color, true));
        }
      }
    }
  }
  // ShowVisibleEdges(edges);
}

void TableCollapsedBorderPainter::RotateBorders(
    TextDirection text_direction,
    WritingMode writing_mode,
    const BorderValue** top,
    const BorderValue** right,
    const BorderValue** bottom,
    const BorderValue** left) const {
  bool is_ltr = blink::IsLtr(text_direction);
  if (blink::IsHorizontalWritingMode(writing_mode)) {
    if (is_ltr) {
    }  // do nothing
    else {
      std::swap(*right, *left);
    }
  } else if (blink::IsFlippedBlocksWritingMode(writing_mode)) {
    if (is_ltr) {
      auto tmp = *top;
      *top = *right;
      *right = *bottom;
      *bottom = *left;
      *left = tmp;
    } else {
      std::swap(*top, *right);
      std::swap(*bottom, *left);
    }
  } else {  // IsFlippedLinesWritingMode
    if (is_ltr) {
      std::swap(*bottom, *right);
      std::swap(*left, *top);
    } else {
      auto tmp = *top;
      *top = *left;
      *left = *bottom;
      *bottom = *right;
      *right = tmp;
    }
  }
}

void TableCollapsedBorderPainter::FillVisibleRect(
    VisibleEdgeContainer& edges,
    unsigned start_row,
    unsigned start_column,
    unsigned end_row,
    unsigned end_column,
    const ComputedStyle* border_style,
    TextDirection text_direction,
    WritingMode writing_mode,
    Color resolved_color,
    EBorderPrecedence precedence) {
  const BorderValue top_border_value = border_style->BorderTop();
  const BorderValue* top_border = &top_border_value;
  const BorderValue right_border_value = border_style->BorderRight();
  const BorderValue* right_border = &right_border_value;
  const BorderValue bottom_border_value = border_style->BorderBottom();
  const BorderValue* bottom_border = &bottom_border_value;
  const BorderValue left_border_value = border_style->BorderLeft();
  const BorderValue* left_border = &left_border_value;
  RotateBorders(TextDirection::kLtr, writing_mode, &top_border, &right_border,
                &bottom_border, &left_border);
  FillHorizontalEdges(edges, start_row, start_column, end_column, *top_border,
                      precedence, resolved_color);
  FillHorizontalEdges(edges, end_row, start_column, end_column, *bottom_border,
                      precedence, resolved_color);
  FillVerticalEdges(edges, start_column, start_row, end_row, *left_border,
                    precedence, resolved_color);
  FillVerticalEdges(edges, end_column, start_row, end_row, *right_border,
                    precedence, resolved_color);
}

void TableCollapsedBorderPainter::FillHorizontalEdges(
    VisibleEdgeContainer& edges,
    unsigned row,
    unsigned start_column,
    unsigned end_column,
    const BorderValue& border,
    EBorderPrecedence precedence,
    Color resolved_color) {
  if (border.Style() == EBorderStyle::kNone)
    return;
  if (row >= start_visible_row_ && row < end_visible_row_) {
    for (unsigned c = start_column; c < end_column; c++) {
      // FIXME: modify for loop instead of comparing guard
      if (c >= start_visible_column_ && c < end_visible_column_) {
        edges.push_back(VisibleEdgeRecord(row, c, East, border, precedence,
                                          resolved_color));
      }
    }
  }
}

void TableCollapsedBorderPainter::FillVerticalEdges(
    VisibleEdgeContainer& edges,
    unsigned column,
    unsigned start_row,
    unsigned end_row,
    const BorderValue& border,
    EBorderPrecedence precedence,
    Color resolved_color) {
  if (border.Style() == EBorderStyle::kNone)
    return;
  if (column >= start_visible_column_ && column < end_visible_column_) {
    for (unsigned r = start_row; r < end_row; r++) {
      // FIXME: modify for loop instead of comparing guard
      if (r >= start_visible_row_ && r < end_visible_row_) {
        edges.push_back(VisibleEdgeRecord(r, column, South, border, precedence,
                                          resolved_color));
      }
    }
  }
}

void TableCollapsedBorderPainter::MergeVisibleEdges(
    VisibleEdgeContainer& edges) {
  // ShowVisibleEdges(edges);
  // ShowEdges();
  for (auto edge = edges.begin(); edge < edges.end(); edge++) {
    unsigned idx =
        EdgeToIndex((*edge).m_row, (*edge).m_column, (*edge).direction_);
    DCHECK(idx != npos);
    EdgeRecord edge_record((*edge).border_, (*edge).precedence_,
                           (*edge).resolved_color_);
    if ((*edge).force_removal_)
      edges_[idx] = edge_record;
    // if both edges are same priority, new edge wins
    if (CompareEdges(edges_[idx], edge_record) != 1)
      edges_[idx] = edge_record;
  }
}

static EBorderStyle CollapsedBorderStyle(EBorderStyle style) {
  if (style == EBorderStyle::kOutset)
    return EBorderStyle::kGroove;
  if (style == EBorderStyle::kInset)
    return EBorderStyle::kRidge;
  return style;
}

void TableCollapsedBorderPainter::PaintEdges(const PaintInfo& paint_info,
                                             const LayoutPoint& paint_offset,
                                             const CellSpan& rows,
                                             const CellSpan& cols) {
  // see TableCellPainter::paintCollapsedBorders
  LayoutRect paint_rect = layout_table_section_->GetCellPhysicalPosition(
      rows.Start(), cols.Start());
  paint_rect.Unite(layout_table_section_->GetCellPhysicalPosition(
      rows.End() > 0 ? rows.End() - 1 : 0,
      cols.End() > 0 ? cols.End() - 1 : 0));
  // paint_rect size calculation is not exact, we expand by maxborder, instead
  // of exact border
  paint_rect.Expand(LayoutSize(LayoutUnit(max_intersection_width_ * 2),
                               LayoutUnit(max_intersection_height_ * 2)));
  paint_rect.MoveBy(paint_offset);
  LayoutPoint location = layout_table_section_->Location();
  paint_rect.MoveBy(location);
  if (LayoutObjectDrawingRecorder::UseCachedDrawingIfPossible(
          paint_info.context, *layout_table_section_,
          DisplayItem::kTableCollapsedBorders))
    return;
  LayoutObjectDrawingRecorder recorder(
      paint_info.context, *layout_table_section_,
      DisplayItem::kTableCollapsedBorders, paint_rect);
  // Paint edges carefully to get them all
  // Every row paint    : ┌┌┌┌┌, then |
  // Bottom border paint: _____
  LayoutTable* table = layout_table_section_->Table();
  WritingMode writing_mode = table->Style()->GetWritingMode();
  TextDirection text_direction = TextDirection::kLtr;
  for (unsigned r = rows.Start(); r < rows.End(); r++) {
    // const LayoutTableRow * row = layout_table_section_->rowLayoutObjectAt(r);
    // if (row)
    //     text_direction = row->Style()->Direction();
    for (unsigned c = cols.Start(); c < cols.End(); c++) {
      PaintOneEdge(paint_info, paint_offset, r, c, East, writing_mode,
                   text_direction);
      PaintOneEdge(paint_info, paint_offset, r, c, South, writing_mode,
                   text_direction);
    }
    // paint the last column on edge
    PaintOneEdge(paint_info, paint_offset, r, cols.End(), South, writing_mode,
                 text_direction);
  }
  // Paint the last row
  for (unsigned c = cols.Start(); c < cols.End(); c++) {
    PaintOneEdge(paint_info, paint_offset, rows.End(), c, East, writing_mode,
                 text_direction);
  }
}

// Returns a rect that encloses 'direction' border of the cell_rect
LayoutRect TableCollapsedBorderPainter::CellRectAsBorder(
    const LayoutRect& cell_rect,
    BoxSide side,
    unsigned border_width,
    WritingMode writing_mode,
    TextDirection text_direction) {
  LayoutUnit small_half(border_width / 2);  // C++ always rounds down
  // unsigned bigHalf = border_width - small_half;
  LayoutUnit top_offset;
  LayoutUnit left_offset;
  /* Tricky: if the border width is odd, we need a consistent way of
   *      distributing the extra pixel. The rule is:
   *           - south and east edges always get the extra pixel on the inside
   *                  of the cell
   *                       */
  bool is_ltr = blink::IsLtr(text_direction);
  if (blink::IsHorizontalWritingMode(writing_mode)) {
    if (is_ltr) {
      top_offset = small_half;
      left_offset = small_half;
    } else {
      top_offset = small_half;
      left_offset = small_half;
    }
  } else {  // vertical
    if (blink::IsFlippedBlocksWritingMode(writing_mode)) {
      if (is_ltr) {
        top_offset = small_half;
        left_offset = small_half;
      } else {
        top_offset = small_half;
        left_offset = small_half;
      }
    } else {  // flippedLines
      if (is_ltr) {
        top_offset = small_half;
        left_offset = small_half;
      } else {
        top_offset = small_half;
        left_offset = small_half;
      }
    }
  }
  switch (side) {
    case kBSTop:
      return LayoutRect(
          LayoutPoint(cell_rect.X(), cell_rect.Y() - top_offset),
          LayoutSize(cell_rect.Width(), LayoutUnit(border_width)));
    case kBSBottom:
      return LayoutRect(
          LayoutPoint(cell_rect.X(), cell_rect.MaxY() - top_offset),
          LayoutSize(cell_rect.Width(), LayoutUnit(border_width)));
    case kBSLeft:
      return LayoutRect(
          LayoutPoint(cell_rect.X() - left_offset, cell_rect.Y()),
          LayoutSize(LayoutUnit(border_width), cell_rect.Height()));
    case kBSRight:
      return LayoutRect(
          LayoutPoint(cell_rect.MaxX() - left_offset, cell_rect.Y()),
          LayoutSize(LayoutUnit(border_width), cell_rect.Height()));
  }
}

//
LayoutRect TableCollapsedBorderPainter::EdgePaintPosition(
    unsigned row,
    unsigned col,
    unsigned border_width,
    EdgeDirection direction,
    WritingMode writing_mode,
    TextDirection text_direction) {
  // Compute edge's size from size of the cell
  LayoutRect position;
  bool is_ltr = blink::IsLtr(text_direction);
  BoxSide side = kBSLeft;
  LayoutRect cell_rect;
  // If this is the last row intersection, get size from cell in next to last
  if (row == num_rows_) {
    DCHECK(direction == East);
    cell_rect = GetCellPhysicalPosition(row - 1, col);
    if (blink::IsHorizontalWritingMode(writing_mode)) {
      side = kBSBottom;
    } else if (blink::IsFlippedBlocksWritingMode(writing_mode)) {
      side = kBSLeft;
    } else {
      side = kBSRight;
    }
  } else if (col == num_effective_columns_) {
    // If this is the last column, get size from the cell before
    DCHECK(direction == South);
    cell_rect = GetCellPhysicalPosition(row, col - 1);
    if (blink::IsHorizontalWritingMode(writing_mode)) {
      side = is_ltr ? kBSRight : kBSLeft;
    } else if (blink::IsFlippedBlocksWritingMode(writing_mode)) {
      side = is_ltr ? kBSBottom : kBSTop;
    } else {
      side = is_ltr ? kBSBottom : kBSTop;
    }
  } else {
    cell_rect = GetCellPhysicalPosition(row, col);
    if (direction == East) {
      if (blink::IsHorizontalWritingMode(writing_mode)) {
        side = kBSTop;
      } else if (blink::IsFlippedBlocksWritingMode(writing_mode)) {
        side = kBSRight;
      } else {
        side = kBSLeft;
      }
    } else {  // direction == South
      if (blink::IsHorizontalWritingMode(writing_mode)) {
        side = is_ltr ? kBSLeft : kBSRight;
      } else if (blink::IsFlippedBlocksWritingMode(writing_mode)) {
        side = is_ltr ? kBSTop : kBSBottom;
      } else {  // flippedLines
        side = is_ltr ? kBSTop : kBSBottom;
      }
    }
  }
  return CellRectAsBorder(cell_rect, side, border_width, writing_mode,
                          text_direction);
}

// Paints an edge starting from intersection[row, col] in direction
void TableCollapsedBorderPainter::PaintOneEdge(const PaintInfo& paint_info,
                                               const LayoutPoint& paint_offset,
                                               unsigned row,
                                               unsigned col,
                                               EdgeDirection direction,
                                               WritingMode writing_mode,
                                               TextDirection text_direction) {
  DCHECK(direction == South || direction == East);
  unsigned idx = EdgeToIndex(row, col, direction);
  DCHECK(idx != npos);
  const BorderValue& border = edges_[idx].border_;
  if (border.Style() == EBorderStyle::kHidden || !border.NonZero()) {
    return;
  }
  LayoutTableSection* section_above =
      layout_table_section_->Table()->SectionAbove(layout_table_section_,
                                                   kSkipEmptySections);
  if (!row && direction == East && section_above) {
    return;
  }
  LayoutRect position = EdgePaintPosition(row, col, border.Width(), direction,
                                          writing_mode, text_direction);
  AdjustForIntersections(position, row, col, direction, writing_mode,
                         text_direction);
  // Offset by section_location
  LayoutPoint section_location = layout_table_section_->Location();
  position.MoveBy(section_location);
  position.MoveBy(paint_offset);
  // EdgeRecord edge = edges_[idx];
  // ShowEdge(row, col, direction, edge.precedence_, edge.border_,
  //         border.GetColor().Resolve(edges_[idx].resolved_color_));
  BoxSide side;
  if (blink::IsHorizontalWritingMode(writing_mode))
    side = direction == East ? kBSTop : kBSLeft;
  else
    side = direction == East ? kBSLeft : kBSTop;
  ObjectPainter::DrawLineForBoxSide(
      paint_info.context, position.X(), position.Y(), position.MaxX(),
      position.MaxY(),  // left, top, right, bottom,
      side, border.GetColor().Resolve(edges_[idx].resolved_color_),
      CollapsedBorderStyle(border.Style()), 0, 0, true);
}

void TableCollapsedBorderPainter::AdjustForIntersections(
    LayoutRect& position,
    unsigned row,
    unsigned col,
    EdgeDirection direction,
    WritingMode writing_mode,
    TextDirection text_direction) {
  DCHECK(direction == South || direction == East);
  const Intersection* start_intersection = nullptr;
  const Intersection* end_intersection = nullptr;
  bool winner_start = false;
  bool winner_end = false;
  // START intersection
  unsigned index = IntersectionToIndex(row, col);
  if (index != npos) {
    start_intersection = &intersections_[index];
    winner_start = start_intersection->direction_ == direction;
  }
  // END intersection
  if (direction == South)
    row += 1;
  else
    col += 1;
  index = IntersectionToIndex(row, col);
  if (index != npos) {
    end_intersection = &intersections_[index];
    if (direction == South)
      winner_end = end_intersection->direction_ == North;
    else
      winner_end = end_intersection->direction_ == West;
  }
  bool is_ltr = blink::IsLtr(text_direction);
  if (blink::IsHorizontalWritingMode(writing_mode)) {
    AdjustForIntersectionsHorizontal(position, direction, is_ltr,
                                     start_intersection, winner_start,
                                     end_intersection, winner_end);
  } else if (blink::IsFlippedBlocksWritingMode(writing_mode)) {
    AdjustForIntersectionsFlippedBlocks(position, direction, is_ltr,
                                        start_intersection, winner_start,
                                        end_intersection, winner_end);
  } else if (blink::IsFlippedLinesWritingMode(writing_mode)) {
    AdjustForIntersectionsFlippedLines(position, direction, is_ltr,
                                       start_intersection, winner_start,
                                       end_intersection, winner_end);
  } else {
    DCHECK(false);
  }
}

/*
   writing-mode: HORIZONTAL-TB
            N
compass: W--+--E  intersection:   <--width -->
            S
  start intersection
    South edge, win : expand top by height/2
    South edge, lose: shrink top by height/2
    East edge, win  : expand left by width/2
    East edge, lose : shrink left by width/2
  end intersection
    South edge, win : expand bottom by height/2
    South edge lose : shrink bottom by height/2
    East edge, win  : expand right by width/2
    East edge, lose : shrink right by width/2

writing-mode: HORIZONTAL(RTL)
            N
compass: E--+--W     intersection: <-- width -->
            S
same as HORIZONTAL-TB in South direction
  start intersection
    South: same as LTR
    East edge, win  : expand right by width/2
    East edge, lose : shrink right by width/2
  end intersection
    South: same as LTR
    East edge, win  : expand left by width/2
    East edge, lose : shrink left by width/2
*/
void TableCollapsedBorderPainter::AdjustForIntersectionsHorizontal(
    LayoutRect& position,
    EdgeDirection direction,
    bool is_ltr,
    const Intersection* start_intersection,
    bool winner_start,
    const Intersection* end_intersection,
    bool winner_end) {
  if (start_intersection) {
    if (direction == South) {
      unsigned small_delta = start_intersection->height_.ToUnsigned() / 2;
      unsigned big_delta =
          start_intersection->height_.ToUnsigned() - small_delta;
      if (winner_start) {
        position.ExpandEdges(LayoutUnit(small_delta), LayoutUnit(0),
                             LayoutUnit(0), LayoutUnit(0));
      } else {
        position.ContractEdges(LayoutUnit(big_delta), LayoutUnit(0),
                               LayoutUnit(0), LayoutUnit(0));
      }
    } else {  // East
      unsigned small_delta = start_intersection->width_.ToUnsigned() / 2;
      unsigned big_delta =
          start_intersection->width_.ToUnsigned() - small_delta;
      if (is_ltr) {
        if (winner_start) {
          position.ExpandEdges(LayoutUnit(0), LayoutUnit(0), LayoutUnit(0),
                               LayoutUnit(small_delta));
        } else {
          position.ContractEdges(LayoutUnit(0), LayoutUnit(0), LayoutUnit(0),
                                 LayoutUnit(big_delta));
        }
      } else {  // RTL
        if (winner_start) {
          position.ExpandEdges(LayoutUnit(0), LayoutUnit(small_delta),
                               LayoutUnit(0), LayoutUnit(0));
        } else {
          position.ContractEdges(LayoutUnit(0), LayoutUnit(big_delta),
                                 LayoutUnit(0), LayoutUnit(0));
        }
      }
    }
  }
  if (end_intersection) {
    if (direction == South) {
      unsigned small_delta = end_intersection->height_.ToUnsigned() / 2;
      unsigned big_delta = end_intersection->height_.ToUnsigned() - small_delta;
      if (winner_end) {
        position.ExpandEdges(LayoutUnit(0), LayoutUnit(0),
                             LayoutUnit(big_delta), LayoutUnit(0));
      } else {
        position.ContractEdges(LayoutUnit(0), LayoutUnit(0),
                               LayoutUnit(small_delta), LayoutUnit(0));
      }
    } else {  // East
      unsigned small_delta = end_intersection->width_.ToUnsigned() / 2;
      unsigned big_delta = end_intersection->width_.ToUnsigned() - small_delta;
      if (is_ltr) {
        if (winner_end) {
          position.ExpandEdges(LayoutUnit(0), LayoutUnit(big_delta),
                               LayoutUnit(0), LayoutUnit(0));
        } else {  // loser
          position.ContractEdges(LayoutUnit(0), LayoutUnit(small_delta),
                                 LayoutUnit(0), LayoutUnit(0));
        }
      } else {  // RTL
        if (winner_end) {
          position.ExpandEdges(LayoutUnit(0), LayoutUnit(0), LayoutUnit(0),
                               LayoutUnit(big_delta));
        } else {
          position.ContractEdges(LayoutUnit(0), LayoutUnit(0), LayoutUnit(0),
                                 LayoutUnit(small_delta));
        }
      }
    }
  }
}

/*
writing-mode: VERTICAL-RL (flipped blocks)
                      W
compass: vertical: S--+--N  intersection: <--- height ---->
                      E
    start intersection
      South edge, win : expand right by height/2
      South edge, lose: shrink right by height/2
      East edge, win  : expand top by width/2
      East edge, lose : shrink top by width/2
    end intersection
      South edge, win : expand left by height/2
      South edge, lose: shrink left by height/2
      East edge, win  : expand bottom by width/2
      East edge, lose : shrink bottom by width/2
writing-mode: VERTICAL-RL RTL
                      E
compass: vertical: S--+--N  intersection: <--- height ---->
                      W
    start intersection
      South edge, win : expand right by height/2
      South edge, lose: shrink right by height/2
      East edge, win  : expand bottom by width/2
      East edge, lose : shrink bottom by width/2
    end intersection
      South edge, win : expand left by height/2
      South edge, lose: shrink left by height/2
      East edge, win  : expand top by width/2
      East edge, lose : shrink top by width/2
*/
void TableCollapsedBorderPainter::AdjustForIntersectionsFlippedBlocks(
    LayoutRect& position,
    EdgeDirection direction,
    bool is_ltr,
    const Intersection* start_intersection,
    bool winner_start,
    const Intersection* end_intersection,
    bool winner_end) {
  if (start_intersection) {
    if (direction == South) {
      unsigned small_delta = start_intersection->height_.ToUnsigned() / 2;
      unsigned big_delta =
          start_intersection->height_.ToUnsigned() - small_delta;
      if (winner_start) {
        position.ExpandEdges(LayoutUnit(0), LayoutUnit(small_delta),
                             LayoutUnit(0), LayoutUnit(0));
      } else {
        position.ContractEdges(LayoutUnit(0), LayoutUnit(big_delta),
                               LayoutUnit(0), LayoutUnit(0));
      }
    } else {  // East
      unsigned small_delta = start_intersection->width_.ToUnsigned() / 2;
      unsigned big_delta =
          start_intersection->width_.ToUnsigned() - small_delta;
      if (is_ltr) {
        if (winner_start) {
          position.ExpandEdges(LayoutUnit(small_delta), LayoutUnit(0),
                               LayoutUnit(0), LayoutUnit(0));
        } else {
          position.ContractEdges(LayoutUnit(big_delta), LayoutUnit(0),
                                 LayoutUnit(0), LayoutUnit(0));
        }
      } else {  // RTL
        if (winner_start) {
          position.ExpandEdges(LayoutUnit(0), LayoutUnit(0),
                               LayoutUnit(big_delta), LayoutUnit(0));
        } else {
          position.ContractEdges(LayoutUnit(0), LayoutUnit(0),
                                 LayoutUnit(small_delta), LayoutUnit(0));
        }
      }
    }
  }
  if (end_intersection) {
    if (direction == South) {
      unsigned small_delta = end_intersection->height_.ToUnsigned() / 2;
      unsigned big_delta = end_intersection->height_.ToUnsigned() - small_delta;
      if (winner_end) {
        position.ExpandEdges(LayoutUnit(0), LayoutUnit(0), LayoutUnit(0),
                             LayoutUnit(big_delta));
      } else {
        position.ContractEdges(LayoutUnit(0), LayoutUnit(0), LayoutUnit(0),
                               LayoutUnit(small_delta));
      }
    } else {  // East
      unsigned small_delta = end_intersection->width_.ToUnsigned() / 2;
      unsigned big_delta = end_intersection->width_.ToUnsigned() - small_delta;
      if (is_ltr) {
        if (winner_end) {
          position.ExpandEdges(LayoutUnit(0), LayoutUnit(0),
                               LayoutUnit(big_delta), LayoutUnit(0));
        } else {  // loser
          position.ContractEdges(LayoutUnit(0), LayoutUnit(0),
                                 LayoutUnit(small_delta), LayoutUnit(0));
        }
      } else {  // RTL
        if (winner_end) {
          position.ExpandEdges(LayoutUnit(big_delta), LayoutUnit(0),
                               LayoutUnit(0), LayoutUnit(0));
        } else {
          position.ContractEdges(LayoutUnit(small_delta), LayoutUnit(0),
                                 LayoutUnit(0), LayoutUnit(0));
        }
      }
    }
  }
}

/*
writing-mode: VERTICAL-LR (flipped lines)
                    W
compass: rotated N--+--S
                    E
    start intersection
        East edge, win  : expand top by width/2
        East edge, lose : shrink top by width/2
        South edge, win : expand left by height/2
        South edge, lose: shrink left by height/2
    end intersection
        East edge, win  : expand bottom by width/2
        East edge, lose : shrink bottom by width/2
        South edge, win : expand right by height/2
        South edge lose : shrink right by height/2
writing-mode: VERTICAL-LR, DIRECTION RTL
           E
compass: N-+-S     intersection <--height -->
           W
    start intersection
        East edge. win : expand bottom by width / 2
        East edge, lose: shrink bottom by width / 2
    end intersection
        East edge, win : expand top by width / 2
        East edge, lose: expand top by width / 2
*/
void TableCollapsedBorderPainter::AdjustForIntersectionsFlippedLines(
    LayoutRect& position,
    EdgeDirection direction,
    bool is_ltr,
    const Intersection* start_intersection,
    bool winner_start,
    const Intersection* end_intersection,
    bool winner_end) {
  if (start_intersection) {
    if (direction == South) {
      unsigned small_delta = start_intersection->height_.ToUnsigned() / 2;
      unsigned big_delta =
          start_intersection->height_.ToUnsigned() - small_delta;
      if (winner_start) {
        position.ExpandEdges(LayoutUnit(0), LayoutUnit(0), LayoutUnit(0),
                             LayoutUnit(small_delta));
      } else {
        position.ContractEdges(LayoutUnit(0), LayoutUnit(0), LayoutUnit(0),
                               LayoutUnit(big_delta));
      }
    } else {  // East
      unsigned small_delta = start_intersection->width_.ToUnsigned() / 2;
      unsigned big_delta =
          start_intersection->width_.ToUnsigned() - small_delta;
      if (is_ltr) {
        if (winner_start) {
          position.ExpandEdges(LayoutUnit(small_delta), LayoutUnit(0),
                               LayoutUnit(0), LayoutUnit(0));
        } else {
          position.ContractEdges(LayoutUnit(big_delta), LayoutUnit(0),
                                 LayoutUnit(0), LayoutUnit(0));
        }
      } else {  // RTL
        if (winner_start) {
          position.ExpandEdges(LayoutUnit(0), LayoutUnit(0),
                               LayoutUnit(small_delta), LayoutUnit(0));
        } else {
          position.ContractEdges(LayoutUnit(0), LayoutUnit(0),
                                 LayoutUnit(big_delta), LayoutUnit(0));
        }
      }
    }
  }
  if (end_intersection) {
    if (direction == South) {
      unsigned small_delta = end_intersection->height_.ToUnsigned() / 2;
      unsigned big_delta = end_intersection->height_.ToUnsigned() - small_delta;
      if (winner_end) {
        position.ExpandEdges(LayoutUnit(0), LayoutUnit(big_delta),
                             LayoutUnit(0), LayoutUnit(0));
      } else {
        position.ContractEdges(LayoutUnit(0), LayoutUnit(small_delta),
                               LayoutUnit(0), LayoutUnit(0));
      }
    } else {  // East
      unsigned small_delta = end_intersection->width_.ToUnsigned() / 2;
      unsigned big_delta = end_intersection->width_.ToUnsigned() - small_delta;
      if (is_ltr) {
        if (winner_end) {
          position.ExpandEdges(LayoutUnit(0), LayoutUnit(0),
                               LayoutUnit(big_delta), LayoutUnit(0));
        } else {  // loser
          position.ContractEdges(LayoutUnit(0), LayoutUnit(0),
                                 LayoutUnit(small_delta), LayoutUnit(0));
        }
      } else {  // RTL
        if (winner_end) {
          position.ExpandEdges(LayoutUnit(big_delta), LayoutUnit(0),
                               LayoutUnit(0), LayoutUnit(0));
        } else {
          position.ContractEdges(LayoutUnit(small_delta), LayoutUnit(0),
                                 LayoutUnit(0), LayoutUnit(0));
        }
      }
    }
  }
}

// returns edge index for position, TableCollapsedBorderPainter::npos if out of
// range topLeft intersection is 0, 0, bottomRight is numRows+1, numColumns + 1
unsigned TableCollapsedBorderPainter::EdgeToIndex(
    unsigned intersection_row,
    unsigned intersection_column,
    EdgeDirection direction) const {
  /*  Edges map to m_edges like this:
      - each intersection 'stores' only two edges: east and south edges
      - to map edge to index:
      - find intersection so that requested edge is east or south
      - edge is row * cols * 2 + (east ? 0 : 1)
      - for example:
      - North edge at intersection 5, 3, is also South edge at intersection 4, 3
      - its index is 4 (rows) * 3 (cols) * 2 + 1 (for South)
      */
  switch (direction) {
    case North:
      intersection_row -= 1;
      direction = South;
      break;
    case West:
      intersection_column -= 1;
      direction = East;
      break;
    case South:
    case East:
    case None:
      break;
  }
  // Check if we are inside the covered area
  if (intersection_row < start_visible_row_ ||
      intersection_column < start_visible_column_ ||
      intersection_row >= end_visible_row_ ||
      intersection_column >= end_visible_column_) {
    // LOG(ERROR) << "GOT AN NPOS";
    return TableCollapsedBorderPainter::npos;
  }
  unsigned index = (intersection_row - start_visible_row_) *
                   (end_visible_column_ - start_visible_column_) * 2;
  index += (intersection_column - start_visible_column_) * 2;
  return direction == East ? index
                           : index + 1;  // each vertex holds [East, South]
}

unsigned TableCollapsedBorderPainter::IntersectionToIndex(
    unsigned row,
    unsigned column) const {
  if (row < start_visible_row_ || column < start_visible_column_ ||
      row >= end_visible_row_ || column >= end_visible_column_)
    return TableCollapsedBorderPainter::npos;
  return (row - start_visible_row_) *
             (end_visible_column_ - start_visible_column_) +
         column;
}
}  // namespace blink
// blink
