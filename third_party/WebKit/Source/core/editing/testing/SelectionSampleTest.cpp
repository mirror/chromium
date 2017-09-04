// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/editing/testing/SelectionSample.h"

#include "core/dom/ProcessingInstruction.h"
#include "core/editing/EditingTestBase.h"

namespace blink {

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
  {
    const auto& selection = AsSelection("^ab|c");
    EXPECT_EQ(SelectionInDOMTree::Builder()
                  .Collapse(Position(GetDocument().body()->firstChild(), 0))
                  .Extend(Position(GetDocument().body()->firstChild(), 2))
                  .Build(),
              selection);
  }
  {
    const auto& selection = AsSelection("a^b|c");
    EXPECT_EQ(SelectionInDOMTree::Builder()
                  .Collapse(Position(GetDocument().body()->firstChild(), 1))
                  .Extend(Position(GetDocument().body()->firstChild(), 2))
                  .Build(),
              selection);
  }
  {
    const auto& selection = AsSelection("ab^|c");
    EXPECT_EQ(SelectionInDOMTree::Builder()
                  .Collapse(Position(GetDocument().body()->firstChild(), 2))
                  .Build(),
              selection);
  }
  {
    const auto& selection = AsSelection("ab|c^");
    EXPECT_EQ(SelectionInDOMTree::Builder()
                  .Collapse(Position(GetDocument().body()->firstChild(), 3))
                  .Extend(Position(GetDocument().body()->firstChild(), 2))
                  .Build(),
              selection);
  }
}

TEST_F(SelectionSampleTest, SerializeAttribute) {
  EXPECT_EQ("<a x=\"'\" y=\"&quot;\" z=\"&amp;\">f|o^o</a>",
            AsSelectionText("<a y='\"' z=& x=\"'\">f|o^o</a>"))
      << "Attributes are alphabetically ordered.";

  EXPECT_EQ(
      "<foo:a foo:x=\"1\" x=\"2\" xmlns::foo=\"http://foo\">xy|z</foo:a>",
      AsSelectionText("<Foo:a x=2 Foo:x=1 xmlns::Foo='http://foo'>xy|z</a>"))
      << "We support namespace";
}

TEST_F(SelectionSampleTest, SerializeComment) {
  EXPECT_EQ("<!-- f|oo -->", AsSelectionText("<!-- f|oo -->"));
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

TEST_F(SelectionSampleTest, SerializeNamespace) {
  SetBodyContent("<div xmlns:foo='http://foo'><foo:bar x=1></foo:bar>");
  ContainerNode& sample = *ToContainerNode(GetDocument().body()->firstChild());
  EXPECT_EQ("<foo:bar x=\"1\"></foo:bar>",
            SelectionSample::ToString(sample, SelectionInDOMTree()))
      << "We don't insert namespace declaration.";
}

TEST_F(SelectionSampleTest, SerializeProcessingInstruction) {
  EXPECT_EQ("<!--?foo ba|r ?-->", AsSelectionText("<?foo ba|r ?>"))
      << "HTML parser turns PI into comment";
  GetDocument().body()->setTextContent("");
  GetDocument().body()->appendChild(GetDocument().createProcessingInstruction(
      "foo", "bar", ASSERT_NO_EXCEPTION));

  // Note: PI ::= '<?' PITarget (S (Char* - (Char* '?>' Char*)))? '?>'
  EXPECT_EQ("<?foo bar?>", SelectionSample::ToString(*GetDocument().body(),
                                                     SelectionInDOMTree()))
      << "No space after 'bar'";
}

TEST_F(SelectionSampleTest, SerializeTable) {
  EXPECT_EQ("|<table></table>", AsSelectionText("<table>|</table>"))
      << "Parser moves Text before TABLE.";
  EXPECT_EQ("<table>|</table>", AsSelectionText("<table><!--|--!></table>"))
      << "Parser does not inserts TBODY.";
  EXPECT_EQ("|start^end<table><tbody><tr><td>a</td></tr></tbody></table>",
            AsSelectionText("<table>|start<tr><td>a</td></tr>^end</table>"))
      << "Parser moves |Text| nodes inside TABLE to before TABLE.";
  EXPECT_EQ(
      "<table>|<tbody><tr><td>a</td></tr></tbody>^</table>",
      AsSelectionText(
          "<table><!--|--><tbody><tr><td>a</td></tr></tbody><!--^--></table>"))
      << "We can use |Comment| node to put selection marker inside TABLE.";
  EXPECT_EQ(
      "<table>|<tbody><tr><td>a</td></tr>^</tbody></table>",
      AsSelectionText("<table><!--|--><tr><td>a</td></tr><!--^--></table>"))
      << "Parser inserts TBODY auto magically.";
}

TEST_F(SelectionSampleTest, SerializeText) {
  EXPECT_EQ("012^3456|789", AsSelectionText("012^3456|789"));
  EXPECT_EQ("012|3456^789", AsSelectionText("012|3456^789"));
}

}  // namespace blink
