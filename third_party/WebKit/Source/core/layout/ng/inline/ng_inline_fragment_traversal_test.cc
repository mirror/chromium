// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/layout/ng/inline/ng_inline_fragment_iterator.h"

#include "core/layout/ng/inline/ng_physical_text_fragment.h"
#include "core/layout/ng/ng_layout_test.h"
#include "core/layout/ng/ng_physical_box_fragment.h"

namespace blink {

class NGInlineFragmentTraversalTest
    : public NGLayoutTest,
      private ScopedLayoutNGPaintFragmentsForTest {
 public:
  NGInlineFragmentTraversalTest()
      : NGLayoutTest(), ScopedLayoutNGPaintFragmentsForTest(true) {}

 protected:
  const NGPhysicalBoxFragment& GetRootFragmentById(const char* id) const {
    const Element* element = GetElementById(id);
    DCHECK(element) << id;
    const LayoutObject* layout_object = element->GetLayoutObject();
    DCHECK(layout_object) << element;
    DCHECK(layout_object->IsLayoutBlockFlow()) << element;
    DCHECK(ToLayoutBlockFlow(layout_object)->CurrentFragment()) << element;
    return *ToLayoutBlockFlow(layout_object)->CurrentFragment();
  }
};

#define EXPECT_NEXT_BOX(iter, layout_object)                                \
  {                                                                         \
    const auto& current = *iter++;                                          \
    EXPECT_TRUE(current.fragment->IsBox()) << current.fragment->ToString(); \
    EXPECT_EQ(layout_object, current.fragment->GetLayoutObject());          \
  }

#define EXPECT_NEXT_LINE_BOX(iter)             \
  {                                            \
    const auto& current = *iter++;             \
    EXPECT_TRUE(current.fragment->IsLineBox()) \
        << current.fragment->ToString();       \
  }

#define EXPECT_NEXT_TEXT(iter, content)                                      \
  {                                                                          \
    const auto& current = *iter++;                                           \
    EXPECT_TRUE(current.fragment->IsText()) << current.fragment->ToString(); \
    EXPECT_EQ(content, ToNGPhysicalTextFragment(current.fragment)->Text());  \
  }

TEST_F(NGInlineFragmentTraversalTest, DescendantsOf) {
  SetBodyInnerHTML("<div id=t>foo<b id=b>bar</b><br>baz</div>");
  const auto descendants =
      NGInlineFragmentTraversal::DescendantsOf(GetRootFragmentById("t"));
  auto iter = descendants.begin();

  EXPECT_NEXT_LINE_BOX(iter);
  EXPECT_NEXT_TEXT(iter, "foo");
  EXPECT_NEXT_BOX(iter, GetLayoutObjectByElementId("b"));
  EXPECT_NEXT_TEXT(iter, "bar");
  EXPECT_NEXT_TEXT(iter, "\n");
  EXPECT_NEXT_LINE_BOX(iter);
  EXPECT_NEXT_TEXT(iter, "baz");
  EXPECT_EQ(iter, descendants.end());
}

TEST_F(NGInlineFragmentTraversalTest, InclusiveDescendantsOf) {
  SetBodyInnerHTML("<div id=t>foo<b id=b>bar</b><br>baz</div>");
  auto descendants = NGInlineFragmentTraversal::InclusiveDescendantsOf(
      GetRootFragmentById("t"));
  auto iter = descendants.begin();

  EXPECT_NEXT_BOX(iter, GetLayoutObjectByElementId("t"));
  EXPECT_NEXT_LINE_BOX(iter);
  EXPECT_NEXT_TEXT(iter, "foo");
  EXPECT_NEXT_BOX(iter, GetLayoutObjectByElementId("b"));
  EXPECT_NEXT_TEXT(iter, "bar");
  EXPECT_NEXT_TEXT(iter, "\n");
  EXPECT_NEXT_LINE_BOX(iter);
  EXPECT_NEXT_TEXT(iter, "baz");
  EXPECT_EQ(iter, descendants.end());
}

TEST_F(NGInlineFragmentTraversalTest, SelfFragmentsOf) {
  SetBodyInnerHTML("<div id=t>foo<b id=filter>bar<br>baz</b>bla</div>");
  const auto descendants = NGInlineFragmentTraversal::SelfFragmentsOf(
      GetRootFragmentById("t"), GetLayoutObjectByElementId("filter"));
  auto iter = descendants.begin();

  // <b> generates two box fragments since its content is in two lines.
  EXPECT_NEXT_BOX(iter, GetLayoutObjectByElementId("filter"));
  EXPECT_NEXT_BOX(iter, GetLayoutObjectByElementId("filter"));
  EXPECT_EQ(iter, descendants.end());
}

TEST_F(NGInlineFragmentTraversalTest, AncestorsOf) {
  SetBodyInnerHTML(
      "<div id=t>x"
      "<b id=b>y<i id=i>z<u id=target>foo</u>z</i>y</b>"
      "x</div>");
  const NGPhysicalContainerFragment& root = GetRootFragmentById("t");
  const NGPhysicalFragment& target =
      *NGInlineFragmentTraversal::SelfFragmentsOf(
           root, GetLayoutObjectByElementId("target")->SlowFirstChild())
           .front()
           .fragment;
  auto ancestors = NGInlineFragmentTraversal::AncestorsOf(root, target);
  auto iter = ancestors.begin();

  EXPECT_NEXT_BOX(iter, GetLayoutObjectByElementId("target"));
  EXPECT_NEXT_BOX(iter, GetLayoutObjectByElementId("i"));
  EXPECT_NEXT_BOX(iter, GetLayoutObjectByElementId("b"));
  EXPECT_NEXT_LINE_BOX(iter);
  EXPECT_EQ(iter, ancestors.end());
}

TEST_F(NGInlineFragmentTraversalTest, InclusiveAncestorsOf) {
  SetBodyInnerHTML(
      "<div id=t>x"
      "<b id=b>y<i id=i>z<u id=target>foo</u>z</i>y</b>"
      "x</div>");
  const NGPhysicalContainerFragment& root = GetRootFragmentById("t");
  const NGPhysicalFragment& target =
      *NGInlineFragmentTraversal::SelfFragmentsOf(
           root, GetLayoutObjectByElementId("target")->SlowFirstChild())
           .front()
           .fragment;
  auto ancestors =
      NGInlineFragmentTraversal::InclusiveAncestorsOf(root, target);
  auto iter = ancestors.begin();

  EXPECT_NEXT_TEXT(iter, "foo");
  EXPECT_NEXT_BOX(iter, GetLayoutObjectByElementId("target"));
  EXPECT_NEXT_BOX(iter, GetLayoutObjectByElementId("i"));
  EXPECT_NEXT_BOX(iter, GetLayoutObjectByElementId("b"));
  EXPECT_NEXT_LINE_BOX(iter);
  EXPECT_EQ(iter, ancestors.end());
}

}  // namespace blink
