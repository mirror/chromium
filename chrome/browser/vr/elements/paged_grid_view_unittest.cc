// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <vector>

#include "base/memory/ptr_util.h"
#include "cc/test/geometry_test_utils.h"
#include "chrome/browser/vr/elements/paged_grid_view.h"

#include "testing/gtest/include/gtest/gtest.h"

namespace vr {

typedef std::vector<std::unique_ptr<UiElement>> ElementVector;

namespace {

static constexpr float kElementWidth = 20.0f;
static constexpr float kElementHeight = 10.0f;

void AddChildren(PagedGridView* view, ElementVector* elements, size_t count) {
  for (size_t i = 0; i < count; ++i) {
    std::unique_ptr<UiElement> child = base::MakeUnique<UiElement>();
    child->SetSize(kElementWidth, kElementHeight);
    child->SetVisible(true);
    view->AddChild(child.get());
    elements->push_back(std::move(child));
  }
}

gfx::Point3F GetLayoutPosition(const UiElement& elements) {
  gfx::Point3F position;
  elements.LocalTransform().TransformPoint(&position);
  return position;
}

}  // namespace

TEST(PagedGridView, NoElements) {
  PagedGridView view(4lu, 4lu, 0.05f);
  view.LayOutChildren();
  EXPECT_EQ(0lu, view.NumPages());
  EXPECT_EQ(0lu, view.CurrentPage());
}

TEST(PagedGridView, SinglePage) {
  float margin = 0.5f;
  ElementVector elements;
  PagedGridView view(2lu, 1lu, margin);

  // Should add exactly one page's worth of elements.
  AddChildren(&view, &elements, 2lu);

  view.LayOutChildren();
  EXPECT_EQ(1lu, view.NumPages());
  EXPECT_EQ(0lu, view.CurrentPage());

  EXPECT_POINT3F_EQ(gfx::Point3F(0.0f, 0.5f * (kElementHeight + margin), 0.0f),
                    GetLayoutPosition(*elements[0]));
  EXPECT_POINT3F_EQ(gfx::Point3F(0.0f, -0.5f * (kElementHeight + margin), 0.0f),
                    GetLayoutPosition(*elements[1]));
}

TEST(PagedGridView, UnfilledPage) {
  float margin = 0.5f;
  ElementVector elements;
  PagedGridView view(2lu, 1lu, margin);

  // Should add exactly one page's worth of elements.
  AddChildren(&view, &elements, 1lu);

  view.LayOutChildren();
  EXPECT_EQ(1lu, view.NumPages());
  EXPECT_EQ(0lu, view.CurrentPage());

  // This test is very much like SinglePage but we've only added a single
  // UiElement rather than two. The grid should not attempt to center the
  // elements in the page, so the omission of the second element should have no
  // effect on the positioning of the first.
  EXPECT_POINT3F_EQ(gfx::Point3F(0.0f, 0.5f * (kElementHeight + margin), 0.0f),
                    GetLayoutPosition(*elements[0]));
}

TEST(PagedGridView, MultiplePages) {
  float margin = 0.5f;
  ElementVector elements;
  PagedGridView view(2lu, 1lu, margin);

  // Should add exactly one page's worth of elements.
  AddChildren(&view, &elements, 3lu);

  view.LayOutChildren();
  EXPECT_EQ(2lu, view.NumPages());
  EXPECT_EQ(0lu, view.CurrentPage());

  // This test is very much like SinglePage but we've added three UiElement
  // instances rather than two. The addition of the third element should have no
  // effect on the positioning of the first two, but it should cause the
  // addition of another page.
  EXPECT_POINT3F_EQ(gfx::Point3F(0.0f, 0.5f * (kElementHeight + margin), 0.0f),
                    GetLayoutPosition(*elements[0]));
  EXPECT_POINT3F_EQ(gfx::Point3F(0.0f, -0.5f * (kElementHeight + margin), 0.0f),
                    GetLayoutPosition(*elements[1]));
  EXPECT_POINT3F_EQ(gfx::Point3F(kElementWidth + margin,
                                 0.5f * (kElementHeight + margin), 0.0f),
                    GetLayoutPosition(*elements[2]));

  // Setting the current page should have no effect on the local positioning of
  // the child elements. Rather, it should have an impact on the inheritable
  // transform provided by the parent. I.e., it will adjust the transform of
  // |view|.
  view.SetCurrentPage(1lu);
  view.LayOutChildren();
  EXPECT_POINT3F_EQ(gfx::Point3F(0.0f, 0.5f * (kElementHeight + margin), 0.0f),
                    GetLayoutPosition(*elements[0]));
  EXPECT_POINT3F_EQ(gfx::Point3F(0.0f, -0.5f * (kElementHeight + margin), 0.0f),
                    GetLayoutPosition(*elements[1]));
  EXPECT_POINT3F_EQ(gfx::Point3F(kElementWidth + margin,
                                 0.5f * (kElementHeight + margin), 0.0f),
                    GetLayoutPosition(*elements[2]));
  EXPECT_POINT3F_EQ(gfx::Point3F(-kElementWidth - margin, 0.0f, 0.001f),
                    GetLayoutPosition(view));

  // If we remove an element from the layout, we should reduce the number of
  // pages back to one. This will cause the current page to become invalid. We
  // should see the grid view update the current page and related transform in
  // response. Again, this should have no impact on the laid out position of the
  // children.
  view.RemoveChild(elements.back().get());
  view.LayOutChildren();

  EXPECT_EQ(1lu, view.NumPages());
  EXPECT_EQ(0lu, view.CurrentPage());
  EXPECT_POINT3F_EQ(gfx::Point3F(0.0f, 0.5f * (kElementHeight + margin), 0.0f),
                    GetLayoutPosition(*elements[0]));
  EXPECT_POINT3F_EQ(gfx::Point3F(0.0f, -0.5f * (kElementHeight + margin), 0.0f),
                    GetLayoutPosition(*elements[1]));
  EXPECT_POINT3F_EQ(gfx::Point3F(0.0f, 0.0f, 0.001f), GetLayoutPosition(view));
}

TEST(PagedGridView, IllegalSetCurrentPage) {
  PagedGridView view(4lu, 4lu, 0.05f);
  ElementVector elements;

  // Should add exactly one page's worth of elements.
  AddChildren(&view, &elements, 16lu);

  view.LayOutChildren();
  EXPECT_EQ(1lu, view.NumPages());
  EXPECT_EQ(0lu, view.CurrentPage());

  view.SetCurrentPage(2lu);
  EXPECT_EQ(0lu, view.CurrentPage());
}

TEST(PagedGridView, LayoutOrder) {
  float margin = 0.5f;
  PagedGridView view(2lu, 2lu, margin);
  ElementVector elements;

  // Should add exactly one page's worth of elements.
  AddChildren(&view, &elements, 8lu);

  view.LayOutChildren();
  EXPECT_EQ(2lu, view.NumPages());
  EXPECT_EQ(0lu, view.CurrentPage());

  EXPECT_POINT3F_EQ(gfx::Point3F(0.5f * (kElementWidth + margin),
                                 0.5f * (kElementHeight + margin), 0.0f),
                    GetLayoutPosition(*elements[1]));
  EXPECT_POINT3F_EQ(gfx::Point3F(1.5f * (kElementWidth + margin),
                                 -0.5f * (kElementHeight + margin), 0.0f),
                    GetLayoutPosition(*elements[6]));
}

}  // namespace vr
