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
  std::string GetRange(const std::string& selection_text) {
    const PositionInFlatTree position =
        ToPositionInFlatTree(SetSelectionTextToBody(selection_text).Base());
    TextOffsetMapping mapping(position);
    return GetSelectionTextInFlatTreeFromBody(
        SelectionInFlatTree::Builder()
            .SetBaseAndExtent(mapping.range_)
            .Build());
  }

  std::string MapToFirstPosition(const std::string& selection_text,
                                 int offset) {
    const PositionInFlatTree position =
        ToPositionInFlatTree(SetSelectionTextToBody(selection_text).Base());
    TextOffsetMapping mapping(position);
    return GetSelectionTextInFlatTreeFromBody(
        SelectionInFlatTree::Builder()
            .Collapse(mapping.MapToFirstPosition(offset))
            .Build());
  }

  std::string MapToLastPosition(const std::string& selection_text, int offset) {
    const PositionInFlatTree position =
        ToPositionInFlatTree(SetSelectionTextToBody(selection_text).Base());
    TextOffsetMapping mapping(position);
    return GetSelectionTextInFlatTreeFromBody(
        SelectionInFlatTree::Builder()
            .Collapse(mapping.MapToLastPosition(offset))
            .Build());
  }
};

TEST_F(TextOffsetMappingTest, RangeOfBlockOnInlineBlock) {
  // display:inline-block doesn't introduce block.
  EXPECT_EQ("^abc<p style=\"display:inline\">def<br>ghi</p>xyz|",
            GetRange("|abc<p style=display:inline>def<br>ghi</p>xyz"));
  EXPECT_EQ("^abc<p style=\"display:inline\">def<br>ghi</p>xyz|",
            GetRange("abc<p style=display:inline>|def<br>ghi</p>xyz"));
}

TEST_F(TextOffsetMappingTest, RangeOfBlockWithAnonymousBlock) {
  // "abc" and "xyz" are in anonymous block.

  // Range is "abc"
  EXPECT_EQ("^abc|<p>def</p>xyz", GetRange("|abc<p>def</p>xyz"));
  EXPECT_EQ("^abc|<p>def</p>xyz", GetRange("a|bc<p>def</p>xyz"));

  // Range is "def"
  EXPECT_EQ("abc<p>^def|</p>xyz", GetRange("abc<p>|def</p>xyz"));
  EXPECT_EQ("abc<p>^def|</p>xyz", GetRange("abc<p>d|ef</p>xyz"));

  // Range is "xyz"
  EXPECT_EQ("abc<p>def</p>^xyz|", GetRange("abc<p>def</p>|xyz"));
  EXPECT_EQ("abc<p>def</p>^xyz|", GetRange("abc<p>def</p>xyz|"));
}

TEST_F(TextOffsetMappingTest, RangeOfBlockWithBR) {
  EXPECT_EQ("^abc<br>xyz|", GetRange("abc|<br>xyz"))
      << "BR doesn't affect block";
}

TEST_F(TextOffsetMappingTest, RangeOfBlockWithPRE) {
  // "\n" doesn't affect block.
  EXPECT_EQ("<pre>^abc\ndef\nghi\n|</pre>",
            GetRange("<pre>|abc\ndef\nghi\n</pre>"));
  EXPECT_EQ("<pre>^abc\ndef\nghi\n|</pre>",
            GetRange("<pre>abc\n|def\nghi\n</pre>"));
  EXPECT_EQ("<pre>^abc\ndef\nghi\n|</pre>",
            GetRange("<pre>abc\ndef\n|ghi\n</pre>"));
  EXPECT_EQ("<pre>^abc\ndef\nghi\n|</pre>",
            GetRange("<pre>abc\ndef\nghi\n|</pre>"));
}

TEST_F(TextOffsetMappingTest, RangeOfBlockWithRUBY) {
  EXPECT_EQ("<ruby>^abc|<rt>123</rt></ruby>",
            GetRange("<ruby>|abc<rt>123</rt></ruby>"));
  EXPECT_EQ("<ruby>abc<rt>^123|</rt></ruby>",
            GetRange("<ruby>abc<rt>1|23</rt></ruby>"));
}

TEST_F(TextOffsetMappingTest, RangeOfBlockWithRUBYandBR) {
  EXPECT_EQ("<ruby>^abc<br>def|<rt>123<br>456</rt></ruby>",
            GetRange("<ruby>|abc<br>def<rt>123<br>456</rt></ruby>"))
      << "RT(LayoutRubyRun) is a block";
  EXPECT_EQ("<ruby>abc<br>def<rt>^123<br>456|</rt></ruby>",
            GetRange("<ruby>abc<br>def<rt>123|<br>456</rt></ruby>"))
      << "RUBY introduce LayoutRuleBase for 'abc'";
}

TEST_F(TextOffsetMappingTest, RangeOfBlockWithTABLE) {
  EXPECT_EQ("^abc|<table><tbody><tr><td>one</td></tr></tbody></table>xyz",
            GetRange("|abc<table><tr><td>one</td></tr></table>xyz"))
      << "Before TABLE";
  EXPECT_EQ("abc<table><tbody><tr><td>^one|</td></tr></tbody></table>xyz",
            GetRange("abc<table><tr><td>o|ne</td></tr></table>xyz"))
      << "In TD";
  EXPECT_EQ("abc<table><tbody><tr><td>one</td></tr></tbody></table>^xyz|",
            GetRange("abc<table><tr><td>one</td></tr></table>x|yz"))
      << "After TABLE";
}

TEST_F(TextOffsetMappingTest, MapToFirstPosition) {
  EXPECT_EQ("  |012  456  ", MapToFirstPosition("|  012  456  ", 0));
  EXPECT_EQ("  0|12  456  ", MapToFirstPosition("|  012  456  ", 1));
  EXPECT_EQ("  01|2  456  ", MapToFirstPosition("|  012  456  ", 2));
  EXPECT_EQ("  012|  456  ", MapToFirstPosition("|  012  456  ", 3));
  EXPECT_EQ("  012  |456  ", MapToFirstPosition("|  012  456  ", 4));
  EXPECT_EQ("  012  4|56  ", MapToFirstPosition("|  012  456  ", 5));
  EXPECT_EQ("  012  45|6  ", MapToFirstPosition("|  012  456  ", 6));
  EXPECT_EQ("  012  456|  ", MapToFirstPosition("|  012  456  ", 7));
  // We hit DCHECK for offset 8, because we walk on "012 456".
}

TEST_F(TextOffsetMappingTest, MapToLastPosition) {
  EXPECT_EQ("  0|12  456  ", MapToLastPosition("|  012  456  ", 0));
  EXPECT_EQ("  01|2  456  ", MapToLastPosition("|  012  456  ", 1));
  EXPECT_EQ("  012|  456  ", MapToLastPosition("|  012  456  ", 2));
  EXPECT_EQ("  012 | 456  ", MapToLastPosition("|  012  456  ", 3));
  EXPECT_EQ("  012  4|56  ", MapToLastPosition("|  012  456  ", 4));
  EXPECT_EQ("  012  45|6  ", MapToLastPosition("|  012  456  ", 5));
  EXPECT_EQ("  012  456|  ", MapToLastPosition("|  012  456  ", 6));
  EXPECT_EQ("  012  456|  ", MapToLastPosition("|  012  456  ", 7));
  // We hit DCHECK for offset 8, because we walk on "012 456".
}

}  // namespace blink
