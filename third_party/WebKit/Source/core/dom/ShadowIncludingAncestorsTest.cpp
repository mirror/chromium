// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/dom/ShadowIncludingAncestors.h"

#include "core/html/HTMLDocument.h"
#include "core/html/HTMLHtmlElement.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace blink {

class ShadowIncludingAncestorsTest : public ::testing::Test {
 public:
  ShadowIncludingAncestorsTest() {}
  Document& GetDocument() { return *document_.Get(); }
  void SetInnerHTML(const String& html_content) {
    document_->documentElement()->SetInnerHTMLFromString(html_content);
  }

 private:
  void SetUp() override {
    document_ = HTMLDocument::CreateForTest();
    HTMLHtmlElement* html = HTMLHtmlElement::Create(*document_);
    document_->AppendChild(html);
  }

  Persistent<Document> document_;
};

TEST_F(ShadowIncludingAncestorsTest, Reduce) {
  SetInnerHTML(
      "<body>"
      "  <div id=d1>"
      "    <div id=d1_1></div>"
      "    <div id=d1_2></div>"
      "  </div>"
      "  <div id=d2>"
      "    <div id=d2_1></div>"
      "    <div id=d2_2></div>"
      "  </div>"
      "</body>");

  ShadowIncludingAncestors ancestors;

  ancestors.Reduce(GetDocument().getElementById("d1_1"));
  EXPECT_EQ(ancestors.LastNode(), GetDocument().getElementById("d1_1"));

  ancestors.Reduce(GetDocument().body());
  EXPECT_EQ(ancestors.LastNode(), GetDocument().body());

  ancestors.Reduce(GetDocument().documentElement());
  EXPECT_EQ(ancestors.LastNode(), GetDocument().documentElement());

  ancestors.Clear();
  EXPECT_FALSE(ancestors.LastNode());

  ancestors.Reduce(GetDocument().getElementById("d2_2"));
  EXPECT_EQ(ancestors.LastNode(), GetDocument().getElementById("d2_2"));

  ancestors.Reduce(GetDocument().getElementById("d2"));
  EXPECT_EQ(ancestors.LastNode(), GetDocument().getElementById("d2"));

  ancestors.Reduce(GetDocument().body());
  EXPECT_EQ(ancestors.LastNode(), GetDocument().body());
}

}  // namespace blink
