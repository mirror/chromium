// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/editing/SelectionAdjuster.h"

#include "core/editing/SelectionTemplate.h"
#include "core/editing/testing/EditingTestBase.h"

namespace blink {

class SelectionAdjusterTest : public EditingTestBase {};

// This is for crbug.com/775701, creating VS should not hang
TEST_F(SelectionAdjusterTest, createCrossingInputShouldNotHang) {
  const SelectionInDOMTree& selection = SetSelectionTextToBody(
      "f^oo"
      "<div contenteditable><template data-mode=open>ba|r</template></div>");
  const SelectionInFlatTree& flat = ConvertToSelectionInFlatTree(selection);
  // This should not hang.
  // const VisibleSelectionInFlatTree& vs_flat = CreateVisibleSelection(flat);
  const SelectionInFlatTree& adjusted =
      SelectionAdjuster::AdjustSelectionToAvoidCrossingEditingBoundaries(flat);
  EXPECT_EQ(adjusted, flat);
}

}  // namespace blink
