// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <gtest/gtest.h>

#include "core/dom/Node.h"
#include "core/dom/Element.h"
#include "core/html/HTMLElement.h"
#include "core/editing/Position.h"
#include "modules/accessibility/AXObject.h"
#include "modules/accessibility/AXPosition.h"
#include "modules/accessibility/testing/AccessibilityTest.h"

namespace blink {

//
// Basic tests
//

TEST_F(AccessibilityTest, TestPositionInText) {
  SetBodyInnerHTML("<p id=\"paragraph\">Hello</p>");
  const Node* text = GetElementById("paragraph")->firstChild();
  ASSERT_NE(nullptr, text);
  const AXObject* ax_static_text = *(GetAXObjectById("paragraph")->Children().begin());
  ASSERT_NE(nullptr, ax_static_text);
  const auto ax_position = AXPosition::CreatePositionInTextObject(ax_static_text, 3);
  const auto position = ax_position.ToPosition();
  ASSERT_EQ(text, position.AnchorNode());
  ASSERT_EQ(3, position.OffsetInContainerNode());
}

// To prevent surprises when comparing equality of two |AXPosition|s, position before text object should be the same as position in text object at offset 0.
TEST_F(AccessibilityTest, TestPositionBeforeText) {
  SetBodyInnerHTML("<p id=\"paragraph\">Hello</p>");
  const Node* text = GetElementById("paragraph")->firstChild();
  ASSERT_NE(nullptr, text);
  const AXObject* ax_static_text = *(GetAXObjectById("paragraph")->Children().begin());
  ASSERT_NE(nullptr, ax_static_text);
  const auto ax_position = AXPosition::CreatePositionBeforeObject(ax_static_text);
  const auto position = ax_position.ToPosition();
  ASSERT_EQ(text, position.AnchorNode());
  ASSERT_EQ(0, position.OffsetInContainerNode());
}

// To prevent surprises when comparing equality of two |AXPosition|s, position after text object should be the same as position in text object at offset text length.
TEST_F(AccessibilityTest, TestPositionAfterText) {
  SetBodyInnerHTML("<p id=\"paragraph\">Hello</p>");
  const Node* text = GetElementById("paragraph")->firstChild();
  ASSERT_NE(nullptr, text);
  const AXObject* ax_static_text = *(GetAXObjectById("paragraph")->Children().begin());
  ASSERT_NE(nullptr, ax_static_text);
  const auto ax_position = AXPosition::CreatePositionAfterObject(ax_static_text);
  const auto position = ax_position.ToPosition();
  ASSERT_EQ(text, position.AnchorNode());
  ASSERT_EQ(5, position.OffsetInContainerNode());
}

TEST_F(AccessibilityTest, TestPositionBeforeLineBreak) {
  SetBodyInnerHTML("Hello<br id=\"br\">there");
  const AXObject* ax_br = GetAXObjectById("br");
  ASSERT_NE(nullptr, ax_br);
  const auto ax_position = AXPosition::CreatePositionBeforeObject(ax_br);
  const auto position = ax_position.ToPosition();
  ASSERT_EQ(GetDocument().body(), position.AnchorNode());
  ASSERT_EQ(1, position.OffsetInContainerNode());
}

TEST_F(AccessibilityTest, TestPositionAfterLineBreak) {
  SetBodyInnerHTML("Hello<br id=\"br\">there");
  const AXObject* ax_br = GetAXObjectById("br");
  ASSERT_NE(nullptr, ax_br);
  const auto ax_position = AXPosition::CreatePositionAfterObject(ax_br);
  const auto position = ax_position.ToPosition();
  ASSERT_EQ(GetDocument().body(), position.AnchorNode());
  ASSERT_EQ(2, position.OffsetInContainerNode());
}

TEST_F(AccessibilityTest, TestFirstPositionInContainerDiv) {
  SetBodyInnerHTML("<div id=\"div\">Hello<br>there</div>");
  const Element* div = GetElementById("div");
  ASSERT_NE(nullptr, div);
  const AXObject* ax_div = GetAXObjectById("br");
  ASSERT_NE(nullptr, ax_div);
  const auto ax_position = AXPosition::CreateFirstPositionInContainerObject(ax_div);
  const auto position = ax_position.ToPosition();
  ASSERT_EQ(div, position.AnchorNode());
  ASSERT_EQ(0, position.OffsetInContainerNode());
}

TEST_F(AccessibilityTest, TestLastPositionInContainerDiv) {
  SetBodyInnerHTML("<div id=\"div\">Hello<br>there</div>");
  const Element* div = GetElementById("div");
  ASSERT_NE(nullptr, div);
  const AXObject* ax_div = GetAXObjectById("br");
  ASSERT_NE(nullptr, ax_div);
  const auto ax_position = AXPosition::CreateLastPositionInContainerObject(ax_div);
  const auto position = ax_position.ToPosition();
  ASSERT_EQ(div, position.AnchorNode());
  ASSERT_EQ(3, position.OffsetInContainerNode());
}

TEST_F(AccessibilityTest, TestPositionFromPosition) {
}

//
// Test converting to and from visible text with white space.
// The accessibility tree is based on visible text with white space compressed, vs. the DOM tree where white space is preserved.
//

TEST_F(AccessibilityTest, TestPositionInTextWithWhiteSpace) {
  SetBodyInnerHTML("<p id=\"paragraph\">     Hello     </p>");
  const Node* text = GetElementById("paragraph")->firstChild();
  ASSERT_NE(nullptr, text);
  const AXObject* ax_static_text = *(GetAXObjectById("paragraph")->Children().begin());
  ASSERT_NE(nullptr, ax_static_text);
  const auto ax_position = AXPosition::CreatePositionInTextObject(ax_static_text, 3);
  const auto position = ax_position.ToPosition();
  ASSERT_EQ(text, position.AnchorNode());
  ASSERT_EQ(8, position.OffsetInContainerNode());
}

TEST_F(AccessibilityTest, TestPositionBeforeTextWithWhiteSpace) {
}

TEST_F(AccessibilityTest, TestPositionAfterTextWithWhiteSpace) {
}

TEST_F(AccessibilityTest, TestPositionBeforeLineBreakWithWhiteSpace) {
}

TEST_F(AccessibilityTest, TestPositionAfterLineBreakWithWhiteSpace) {
}

TEST_F(AccessibilityTest, TestFirstPositionInContainerDivWithWhiteSpace) {
}

TEST_F(AccessibilityTest, TestLastPositionInContainerDivWithWhiteSpace) {
}

TEST_F(AccessibilityTest, TestPositionFromTextPositionWithWhiteSpace) {
}

//
// Test affinity.
// We need to distinguish between the caret at the end of one line and the beginning of the next.
//

TEST_F(AccessibilityTest, TestPositionInTextWithAffinity) {
}

TEST_F(AccessibilityTest, TestPositionFromTextPositionWithAffinity) {
}

TEST_F(AccessibilityTest, TestPositionInTextWithAffinityAndWhiteSpace) {
}

TEST_F(AccessibilityTest, TestPositionFromTextPositionWithAffinityAndWhiteSpace) {
}

//
// Test converting to and from accessibility positions with offsets in labels and alt text.
// Alt text, aria-label and other ARIA relationships can cause the accessible name of an object to be different than its DOM text.
//

TEST_F(AccessibilityTest, TestPositionInHTMLLabel) {
}

TEST_F(AccessibilityTest, TestPositionInARIALabel) {
}

TEST_F(AccessibilityTest, TestPositionInARIALabelledBy) {
}

TEST_F(AccessibilityTest, TestPositionInPlaceholder) {
}

TEST_F(AccessibilityTest, TestPositionInAltText) {
}

TEST_F(AccessibilityTest, TestPositionInTitle) {
}

//
// Some objects are accessibility ignored.
//

TEST_F(AccessibilityTest, TestPositionInIgnoredObject) {
}

//
// Aria-hidden can cause things in the DOM to be hidden from accessibility.
//

TEST_F(AccessibilityTest, TestBeforePositionInARIAHidden) {
}

TEST_F(AccessibilityTest, TestAfterPositionInARIAHidden) {
}

TEST_F(AccessibilityTest, TestFromPositionInARIAHidden) {
}

//
// Canvas fallback can cause things to be in the accessibility tree that are not in the layout tree.
//

TEST_F(AccessibilityTest, TestPositionInCanvas) {
}

//
// Some layout objects, e.g. list bullets and CSS::before/after content, appears in the accessibility tree but is not present in the DOM.
//

TEST_F(AccessibilityTest, TestPositionBeforeListBullet) {
}

TEST_F(AccessibilityTest, TestPositionAfterListBullet) {
}

TEST_F(AccessibilityTest, TestPositionInCSSContent) {
}

//
// Objects deriving from |AXMockObject|, e.g. table columns, are in the accessibility tree but are neither in the DOM or layout trees.
//

TEST_F(AccessibilityTest, TestPositionInTableColumn) {
}

}  // namespace blink
