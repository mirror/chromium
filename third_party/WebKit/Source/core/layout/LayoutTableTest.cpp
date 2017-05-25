// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/layout/LayoutTable.h"

#include "core/layout/LayoutTestHelper.h"

namespace blink {

namespace {

class LayoutTableTest : public RenderingTest {
 protected:
  LayoutTable* GetTableByElementId(const char* id) {
    return ToLayoutTable(GetLayoutObjectByElementId(id));
  }
};

TEST_F(LayoutTableTest, OverflowWithCollapsedBorders) {
  SetBodyInnerHTML(
      "<style>"
      "  table { border-collapse: collapse }"
      "  td { border: 0px solid blue; padding: 0 }"
      "  div { width: 100px; height: 100px }"
      "</style>"
      "<table id='table'>"
      "  <tr>"
      "    <td style='border-bottom-width: 10px;"
      "        outline: 3px solid blue'><div></div></td>"
      "    <td style='border-width: 3px 15px'><div></div></td>"
      "  </tr>"
      "  <tr style='outline: 8px solid green'><td><div></div></td></tr>"
      "</table>");

  auto* table = GetTableByElementId("table");

  // The table's self visual overflow covers the collapsed borders.
  LayoutRect expected_self_visual_overflow = table->BorderBoxRect();
  expected_self_visual_overflow.ExpandEdges(LayoutUnit(1), LayoutUnit(8),
                                            LayoutUnit(0), LayoutUnit(0));
  EXPECT_EQ(expected_self_visual_overflow, table->SelfVisualOverflowRect());

  // The table's visual overflow covers self visual overflow and visual
  // overflows rows.
  LayoutRect expected_visual_overflow = table->BorderBoxRect();
  expected_visual_overflow.ExpandEdges(LayoutUnit(3), LayoutUnit(8),
                                       LayoutUnit(8), LayoutUnit(8));
  EXPECT_EQ(expected_visual_overflow, table->VisualOverflowRect());
}

TEST_F(LayoutTableTest, CollapsedBorders) {
  SetBodyInnerHTML(
      "<style>table { border-collapse: collapse }</style>"
      "<table id='table1'"
      "    style='border-top: hidden; border-bottom: 8px solid;"
      "           border-left: hidden; border-right: 10px solid'>"
      "  <tr><td>A</td><td>B</td></tr>"
      "</table>"
      "<table id='table2' style='border: 10px solid'>"
      "  <tr>"
      "    <td style='border: hidden'>C</td>"
      "    <td style='border: hidden'>D</td>"
      "  </tr>"
      "</table>"
      "<table id='table3' style='border: 10px solid'>"
      "  <tr>"
      "    <td style='border-top: 15px solid;"
      "               border-left: 21px solid'>E</td>"
      "    <td style='border-bottom: 19px solid;"
      "               border-right: 25px solid'>F</td>"
      "  </tr>"
      "</table>");

  auto* table1 = GetTableByElementId("table1");
  EXPECT_EQ(0, table1->BorderBefore());
  EXPECT_EQ(4, table1->BorderAfter());
  EXPECT_EQ(0, table1->BorderStart());
  EXPECT_EQ(5, table1->BorderEnd());

  // All cells have hidden border.
  auto* table2 = GetTableByElementId("table2");
  EXPECT_EQ(0, table2->BorderBefore());
  EXPECT_EQ(0, table2->BorderAfter());
  EXPECT_EQ(0, table2->BorderStart());
  EXPECT_EQ(0, table2->BorderEnd());

  // Cells have wider borders.
  auto* table3 = GetTableByElementId("table3");
  // Cell E's border-top won.
  EXPECT_EQ(7, table3->BorderBefore());
  // Cell F's border-bottom won.
  EXPECT_EQ(10, table3->BorderAfter());
  // Cell E's border-left won.
  EXPECT_EQ(10, table3->BorderStart());
  // Cell F's border-bottom won.
  EXPECT_EQ(13, table3->BorderEnd());
}

TEST_F(LayoutTableTest, CollapsedBordersWithCol) {
  SetBodyInnerHTML(
      "<style>table { border-collapse: collapse }</style>"
      "<table id='table1' style='border: hidden'>"
      "  <colgroup>"
      "    <col colspan='2000' style='border: 10px solid'>"
      "    <col colspan='2000' style='border: 20px solid'>"
      "  </colgroup>"
      "  <tr>"
      "    <td colspan='2000'>A</td>"
      "    <td colspan='2000'>B</td>"
      "  </tr>"
      "</table>"
      "<table id='table2' style='border: 10px solid'>"
      "  <colgroup>"
      "    <col colspan='2000' style='border: 10px solid'>"
      "    <col colspan='2000' style='border: 20px solid'>"
      "  </colgroup>"
      "  <tr>"
      "    <td colspan='2000' style='border: hidden'>C</td>"
      "    <td colspan='2000' style='border: hidden'>D</td>"
      "  </tr>"
      "</table>"
      "<table id='table3'>"
      "  <colgroup>"
      "    <col colspan='2000' style='border: 10px solid'>"
      "    <col colspan='2000' style='border: 20px solid'>"
      "  </colgroup>"
      "  <tr>"
      "    <td colspan='2000' style='border: 12px solid'>E</td>"
      "    <td colspan='2000' style='border: 16px solid'>F</td>"
      "  </tr>"
      "</table>");

  // Table has hidden border.
  auto* table1 = GetTableByElementId("table1");
  EXPECT_EQ(0, table1->BorderBefore());
  EXPECT_EQ(0, table1->BorderAfter());
  EXPECT_EQ(0, table1->BorderStart());
  EXPECT_EQ(0, table1->BorderEnd());

  // All cells have hidden border.
  auto* table2 = GetTableByElementId("table2");
  EXPECT_EQ(0, table2->BorderBefore());
  EXPECT_EQ(0, table2->BorderAfter());
  EXPECT_EQ(0, table2->BorderStart());
  EXPECT_EQ(0, table2->BorderEnd());

  // Combined cell and col borders.
  auto* table3 = GetTableByElementId("table3");
  // Col 2's border-top won.
  EXPECT_EQ(10, table3->BorderBefore());
  // Col 2's border-bottom won.
  EXPECT_EQ(10, table3->BorderAfter());
  // Cell E's border-left won.
  EXPECT_EQ(6, table3->BorderStart());
  // Col 2's border-right won.
  EXPECT_EQ(10, table3->BorderEnd());
}

}  // anonymous namespace

}  // namespace blink
