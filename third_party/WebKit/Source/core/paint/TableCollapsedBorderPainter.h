// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#ifndef TableCollapsedBorderPainter_h
#define TableCollapsedBorderPainter_h
#include "core/layout/CollapsedBorderValue.h"
#include "core/layout/LayoutTableSection.h"  // for LayoutTableSection::CellStruct
#include "core/style/BorderValue.h"
#include "platform/graphics/paint/DisplayItem.h"
#include "platform/wtf/Allocator.h"
namespace blink {
struct PaintInfo;
class CellSpan;
class BorderValue;
class LayoutPoint;
class LayoutTable;
class LayoutTableCell;
class LayoutTableCol;
class LayoutTableRow;
class LayoutTableSection;
/*
 *  * Paints all the borders inside a single section
 *   */
class TableCollapsedBorderPainter {
  STACK_ALLOCATED();

 public:
  TableCollapsedBorderPainter(const LayoutTableSection* layoutTableSection)
      : layout_table_section_(layoutTableSection) {
    hidden_border_.SetStyle(EBorderStyle::kNone);
  }
  TableCollapsedBorderPainter() : layout_table_section_(nullptr) {
    hidden_border_.SetStyle(EBorderStyle::kNone);
  }
  void PaintBorders(const PaintInfo&,
                    const LayoutPoint&,
                    const CellSpan&,
                    const CellSpan&,
                    const TableCollapsedBorderPainter& previous_painter);
  struct EdgeRecord {
    EdgeRecord() : precedence_(kBorderPrecedenceOff){};
    EdgeRecord(const BorderValue& border,
               EBorderPrecedence precedence,
               Color resolved_color)
        : precedence_(precedence),
          border_(border),
          resolved_color_(resolved_color){};
    EBorderPrecedence precedence_;
    BorderValue border_;
    Color resolved_color_;
  };
  // Ordered by winning order, do not modify
  enum EdgeDirection { North, West, East, South, None };

 private:
  // Represents a visible edge in a table
  struct VisibleEdgeRecord {
    VisibleEdgeRecord(unsigned row,
                      unsigned column,
                      EdgeDirection direction,
                      const BorderValue& border,
                      EBorderPrecedence precedence,
                      Color resolved_color,
                      bool force_removal = false)
        : m_row(row),
          m_column(column),
          direction_(direction),
          precedence_(precedence),
          border_(border),
          resolved_color_(resolved_color),
          force_removal_(force_removal){};
    unsigned m_row;
    unsigned m_column;
    EdgeDirection direction_;
    EBorderPrecedence precedence_;
    BorderValue border_;
    Color resolved_color_;
    bool force_removal_;
  };
  typedef Vector<VisibleEdgeRecord> VisibleEdgeContainer;
  typedef Vector<EdgeRecord> EdgeRecordContainer;
  // Represents edge intersection
  struct Intersection {
    Intersection() : width_(0), height_(0), direction_(None){};
    Intersection(LayoutUnit width, LayoutUnit height, EdgeDirection direction)
        : width_(width), height_(height), direction_(direction){};
    LayoutUnit width_;         // width of widest north-south edge
    LayoutUnit height_;        // width of widest east-west edge
    EdgeDirection direction_;  // direction of the winning edge
  };
  typedef Vector<Intersection> IntersectionContainer;
  void InitEdges(const CellSpan&,
                 const CellSpan&,
                 const TableCollapsedBorderPainter& previous_painter);
  void PopulateEdges(const CellSpan&,
                     const CellSpan&,
                     const TableCollapsedBorderPainter& previous_painter);
  // Visible edges traversal
  void GetVisibleEdgesTable(VisibleEdgeContainer& edges, const LayoutTable*);
  void GetVisibleEdgesSection(VisibleEdgeContainer& edges);
  void GetVisibleEdgesColgroup(VisibleEdgeContainer& edges,
                               const LayoutTableCol*);
  void GetVisibleEdgesCol(VisibleEdgeContainer& edges, const LayoutTableCol*);
  void GetVisibleEdgesRow(VisibleEdgeContainer& edges, const LayoutTableRow*);
  void GetVisibleEdgesCell(VisibleEdgeContainer& edges,
                           const LayoutTableCell*,
                           TextDirection);
  void GetVisibleEdgesSiblingSection(
      VisibleEdgeContainer& edges,
      const TableCollapsedBorderPainter& previous_painter);
  // Visible edges utils
  void RotateBorders(TextDirection,
                     WritingMode,
                     const BorderValue** top_border,
                     const BorderValue** right_border,
                     const BorderValue** bottom_border,
                     const BorderValue** left_border) const;
  void FillVisibleRect(VisibleEdgeContainer& edges,
                       unsigned startRow,
                       unsigned start_column,
                       unsigned end_row,
                       unsigned end_column,
                       const ComputedStyle* border_style,
                       TextDirection,
                       WritingMode,
                       Color resolved_color,
                       EBorderPrecedence);
  void FillHorizontalEdges(VisibleEdgeContainer& edges,
                           unsigned row,
                           unsigned start_column,
                           unsigned end_column,
                           const BorderValue&,
                           EBorderPrecedence,
                           Color resolved_color);
  void FillVerticalEdges(VisibleEdgeContainer& edges,
                         unsigned column,
                         unsigned startRow,
                         unsigned end_row,
                         const BorderValue&,
                         EBorderPrecedence,
                         Color resolved_color);
  void MergeVisibleEdges(VisibleEdgeContainer& edges);
  void InitIntersections();
  void InitIntersection(unsigned row, unsigned col);
  LayoutRect GetCellPhysicalPosition(unsigned row, unsigned effective_column) {
    DCHECK(row != npos && effective_column != npos);
    return layout_table_section_->GetCellPhysicalPosition(row,
                                                          effective_column);
  };
  // Painting
  LayoutRect CellRectAsBorder(const LayoutRect& cell_rect,
                              BoxSide,
                              unsigned border_width,
                              WritingMode,
                              TextDirection);
  LayoutRect EdgePaintPosition(unsigned row,
                               unsigned col,
                               unsigned border_width,
                               EdgeDirection,
                               WritingMode,
                               TextDirection);
  void PaintEdges(const PaintInfo&,
                  const LayoutPoint&,
                  const CellSpan& rows,
                  const CellSpan& cols);
  void PaintOneEdge(const PaintInfo&,
                    const LayoutPoint&,
                    unsigned row,
                    unsigned col,
                    EdgeDirection,
                    WritingMode,
                    TextDirection);
  void AdjustForIntersections(LayoutRect& position,
                              unsigned row,
                              unsigned col,
                              EdgeDirection,
                              WritingMode,
                              TextDirection);
  void AdjustForIntersectionsHorizontal(LayoutRect& position,
                                        EdgeDirection,
                                        bool isLTR,
                                        const Intersection* start_intersection,
                                        bool winner_start,
                                        const Intersection* end_intersection,
                                        bool winner_end);
  void AdjustForIntersectionsFlippedBlocks(
      LayoutRect& position,
      EdgeDirection,
      bool isLTR,
      const Intersection* start_intersection,
      bool winner_start,
      const Intersection* end_intersection,
      bool winner_end);
  void AdjustForIntersectionsFlippedLines(
      LayoutRect& position,
      EdgeDirection,
      bool isLTR,
      const Intersection* start_intersection,
      bool winner_start,
      const Intersection* end_intersection,
      bool winner_end);
  void ShowEdge(unsigned row,
                unsigned col,
                EdgeDirection,
                EBorderPrecedence,
                const BorderValue&,
                Color) const;
  void ShowEdges(bool showHidden = false) const;
  void ShowVisibleEdges(const VisibleEdgeContainer& edges) const;
  void ShowIntersections() const;
  void ShowIntersection(unsigned row, unsigned col) const;

  // Edges:
  // Mental model for edges is that they originate at intersections (where cells
  // meet) We store two edges for each intersection: East and South. We only
  // store visible edges, not all edges in a section.
  EdgeRecordContainer edges_;
  // Limits for rows/cols stored in edges_. effective
  unsigned start_visible_row_;
  unsigned start_visible_column_;
  unsigned end_visible_row_;     // inclusive, +1 is out of bounds
  unsigned end_visible_column_;  // inclusive, +1 is out of bounds
  // not-a-position edge index
  static const unsigned npos = 0xFFFFFFFF;
  // returns edge index for position, npos for none
  unsigned EdgeToIndex(unsigned row, unsigned column, EdgeDirection) const;
  // Intersections:
  // Each intersection has width, height, and winning border
  IntersectionContainer intersections_;
  unsigned max_intersection_width_;
  unsigned max_intersection_height_;
  unsigned IntersectionToIndex(unsigned row, unsigned column) const;
  const LayoutTableSection* layout_table_section_;
  // number of rows/cols in the section
  unsigned num_rows_;
  unsigned num_effective_columns_;
  BorderValue hidden_border_;
};
}  // namespace blink
#endif  // TableCollapsedBorderPainter_h
