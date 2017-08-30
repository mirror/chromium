// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/editing/VisibleUnits.h"

#include "core/editing/EditingTestBase.h"

namespace blink {

class VisibleUnitsWordTest : public EditingTestBase {
 protected:
  std::string DoStartOfWord(const std::string& selection_text) {
    return ToSelectionText(
        *GetDocument().body(),
        SelectionInDOMTree::Builder()
            .Collapse(
                StartOfWord(CreateVisiblePosition(AsPosition(selection_text)))
                    .DeepEquivalent())
            .Build());
  }
};

TEST_F(VisibleUnitsWordTest, StartOfWordFirstLetter) {
  EXPECT_EQ("<p> (1|) abc def</p>",
            DoStartOfWord("<style>p::first-letter {font-size:200%;}</style>"
                          "<p>| (1) abc def</p>"));
}

}  // namespace blink
