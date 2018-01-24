// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/hover_button.h"

#include <memory>

#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/ui/views/harmony/chrome_layout_provider.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/accessibility/ax_node_data.h"
#include "ui/gfx/text_utils.h"
#include "ui/views/bubble/bubble_dialog_delegate.h"
#include "ui/views/controls/button/button.h"
#include "ui/views/controls/label.h"
#include "ui/views/focus/focus_manager.h"
#include "ui/views/test/test_views_delegate.h"
#include "ui/views/test/views_test_base.h"
#include "ui/views/test/widget_test.h"
#include "ui/views/view.h"
#include "ui/views/widget/widget.h"

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

class BubbleWithButton : public views::BubbleDialogDelegateView {
 public:
  BubbleWithButton(HoverButton* button, views::View* anchor)
      : BubbleDialogDelegateView(anchor, views::BubbleBorder::NONE) {
    this->AddChildView(button);
  }
};

class HoverButtonTest : public views::ViewsTestBase {
 public:
  HoverButtonTest() {}

  std::unique_ptr<views::View> CreateIcon() {
    auto icon = std::make_unique<views::View>();
    icon->SetPreferredSize(gfx::Size(16, 16));
    return icon;
  }

  // views::ViewsTestBase:
  void SetUp() override {
    ViewsTestBase::SetUp();
    // HoverButton uses Chrome-specific layout constants, so make sure these
    // exist for testing.
    test_views_delegate()->set_layout_provider(
        ChromeLayoutProvider::CreateLayoutProvider());
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(HoverButtonTest);
};

class HoverButtonWidgetTest : public views::test::WidgetTest {
 public:
  HoverButtonWidgetTest() {}

  void SetUp() override {
    WidgetTest::SetUp();
    desktop_widget_ = CreateTopLevelPlatformWidget();
    test_views_delegate()->set_layout_provider(
        ChromeLayoutProvider::CreateLayoutProvider());
  }

  void TearDown() override {
    desktop_widget_->CloseNow();
    WidgetTest::TearDown();
  }

 private:
  views::Widget* desktop_widget_;

  DISALLOW_COPY_AND_ASSIGN(HoverButtonWidgetTest);
};

}  // namespace

// Double check the length of the strings used for testing are either over or
// under the width used for the following tests.
TEST_F(HoverButtonTest, ValidateTestData) {
  auto get_width = [](const char* text) {
    return views::Label(base::ASCIIToUTF16(text)).GetPreferredSize().width();
  };
  EXPECT_GT(kButtonWidth, get_width(kTitleSubtitlePairs[0].title));
  EXPECT_GT(kButtonWidth, get_width(kTitleSubtitlePairs[0].subtitle));

  EXPECT_LT(kButtonWidth, get_width(kTitleSubtitlePairs[1].title));
  EXPECT_GT(kButtonWidth, get_width(kTitleSubtitlePairs[1].subtitle));

  EXPECT_GT(kButtonWidth, get_width(kTitleSubtitlePairs[2].title));
  EXPECT_LT(kButtonWidth, get_width(kTitleSubtitlePairs[2].subtitle));

  EXPECT_LT(kButtonWidth, get_width(kTitleSubtitlePairs[3].title));
  EXPECT_LT(kButtonWidth, get_width(kTitleSubtitlePairs[3].subtitle));
}

// Tests whether the HoverButton has the correct tooltip and accessible name.
TEST_F(HoverButtonTest, TooltipAndAccessibleName) {
  for (size_t i = 0; i < arraysize(kTitleSubtitlePairs); ++i) {
    TitleSubtitlePair pair = kTitleSubtitlePairs[i];
    SCOPED_TRACE(testing::Message() << "Index: " << i << ", expected_tooltip="
                                    << (pair.tooltip ? "true" : "false"));
    auto button = std::make_unique<HoverButton>(
        nullptr, CreateIcon(), base::ASCIIToUTF16(pair.title),
        base::ASCIIToUTF16(pair.subtitle));
    button->SetSize(gfx::Size(kButtonWidth, 40));

    ui::AXNodeData data;
    button->GetAccessibleNodeData(&data);
    std::string accessible_name;
    data.GetStringAttribute(ui::AX_ATTR_NAME, &accessible_name);

    // The accessible name should always be the title and subtitle concatenated
    // by \n.
    base::string16 expected = base::JoinString(
        {base::ASCIIToUTF16(pair.title), base::ASCIIToUTF16(pair.subtitle)},
        base::ASCIIToUTF16("\n"));
    EXPECT_EQ(expected, base::UTF8ToUTF16(accessible_name));

    base::string16 tooltip_text;
    button->GetTooltipText(gfx::Point(), &tooltip_text);
    if (pair.tooltip) {
      EXPECT_EQ(expected, tooltip_text);
    } else {
      EXPECT_EQ(base::string16(), tooltip_text);
    }
  }
}

// Tests that setting a custom tooltip on a HoverButton will not be overwritten
// by HoverButton's own tooltips.
TEST_F(HoverButtonTest, CustomTooltip) {
  const base::string16 custom_tooltip = base::ASCIIToUTF16("custom");

  for (size_t i = 0; i < arraysize(kTitleSubtitlePairs); ++i) {
    SCOPED_TRACE(testing::Message() << "Index: " << i);
    TitleSubtitlePair pair = kTitleSubtitlePairs[i];
    auto button = std::make_unique<HoverButton>(
        nullptr, CreateIcon(), base::ASCIIToUTF16(pair.title),
        base::ASCIIToUTF16(pair.subtitle));
    button->set_auto_compute_tooltip(false);
    button->SetTooltipText(custom_tooltip);
    button->SetSize(gfx::Size(kButtonWidth, 40));

    base::string16 tooltip_text;
    button->GetTooltipText(gfx::Point(), &tooltip_text);
    EXPECT_EQ(custom_tooltip, tooltip_text);

    // Make sure the accessible name is still set.
    ui::AXNodeData data;
    button->GetAccessibleNodeData(&data);
    std::string accessible_name;
    data.GetStringAttribute(ui::AX_ATTR_NAME, &accessible_name);

    // The accessible name should always be the title and subtitle concatenated
    // by \n.
    base::string16 expected = base::JoinString(
        {base::ASCIIToUTF16(pair.title), base::ASCIIToUTF16(pair.subtitle)},
        base::ASCIIToUTF16("\n"));
    EXPECT_EQ(expected, base::UTF8ToUTF16(accessible_name));
  }
}

// Test that a hover button only gets focus when it is in the active window.
TEST_F(HoverButtonWidgetTest, CorrectFocusState) {
  HoverButton* button1 =
      new HoverButton(nullptr, base::ASCIIToUTF16("Button 1"));
  HoverButton* button2 =
      new HoverButton(nullptr, base::ASCIIToUTF16("Button 2"));

  views::Widget* anchor = CreateTopLevelPlatformWidget();
  // Place |button1| and |button2| in different windows.
  views::Widget* bubble1 = BubbleWithButton::CreateBubble(
      new BubbleWithButton(button1, anchor->GetContentsView()));
  views::Widget* bubble2 = BubbleWithButton::CreateBubble(
      new BubbleWithButton(button2, anchor->GetContentsView()));

  // Show |bubble1| first and then |bubble2|, such that |bubble2| is the active
  // window.
  bubble1->Show();
  bubble2->Show();

  // Check whether the buttons are assigned to the correct widgets.
  EXPECT_EQ(bubble1, button1->GetWidget());
  EXPECT_EQ(bubble2, button2->GetWidget());

  // Since the buttons are in different windows, they should have different
  // focus managers.
  views::FocusManager* manager1 = button1->GetFocusManager();
  views::FocusManager* manager2 = button2->GetFocusManager();
  EXPECT_NE(manager1, manager2);

  // Hover over |button1|. It shouldn't affect the focus.
  button1->SetState(views::Button::ButtonState::STATE_HOVERED);
  EXPECT_NE(manager1->GetFocusedView(), button1);

  // |button2| also shouldn't be focused at this point.
  EXPECT_NE(manager2->GetFocusedView(), button2);

  // Hover over |button2| such that it gets the focus.
  button2->SetState(views::Button::ButtonState::STATE_HOVERED);
  EXPECT_EQ(manager2->GetFocusedView(), button2);

  // Close all showing widgets.
  bubble1->Close();
  bubble2->Close();
  anchor->Close();
}
