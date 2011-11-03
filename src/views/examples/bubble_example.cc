// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "views/examples/bubble_example.h"

#include "base/utf_string_conversions.h"
#include "views/bubble/bubble_delegate.h"
#include "views/controls/button/text_button.h"
#include "views/controls/label.h"
#include "views/layout/box_layout.h"
#include "views/layout/fill_layout.h"
#include "views/widget/widget.h"

namespace examples {

struct BubbleConfig {
  string16 label;
  SkColor color;
  gfx::Point anchor_point;
  views::BubbleBorder::ArrowLocation arrow;
  bool fade_in;
  bool fade_out;
};

// Create four types of bubbles, one without arrow, one with an arrow, one
// that fades in, and another that fades out and won't close on the escape key.
BubbleConfig kRoundConfig = { ASCIIToUTF16("RoundBubble"), 0xFFC1B1E1,
    gfx::Point(), views::BubbleBorder::NONE, false, false };
BubbleConfig kArrowConfig = { ASCIIToUTF16("ArrowBubble"), SK_ColorGRAY,
    gfx::Point(), views::BubbleBorder::TOP_LEFT, false, false };
BubbleConfig kFadeInConfig = { ASCIIToUTF16("FadeInBubble"), SK_ColorYELLOW,
    gfx::Point(), views::BubbleBorder::BOTTOM_RIGHT, true, false };
BubbleConfig kFadeOutConfig = { ASCIIToUTF16("FadeOutBubble"), SK_ColorWHITE,
    gfx::Point(), views::BubbleBorder::BOTTOM_RIGHT, false, true };

class ExampleBubbleDelegateView : public views::BubbleDelegateView {
 public:
  ExampleBubbleDelegateView(const BubbleConfig& config)
      : BubbleDelegateView(config.anchor_point, config.arrow, config.color),
        label_(config.label) {}

 protected:
  virtual void Init() OVERRIDE {
    SetLayoutManager(new views::FillLayout());
    views::Label* label = new views::Label(label_);
    label->set_border(views::Border::CreateSolidBorder(10, GetColor()));
    AddChildView(label);
  }

 private:
  string16 label_;
};

BubbleExample::BubbleExample(ExamplesMain* main)
    : ExampleBase(main, "Bubble") {}

BubbleExample::~BubbleExample() {}

void BubbleExample::CreateExampleView(views::View* container) {
  container->SetLayoutManager(
      new views::BoxLayout(views::BoxLayout::kHorizontal, 0, 0, 1));
  round_ = new views::TextButton(this, kRoundConfig.label);
  arrow_ = new views::TextButton(this, kArrowConfig.label);
  fade_in_ = new views::TextButton(this, kFadeInConfig.label);
  fade_out_ = new views::TextButton(this, kFadeOutConfig.label);
  container->AddChildView(round_);
  container->AddChildView(arrow_);
  container->AddChildView(fade_in_);
  container->AddChildView(fade_out_);
}

void BubbleExample::ButtonPressed(views::Button* sender,
                                  const views::Event& event) {
  BubbleConfig config;
  if (sender == round_)
    config = kRoundConfig;
  else if (sender == arrow_)
    config = kArrowConfig;
  else if (sender == fade_in_)
    config = kFadeInConfig;
  else if (sender == fade_out_)
    config = kFadeOutConfig;

  config.anchor_point.set_x(sender->width() / 2);
  config.anchor_point.set_y(sender->height() / 2);
  views::View::ConvertPointToScreen(sender, &config.anchor_point);

  ExampleBubbleDelegateView* bubble_delegate =
      new ExampleBubbleDelegateView(config);
  views::BubbleDelegateView::CreateBubble(bubble_delegate,
                                          example_view()->GetWidget());

  if (config.fade_in)
    bubble_delegate->StartFade(true);
  else
    bubble_delegate->Show();

  if (config.fade_out) {
    bubble_delegate->set_close_on_esc(false);
    bubble_delegate->StartFade(false);
  }
}

}  // namespace examples
