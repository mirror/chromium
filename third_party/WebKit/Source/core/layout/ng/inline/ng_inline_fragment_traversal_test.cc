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

  const NGPhysicalFragment& GetFragmentOfNode(
      const NGPhysicalContainerFragment& container,
      const Node* node) const {
    const LayoutObject* layout_object = node->GetLayoutObject();
    auto fragments =
        NGInlineFragmentTraversal::SelfFragmentsOf(container, layout_object);
    return *fragments.front().fragment;
  }
};

#define EXPECT_NEXT_BOX(iter, id)                                           \
  {                                                                         \
    const auto& current = *iter++;                                          \
    EXPECT_TRUE(current.fragment->IsBox()) << current.fragment->ToString(); \
    EXPECT_EQ(GetLayoutObjectByElementId(id),                               \
              current.fragment->GetLayoutObject());                         \
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
  SetBodyInnerHTML(
      "<style>* { border: 1px solid}</style>"
      "<div id=t>foo<b id=b>bar</b><br>baz</div>");
  const auto descendants =
      NGInlineFragmentTraversal::DescendantsOf(GetRootFragmentById("t"));
  auto iter = descendants.begin();

  EXPECT_NEXT_LINE_BOX(iter);
  EXPECT_NEXT_TEXT(iter, "foo");
  EXPECT_NEXT_BOX(iter, "b");
  EXPECT_NEXT_TEXT(iter, "bar");
  EXPECT_NEXT_TEXT(iter, "\n");
  EXPECT_NEXT_LINE_BOX(iter);
  EXPECT_NEXT_TEXT(iter, "baz");
  EXPECT_EQ(iter, descendants.end());
}

TEST_F(NGInlineFragmentTraversalTest, InclusiveDescendantsOf) {
  SetBodyInnerHTML(
      "<style>* { border: 1px solid}</style>"
      "<div id=t>foo<b id=b>bar</b><br>baz</div>");
  auto descendants = NGInlineFragmentTraversal::InclusiveDescendantsOf(
      GetRootFragmentById("t"));
  auto iter = descendants.begin();

  EXPECT_NEXT_BOX(iter, "t");
  EXPECT_NEXT_LINE_BOX(iter);
  EXPECT_NEXT_TEXT(iter, "foo");
  EXPECT_NEXT_BOX(iter, "b");
  EXPECT_NEXT_TEXT(iter, "bar");
  EXPECT_NEXT_TEXT(iter, "\n");
  EXPECT_NEXT_LINE_BOX(iter);
  EXPECT_NEXT_TEXT(iter, "baz");
  EXPECT_EQ(iter, descendants.end());
}

TEST_F(NGInlineFragmentTraversalTest, SelfFragmentsOf) {
  SetBodyInnerHTML(
      "<style>* { border: 1px solid}</style>"
      "<div id=t>foo<b id=filter>bar<br>baz</b>bla</div>");
  const auto descendants = NGInlineFragmentTraversal::SelfFragmentsOf(
      GetRootFragmentById("t"), GetLayoutObjectByElementId("filter"));
  auto iter = descendants.begin();

  // <b> generates two box fragments since its content is in two lines.
  EXPECT_NEXT_BOX(iter, "filter");
  EXPECT_NEXT_BOX(iter, "filter");
  EXPECT_EQ(iter, descendants.end());
}

TEST_F(NGInlineFragmentTraversalTest, AncestorsOf) {
  SetBodyInnerHTML(
      "<style>* { border: 1px solid}</style>"
      "<div id=t>x"
      "<b id=b>y<i id=i>z<u id=target>foo</u>z</i>y</b>"
      "x</div>");
  const NGPhysicalContainerFragment& root = GetRootFragmentById("t");
  const NGPhysicalFragment& target =
      GetFragmentOfNode(root, GetElementById("target")->firstChild());
  auto ancestors = NGInlineFragmentTraversal::AncestorsOf(root, target);
  auto iter = ancestors.begin();

  EXPECT_NEXT_BOX(iter, "target");
  EXPECT_NEXT_BOX(iter, "i");
  EXPECT_NEXT_BOX(iter, "b");
  EXPECT_NEXT_LINE_BOX(iter);
  EXPECT_EQ(iter, ancestors.end());
}

TEST_F(NGInlineFragmentTraversalTest, InclusiveAncestorsOf) {
  SetBodyInnerHTML(
      "<style>* { border: 1px solid}</style>"
      "<div id=t>x"
      "<b id=b>y<i id=i>z<u id=target>foo</u>z</i>y</b>"
      "x</div>");
  const NGPhysicalContainerFragment& root = GetRootFragmentById("t");
  const NGPhysicalFragment& target =
      GetFragmentOfNode(root, GetElementById("target")->firstChild());
  auto ancestors =
      NGInlineFragmentTraversal::InclusiveAncestorsOf(root, target);
  auto iter = ancestors.begin();

  EXPECT_NEXT_TEXT(iter, "foo");
  EXPECT_NEXT_BOX(iter, "target");
  EXPECT_NEXT_BOX(iter, "i");
  EXPECT_NEXT_BOX(iter, "b");
  EXPECT_NEXT_LINE_BOX(iter);
  EXPECT_EQ(iter, ancestors.end());
}

#define EXPECT_NEXT_BOX_OF_TYPE(iter, id, box_type, is_old_layout_root)     \
  {                                                                         \
    const auto& current = *iter++;                                          \
    EXPECT_TRUE(current.fragment->IsBox()) << current.fragment->ToString(); \
    EXPECT_EQ(GetLayoutObjectByElementId(id),                               \
              current.fragment->GetLayoutObject());                         \
    EXPECT_EQ(current.fragment->BoxType(), NGPhysicalFragment::box_type);   \
    EXPECT_EQ(current.fragment->IsOldLayoutRoot(), is_old_layout_root);     \
  }

TEST_F(NGInlineFragmentTraversalTest, MixedLayoutTree) {
  // TODO(layout-dev): Design more straightforward way to ensure old layout
  // instead of using |contenteditable|.
  SetBodyInnerHTML(
      "<div id=container style='position: absolute'>"
      "1"
      "<span contenteditable id=float-old style='float:left'>X</span>"
      "<span id=float-new style='float:left'>X</span>"
      "2"
      "<span contenteditable id=atomic-inline-old "
      "style='display:inline-block'>Y</span>"
      "<span id=atomic-inline-new style='display:inline-block'>Y</span>"
      "3"
      "<span contenteditable id=oof-old style='position: absolute'>Z</span>"
      "<span id=oof-new style='position: absolute'>Z</span>"
      "4"
      "</div>");
  auto descendants = NGInlineFragmentTraversal::DescendantsOf(
      GetRootFragmentById("container"));
  auto iter = descendants.begin();

  const bool is_old_layout = true;
  const bool is_new_layout = false;
  EXPECT_NEXT_BOX_OF_TYPE(iter, "float-old", kFloating, is_old_layout);
  EXPECT_NEXT_BOX_OF_TYPE(iter, "float-new", kFloating, is_new_layout);
  EXPECT_NEXT_LINE_BOX(iter);
  EXPECT_NEXT_TEXT(iter, "1");
  EXPECT_NEXT_TEXT(iter, "2");
  EXPECT_NEXT_BOX_OF_TYPE(iter, "atomic-inline-old", kInlineBlock,
                          is_old_layout);
  EXPECT_NEXT_BOX_OF_TYPE(iter, "atomic-inline-new", kInlineBlock,
                          is_new_layout);
  EXPECT_NEXT_TEXT(iter, "3");
  EXPECT_NEXT_TEXT(iter, "4");
  EXPECT_NEXT_BOX_OF_TYPE(iter, "oof-old", kOutOfFlowPositioned, is_old_layout);
  EXPECT_NEXT_BOX_OF_TYPE(iter, "oof-new", kOutOfFlowPositioned, is_new_layout);
  EXPECT_EQ(iter, descendants.end());
}

}  // namespace blink
