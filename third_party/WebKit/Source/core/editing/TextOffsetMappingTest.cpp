// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/editing/TextOffsetMapping.h"

#include "core/editing/Position.h"
#include "core/editing/SelectionTemplate.h"
#include "core/editing/testing/EditingTestBase.h"

namespace blink {

class TextOffsetMappingTest : public EditingTestBase {
 protected:
  std::string ComputeFirstPosition(const std::string& selection_text,
                                   int offset) {
    const PositionInFlatTree position =
        ToPositionInFlatTree(SetSelectionTextToBody(selection_text).Base());
    TextOffsetMapping mapping(position);
    return GetSelectionTextInFlatTreeFromBody(
        SelectionInFlatTree::Builder()
            .Collapse(mapping.ComputeFirstPosition(offset))
            .Build());
  }

  std::string ComputeLastPosition(const std::string& selection_text,
                                  int offset) {
    const PositionInFlatTree position =
        ToPositionInFlatTree(SetSelectionTextToBody(selection_text).Base());
    TextOffsetMapping mapping(position);
    return GetSelectionTextInFlatTreeFromBody(
        SelectionInFlatTree::Builder()
            .Collapse(mapping.ComputeLastPosition(offset))
            .Build());
  }
};

TEST_F(TextOffsetMappingTest, ComputeFirstPosition) {
  EXPECT_EQ("  |012  456  ", ComputeFirstPosition("|  012  456  ", 0));
  EXPECT_EQ("  0|12  456  ", ComputeFirstPosition("|  012  456  ", 1));
  EXPECT_EQ("  01|2  456  ", ComputeFirstPosition("|  012  456  ", 2));
  EXPECT_EQ("  012|  456  ", ComputeFirstPosition("|  012  456  ", 3));
  EXPECT_EQ("  012  |456  ", ComputeFirstPosition("|  012  456  ", 4));
  EXPECT_EQ("  012  4|56  ", ComputeFirstPosition("|  012  456  ", 5));
  EXPECT_EQ("  012  45|6  ", ComputeFirstPosition("|  012  456  ", 6));
  EXPECT_EQ("  012  456|  ", ComputeFirstPosition("|  012  456  ", 7));
  // We hit DCHECK for offset 8, because we walk on "012 def".
}

TEST_F(TextOffsetMappingTest, ComputeLastPosition) {
  EXPECT_EQ("  0|12  456  ", ComputeLastPosition("|  012  456  ", 0));
  EXPECT_EQ("  01|2  456  ", ComputeLastPosition("|  012  456  ", 1));
  EXPECT_EQ("  012|  456  ", ComputeLastPosition("|  012  456  ", 2));
  EXPECT_EQ("  012 | 456  ", ComputeLastPosition("|  012  456  ", 3));
  EXPECT_EQ("  012  4|56  ", ComputeLastPosition("|  012  456  ", 4));
  EXPECT_EQ("  012  45|6  ", ComputeLastPosition("|  012  456  ", 5));
  EXPECT_EQ("  012  456|  ", ComputeLastPosition("|  012  456  ", 6));
  EXPECT_EQ("  012  456|  ", ComputeLastPosition("|  012  456  ", 7));
  // We hit DCHECK for offset 8, because we walk on "012 def".
}

}  // namespace blink
