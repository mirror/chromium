// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/hover_button.h"

#include <memory>

#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/ui/views/harmony/chrome_layout_provider.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/accessibility/ax_node_data.h"
#include "ui/gfx/text_utils.h"
#include "ui/views/controls/button/button.h"
#include "ui/views/controls/label.h"
#include "ui/views/test/scoped_views_test_helper.h"
#include "ui/views/test/test_views_delegate.h"
#include "ui/views/test/views_test_base.h"
#include "ui/views/view.h"
#include "ui/views/widget/widget.h"

namespace test {

namespace {

constexpr int kButtonWidth = 150;

struct TitleSubtitlePair {
  const char* const title;
  const char* const subtitle;
  // Whether the HoverButton is expected to have a tooltip for this text.
  bool tooltip;
};

constexpr TitleSubtitlePair kTitleSubtitlePairs[] = {
    // Two short strings that will fit in the space given.
    {"Clap!", "Clap!", false},
    // First string fits, second string doesn't.
    {"If you're happy and you know it, clap your hands!", "Clap clap!", true},
    // Second string fits, first string doesn't.
    {"Clap clap!",
     "If you're happy and you know it, and you really want to show it,", true},
    // Both strings don't fit.
    {"If you're happy and you know it, and you really want to show it,",
     "If you're happy and you know it, clap your hands!", true},
};

class DummyButtonListener : public views::ButtonListener {
 public:
  DummyButtonListener() {}

  // views::ButtonListener:
  void ButtonPressed(views::Button* sender, const ui::Event& event) override {}

 private:
  DISALLOW_COPY_AND_ASSIGN(DummyButtonListener);
};

class HoverButtonTest : public views::ViewsTestBase,
                        public testing::WithParamInterface<TitleSubtitlePair> {
 public:
  HoverButtonTest() {}

  std::unique_ptr<views::View> CreateIcon() {
    auto icon = std::make_unique<views::View>();
    icon->SetPreferredSize(gfx::Size(16, 16));
    return icon;
  }

  views::Widget* widget() { return widget_.get(); }

  // views::ViewsTestBase:
  void SetUp() override {
    ViewsTestBase::SetUp();
    test_views_delegate()->set_layout_provider(
        ChromeLayoutProvider::CreateLayoutProvider());

    widget_ = std::make_unique<views::Widget>();
    views::Widget::InitParams params = views::ViewsTestBase::CreateParams(
        views::Widget::InitParams::TYPE_POPUP);
    params.ownership = views::Widget::InitParams::WIDGET_OWNS_NATIVE_WIDGET;
    widget_->Init(params);
  }

  void TearDown() override {
    widget()->CloseNow();
    ViewsTestBase::TearDown();
  }

 private:
  std::unique_ptr<views::Widget> widget_;

  DISALLOW_COPY_AND_ASSIGN(HoverButtonTest);
};

}  // namespace

TEST_F(HoverButtonTest, ValidateTestData) {
  const gfx::FontList font_list = views::Label::GetDefaultFontList();
  // Double check the length of the strings used for testing are either over
  // or under the width used for the test.
  ASSERT_GT(kButtonWidth,
            gfx::GetStringWidth(
                base::ASCIIToUTF16(kTitleSubtitlePairs[0].title), font_list));
  ASSERT_GT(
      kButtonWidth,
      gfx::GetStringWidth(base::ASCIIToUTF16(kTitleSubtitlePairs[0].subtitle),
                          font_list));

  ASSERT_LT(kButtonWidth,
            gfx::GetStringWidth(
                base::ASCIIToUTF16(kTitleSubtitlePairs[1].title), font_list));
  ASSERT_GT(
      kButtonWidth,
      gfx::GetStringWidth(base::ASCIIToUTF16(kTitleSubtitlePairs[1].subtitle),
                          font_list));

  ASSERT_GT(kButtonWidth,
            gfx::GetStringWidth(
                base::ASCIIToUTF16(kTitleSubtitlePairs[2].title), font_list));
  ASSERT_LT(
      kButtonWidth,
      gfx::GetStringWidth(base::ASCIIToUTF16(kTitleSubtitlePairs[2].subtitle),
                          font_list));

  ASSERT_LT(kButtonWidth,
            gfx::GetStringWidth(
                base::ASCIIToUTF16(kTitleSubtitlePairs[3].title), font_list));
  ASSERT_LT(
      kButtonWidth,
      gfx::GetStringWidth(base::ASCIIToUTF16(kTitleSubtitlePairs[3].subtitle),
                          font_list));
}

// Tests whether the HoverButton has the correct tooltip and accessible name.
TEST_P(HoverButtonTest, TooltipAndAccessibleName) {
  DummyButtonListener listener;
  HoverButton* button = new HoverButton(
      &listener, CreateIcon(), base::ASCIIToUTF16(GetParam().title),
      base::ASCIIToUTF16(GetParam().subtitle));
  widget()->SetContentsView(button);
  widget()->SetSize(gfx::Size(kButtonWidth, 40));

  ui::AXNodeData data;
  button->GetAccessibleNodeData(&data);
  std::string accessible_name;
  data.GetStringAttribute(ui::AX_ATTR_NAME, &accessible_name);

  base::string16 expected =
      base::JoinString({base::ASCIIToUTF16(GetParam().title),
                        base::ASCIIToUTF16(GetParam().subtitle)},
                       base::ASCIIToUTF16("\n"));
  EXPECT_EQ(expected, base::UTF8ToUTF16(accessible_name));

  base::string16 tooltip_text;
  button->GetTooltipText(button->GetBoundsInScreen().CenterPoint(),
                         &tooltip_text);
  if (GetParam().tooltip) {
    EXPECT_EQ(expected, tooltip_text);
  } else {
    EXPECT_EQ(base::string16(), tooltip_text);
  }
}

// Tests that setting a custom tooltip on a HoverButton will not be overwritten
// by HoverButton's own tooltips.
TEST_P(HoverButtonTest, CustomTooltip) {
  const base::string16 custom_tooltip = base::ASCIIToUTF16("custom");
  DummyButtonListener listener;
  HoverButton* button = new HoverButton(
      &listener, CreateIcon(), base::ASCIIToUTF16(GetParam().title),
      base::ASCIIToUTF16(GetParam().subtitle));
  button->set_auto_compute_tooltip(false);
  button->SetTooltipText(custom_tooltip);
  widget()->SetContentsView(button);
  widget()->SetSize(gfx::Size(kButtonWidth, 40));

  base::string16 tooltip_text;
  button->GetTooltipText(button->GetBoundsInScreen().CenterPoint(),
                         &tooltip_text);
  EXPECT_EQ(custom_tooltip, tooltip_text);

  // Make sure the accessible name is still set.
  ui::AXNodeData data;
  button->GetAccessibleNodeData(&data);
  std::string accessible_name;
  data.GetStringAttribute(ui::AX_ATTR_NAME, &accessible_name);

  base::string16 expected =
      base::JoinString({base::ASCIIToUTF16(GetParam().title),
                        base::ASCIIToUTF16(GetParam().subtitle)},
                       base::ASCIIToUTF16("\n"));
  EXPECT_EQ(expected, base::UTF8ToUTF16(accessible_name));
}

INSTANTIATE_TEST_CASE_P(,
                        HoverButtonTest,
                        testing::ValuesIn(kTitleSubtitlePairs));

}  // namespace test
