// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/page/SpatialNavigation.h"

#include "core/layout/LayoutTestHelper.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace blink {

class SpatialNavigationTest : public RenderingTest {
 public:
  SpatialNavigationTest()
      : RenderingTest(SingleChildLocalFrameClient::Create()) {}
};

TEST_F(SpatialNavigationTest, FindContainerWhenEnclosingContainerIsDocument) {
  SetBodyInnerHTML(
      "<!DOCTYPE html>"
      "<div id='child'></div>");

  Element* child_element = GetDocument().getElementById("child");

  Node* enclosing_container =
      ScrollableEnclosingBoxOrParentFrameForNode(child_element);

  EXPECT_EQ(enclosing_container, GetDocument());
}

TEST_F(SpatialNavigationTest, FindContainerWhenEnclosingContainerIsIframe) {
  SetBodyInnerHTML(
      "<!DOCTYPE html>"
      "<style>"
      "  iframe {"
      "    width: 100px;"
      "    height: 100px;"
      "  }"
      "</style>"
      "<iframe id='iframe'></iframe>");

  SetChildFrameHTML(
      "<!DOCTYPE html>"
      "<style>"
      "  #child {"
      "    height: 1000px;"
      "  }"
      "</style>"
      "<div id='child'></div>");
  ChildDocument().View()->UpdateAllLifecyclePhases();

  Element* child_element = ChildDocument().getElementById("child");

  Node* enclosing_container =
      ScrollableEnclosingBoxOrParentFrameForNode(child_element);

  EXPECT_EQ(enclosing_container, ChildDocument());
}

TEST_F(SpatialNavigationTest,
       FindContainerWhenEnclosingContainerIsScrollableOverflowBox) {
  SetBodyInnerHTML(
      "<!DOCTYPE html>"
      "<style>"
      "  #content {"
      "    padding-top: 600px;"
      "  }"
      "  #container {"
      "    height: 100px;"
      "    overflow-y: scroll;"
      "  }"
      "</style>"
      "<div id='container'>"
      "  <div id='content'>some text here</div>"
      "</div>");

  Element* content_element = GetDocument().getElementById("content");
  Element* container_element = GetDocument().getElementById("container");

  Node* enclosing_container =
      ScrollableEnclosingBoxOrParentFrameForNode(content_element);
  EXPECT_EQ(enclosing_container, container_element);
}

}  // namespace blink
