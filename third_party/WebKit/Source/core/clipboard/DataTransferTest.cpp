// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/clipboard/DataTransfer.h"

#include "core/frame/LocalFrame.h"
#include "core/frame/LocalFrameView.h"
#include "core/frame/PerformanceMonitor.h"
#include "core/frame/Settings.h"
#include "core/layout/LayoutObject.h"
#include "core/testing/DummyPageHolder.h"
#include "core/timing/Performance.h"
#include "platform/DragImage.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace blink {

class DataTransferTest : public ::testing::Test {
 protected:
  DataTransferTest() = default;
  ~DataTransferTest() override = default;

  Document& GetDocument() const { return dummy_page_holder_->GetDocument(); }
  Page& GetPage() const { return dummy_page_holder_->GetPage(); }
  LocalFrame& GetFrame() const { return *GetDocument().GetFrame(); }
  Performance* GetPerformance() const { return performance_; }

  void SetBodyContent(const std::string& body_content) {
    GetDocument().body()->setInnerHTML(String::FromUTF8(body_content.c_str()));
    UpdateAllLifecyclePhases();
  }

  void UpdateAllLifecyclePhases() {
    GetDocument().View()->UpdateAllLifecyclePhases();
  }

 private:
  void SetUp() override {
    dummy_page_holder_ = DummyPageHolder::Create(IntSize(800, 600));
    performance_ = Performance::Create(&GetFrame());
  }

  std::unique_ptr<DummyPageHolder> dummy_page_holder_;
  Persistent<Performance> performance_;
};

TEST_F(DataTransferTest, nodeImage) {
  SetBodyContent(
      "<style>"
      "#sample { width: 100px; height: 100px; }"
      "</style>"
      "<div id=sample></div>");
  Element* sample = GetDocument().getElementById("sample");
  const std::unique_ptr<DragImage> image =
      DataTransfer::NodeImage(GetFrame(), *sample);
  EXPECT_EQ(IntSize(100, 100), image->Size());
}

TEST_F(DataTransferTest, nodeImageWithNestedElement) {
  SetBodyContent(
      "<style>"
      "div { -webkit-user-drag: element }"
      "span:-webkit-drag { color: #0F0 }"
      "</style>"
      "<div id=sample><span>Green when dragged</span></div>");
  Element* sample = GetDocument().getElementById("sample");
  const std::unique_ptr<DragImage> image =
      DataTransfer::NodeImage(GetFrame(), *sample);
  EXPECT_EQ(
      Color(0, 255, 0),
      sample->firstChild()->GetLayoutObject()->ResolveColor(CSSPropertyColor))
      << "Descendants node should have :-webkit-drag.";
}

TEST_F(DataTransferTest, nodeImageWithPsuedoClassWebKitDrag) {
  SetBodyContent(
      "<style>"
      "#sample { width: 100px; height: 100px; }"
      "#sample:-webkit-drag { width: 200px; height: 200px; }"
      "</style>"
      "<div id=sample></div>");
  Element* sample = GetDocument().getElementById("sample");
  const std::unique_ptr<DragImage> image =
      DataTransfer::NodeImage(GetFrame(), *sample);
  EXPECT_EQ(IntSize(200, 200), image->Size())
      << ":-webkit-drag should affect dragged image.";
}

TEST_F(DataTransferTest, nodeImageWithoutDraggedLayoutObject) {
  SetBodyContent(
      "<style>"
      "#sample { width: 100px; height: 100px; }"
      "#sample:-webkit-drag { display:none }"
      "</style>"
      "<div id=sample></div>");
  Element* sample = GetDocument().getElementById("sample");
  const std::unique_ptr<DragImage> image =
      DataTransfer::NodeImage(GetFrame(), *sample);
  EXPECT_EQ(nullptr, image.get()) << ":-webkit-drag blows away layout object";
}

TEST_F(DataTransferTest, nodeImageWithChangingLayoutObject) {
  SetBodyContent(
      "<style>"
      "#sample { color: blue; }"
      "#sample:-webkit-drag { display: inline-block; color: red; }"
      "</style>"
      "<span id=sample>foo</span>");
  Element* sample = GetDocument().getElementById("sample");
  UpdateAllLifecyclePhases();
  LayoutObject* before_layout_object = sample->GetLayoutObject();
  const std::unique_ptr<DragImage> image =
      DataTransfer::NodeImage(GetFrame(), *sample);

  EXPECT_TRUE(sample->GetLayoutObject() != before_layout_object)
      << ":-webkit-drag causes sample to have different layout object.";
  EXPECT_EQ(Color(255, 0, 0),
            sample->GetLayoutObject()->ResolveColor(CSSPropertyColor))
      << "#sample has :-webkit-drag.";

  // Layout w/o :-webkit-drag
  UpdateAllLifecyclePhases();

  EXPECT_EQ(Color(0, 0, 255),
            sample->GetLayoutObject()->ResolveColor(CSSPropertyColor))
      << "#sample doesn't have :-webkit-drag.";
}

TEST_F(DataTransferTest, NodeImageExceedsWindowBounds) {
  SetBodyContent(
      "<style>"
      "  * { margin: 0; } "
      "  #sample { width: 2000px; height: 2000px; }"
      "</style>"
      "<div id=sample></div>");
  Element& sample = *GetDocument().getElementById("sample");
  const std::unique_ptr<DragImage> image =
      DataTransfer::NodeImage(GetFrame(), sample);
  EXPECT_EQ(IntSize(800, 600), image->Size());
}

TEST_F(DataTransferTest, NodeImageUnderScrollOffset) {
  SetBodyContent(
      "<style>"
      "  * { margin: 0; } "
      "  #first { width: 500px; height: 500px; }"
      "  #second { width: 500px; height: 500px; }"
      "</style>"
      "<div id=first></div>"
      "<div id=second></div>");
  LocalFrameView* frame_view = GetDocument().View();
  frame_view->LayoutViewportScrollableArea()->SetScrollOffset(
      ScrollOffset(0, 72), kProgrammaticScroll);

  Element& first = *GetDocument().getElementById("first");
  const std::unique_ptr<DragImage> first_image =
      DataTransfer::NodeImage(GetFrame(), first);
  EXPECT_EQ(IntSize(500, 500 - 72), first_image->Size());

  Element& second = *GetDocument().getElementById("second");
  const std::unique_ptr<DragImage> second_image =
      DataTransfer::NodeImage(GetFrame(), second);
  EXPECT_EQ(IntSize(500, 600 - 500 + 72), second_image->Size());
}

TEST_F(DataTransferTest, NodeImageWithPageScaleFactor) {
  SetBodyContent(
      "<style>"
      "  * { margin: 0; } "
      "  html, body { height: 2000px; }"
      "  #sample { width: 200px; height: 141px; }"
      "</style>"
      "<div id=sample></div>");
  GetPage().SetPageScaleFactor(2);
  Element& sample = *GetDocument().getElementById("sample");
  const std::unique_ptr<DragImage> image =
      DataTransfer::NodeImage(GetFrame(), sample);
  EXPECT_EQ(IntSize(400, 282), image->Size());

  // Check that a scroll offset of 10px is scaled to device coordinates which
  // includes page scale factor.
  LocalFrameView* frame_view = GetDocument().View();
  frame_view->LayoutViewportScrollableArea()->SetScrollOffset(
      ScrollOffset(0, 10), kProgrammaticScroll);
  const std::unique_ptr<DragImage> image_with_scroll_offset =
      DataTransfer::NodeImage(GetFrame(), sample);
  EXPECT_EQ(IntSize(400, 262), image_with_scroll_offset->Size());
}

}  // namespace blink
