// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/editing/SelectionSample.h"

#include "core/editing/EditingTestBase.h"

namespace blink {

// Make sure |macro_actual_expression| is evaluated before
// |macro_expected_expression|.
#define EDITING_EXPECT_EQ(expected_expression, actual_expression) \
  {                                                               \
    const auto& actual_result = (actual_expression);              \
    EXPECT_EQ((expected_expression), actual_result);              \
  }

class SelectionSampleTest : public EditingTestBase {
 protected:
  std::string AsSelectionText(const std::string& sample_text) {
    return ToSelectionText(AsSelection(sample_text));
  }
};

TEST_F(SelectionSampleTest, ParseEmpty1) {
  const SelectionInDOMTree& selection = AsSelection("|");
  EXPECT_EQ(0u, GetDocument().body()->CountChildren());
  EXPECT_EQ(SelectionInDOMTree::Builder()
                .Collapse(Position(GetDocument().body(), 0))
                .Build(),
            selection);
}

TEST_F(SelectionSampleTest, ParseEmpty2) {
  const SelectionInDOMTree& selection = AsSelection("^|");
  EXPECT_EQ(0u, GetDocument().body()->CountChildren());
  EXPECT_EQ(SelectionInDOMTree::Builder()
                .Collapse(Position(GetDocument().body(), 0))
                .Build(),
            selection);
}

TEST_F(SelectionSampleTest, ParseElementAsSelection) {
  const SelectionInDOMTree& selection = AsSelection("<p>^<a>0</a>|<b>1</b>");
  const Element* const sample = GetDocument().QuerySelector("p");
  EXPECT_EQ(2u, sample->CountChildren())
      << "We should remove Text node for '^' and '|'.";
  EXPECT_EQ(SelectionInDOMTree::Builder()
                .Collapse(Position(sample, 0))
                .Extend(Position(sample, 1))
                .Build(),
            selection);
}

TEST_F(SelectionSampleTest, ParseTextAsSelection) {
  EDITING_EXPECT_EQ(
      SelectionInDOMTree::Builder()
          .Collapse(Position(GetDocument().body()->firstChild(), 0))
          .Extend(Position(GetDocument().body()->firstChild(), 2))
          .Build(),
      AsSelection("^ab|c"));
  EDITING_EXPECT_EQ(
      SelectionInDOMTree::Builder()
          .Collapse(Position(GetDocument().body()->firstChild(), 1))
          .Extend(Position(GetDocument().body()->firstChild(), 2))
          .Build(),
      AsSelection("a^b|c"));
  EDITING_EXPECT_EQ(
      SelectionInDOMTree::Builder()
          .Collapse(Position(GetDocument().body()->firstChild(), 2))
          .Build(),
      AsSelection("ab^|c"));
  EDITING_EXPECT_EQ(
      SelectionInDOMTree::Builder()
          .Collapse(Position(GetDocument().body()->firstChild(), 3))
          .Extend(Position(GetDocument().body()->firstChild(), 2))
          .Build(),
      AsSelection("ab|c^"));
}

TEST_F(SelectionSampleTest, SerializeAttribute) {
  EXPECT_EQ("<a x=\"'\" y=\"&quot;\" z=\"&amp;\">f|o^o</a>",
            AsSelectionText("<a y='\"' z=& x=\"'\">f|o^o</a>"));
}

TEST_F(SelectionSampleTest, SerializeElement) {
  EXPECT_EQ("<a>|</a>", AsSelectionText("<a>|</a>"));
  EXPECT_EQ("<a>^</a>|", AsSelectionText("<a>^</a>|"));
  EXPECT_EQ("<a>^foo</a><b>bar</b>|",
            AsSelectionText("<a>^foo</a><b>bar</b>|"));
}

TEST_F(SelectionSampleTest, SerializeEmpty) {
  EXPECT_EQ("|", AsSelectionText("|"));
  EXPECT_EQ("|", AsSelectionText("^|"));
  EXPECT_EQ("|", AsSelectionText("|^"));
}

TEST_F(SelectionSampleTest, SerializeTable) {
  // Parser moves |Text| nodes inside TABLE to before TABLE.
  EXPECT_EQ("|start^end<table><tbody><tr><td>a</td></tr></tbody></table>",
            AsSelectionText("<table>|start<tr><td>a</td></tr>^end</table>"));
  // We can use |Comment| node to put selection marker inside TABLE.
  EXPECT_EQ(
      "<table>|<tbody><tr><td>a</td></tr></tbody>^</table>",
      AsSelectionText(
          "<table><!--|--><tbody><tr><td>a</td></tr></tbody><!--^--></table>"));
  // Parser inserts TBODY auto magically.
  EXPECT_EQ(
      "<table>|<tbody><tr><td>a</td></tr>^</tbody></table>",
      AsSelectionText("<table><!--|--><tr><td>a</td></tr><!--^--></table>"));
}

TEST_F(SelectionSampleTest, SerializeText) {
  EXPECT_EQ("012^3456|789", AsSelectionText("012^3456|789"));
  EXPECT_EQ("012|3456^789", AsSelectionText("012|3456^789"));
}

}  // namespace blink
