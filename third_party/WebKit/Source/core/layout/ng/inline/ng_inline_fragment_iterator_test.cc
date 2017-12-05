// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/layout/ng/inline/ng_inline_fragment_iterator.h"

#include "core/layout/ng/inline/ng_physical_text_fragment.h"
#include "core/layout/ng/ng_layout_test.h"

namespace blink {

class NGInlineFragmentIteratorTest
    : public NGLayoutTest,
      private ScopedLayoutNGPaintFragmentsForTest {
 public:
  NGInlineFragmentIteratorTest()
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

#define EXPECT_NEXT_BOX(iter)               \
  {                                         \
    const auto& current = *iter++;          \
    EXPECT_TRUE(current.fragment->IsBox()); \
  }

#define EXPECT_NEXT_LINE_BOX(iter)              \
  {                                             \
    const auto& current = *iter++;              \
    EXPECT_TRUE(current.fragment->IsLineBox()); \
  }

#define EXPECT_NEXT_TEXT(iter, content)                                     \
  {                                                                         \
    const auto& current = *iter++;                                          \
    EXPECT_TRUE(current.fragment->IsText());                                \
    EXPECT_EQ(content, ToNGPhysicalTextFragment(current.fragment)->Text()); \
  }

TEST_F(NGInlineFragmentIteratorTest, IteratorAll) {
  SetBodyInnerHTML("<div id=t>foo<b>bar</b><br>baz</div>");
  NGInlineFragmentIterator descendants(GetRootFragmentById("t"));
  auto iter = descendants.begin();

  EXPECT_NEXT_LINE_BOX(iter);
  EXPECT_NEXT_TEXT(iter, "foo");
  EXPECT_NEXT_BOX(iter);
  EXPECT_NEXT_TEXT(iter, "bar");
  EXPECT_NEXT_TEXT(iter, "\n");
  EXPECT_NEXT_LINE_BOX(iter);
  EXPECT_NEXT_TEXT(iter, "baz");
  EXPECT_EQ(iter, descendants.end());
}

TEST_F(NGInlineFragmentIteratorTest, IteratorWithFilter) {
  SetBodyInnerHTML("<div id=t>foo<b id=filter>bar<br>baz</b>bla</div>");
  NGInlineFragmentIterator descendants(GetRootFragmentById("t"),
                                       GetLayoutObjectByElementId("filter"));
  auto iter = descendants.begin();

  // <b> generates two boxes since its content is in two lines.
  EXPECT_NEXT_BOX(iter);
  EXPECT_NEXT_BOX(iter);
  EXPECT_EQ(iter, descendants.end());
}

}  // namespace blink
