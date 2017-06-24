// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/tabs/new_tab_promo.h"
#include "base/logging.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/grit/generated_resources.h"

namespace {

// The default time that the promo bubble should stay on the screen if it will
// close automatically.
constexpr base::TimeDelta kBubbleCloseDelayDefault =
    base::TimeDelta::FromMilliseconds(5000);

// A shorter timeout used for how long the promo bubble should stay on the
// screen before it will close automatically after it has been hovered over.
constexpr base::TimeDelta kBubbleCloseDelayShort =
    base::TimeDelta::FromMilliseconds(2500);

}  // namespace

void NewTabPromo::CloseBubble() {
  if (has_children())
    GetWidget()->Close();
}

NewTabPromo::NewTabPromo(TabStrip* tab_strip)
    : auto_close_duration_(kBubbleCloseDelayDefault),
      BubbleDialogDelegateView(),
      tab_strip_(tab_strip) {
  SetAnchorRect(tab_strip_->GetPromoAnchorRect());
  this->set_arrow(views::BubbleBorder::LEFT_TOP);

  views::Widget* newtab_promo_widget =
      views::BubbleDialogDelegateView::CreateBubble(this);
  this->StartTimer();
  newtab_promo_widget->Show();
}

NewTabPromo::~NewTabPromo() {}

int NewTabPromo::GetDialogButtons() const {
  return ui::DIALOG_BUTTON_NONE;
}

void NewTabPromo::OnMouseEntered(const ui::MouseEvent& event) {
  StopTimer();
}

void NewTabPromo::OnMouseExited(const ui::MouseEvent& event) {
  auto_close_duration_ = kBubbleCloseDelayShort;
  StartTimer();
}

bool NewTabPromo::OnMousePressed(const ui::MouseEvent& event) {
  NotifyTabStripOnClose();
  return true;
}

void NewTabPromo::Init() {
  // Set up the layout of the promo bubble.
  views::BoxLayout* box_layout =
      new views::BoxLayout(views::BoxLayout::kHorizontal, gfx::Insets(0), 0);

  box_layout->set_main_axis_alignment(
      views::BoxLayout::MAIN_AXIS_ALIGNMENT_CENTER);
  box_layout->set_cross_axis_alignment(
      views::BoxLayout::CROSS_AXIS_ALIGNMENT_CENTER);
  SetLayoutManager(box_layout);

  views::Label* label_(
      new views::Label(l10n_util::GetStringUTF16(IDS_ACCNAME_NEWTAB_PROMO)));
  AddChildView(label_);
}

void NewTabPromo::StartTimer() {
  timer_.Start(FROM_HERE, auto_close_duration_, this,
               &NewTabPromo::NotifyTabStripOnClose);
}

void NewTabPromo::StopTimer() {
  timer_.Stop();
}

void NewTabPromo::NotifyTabStripOnClose() {
  tab_strip_->ClosePromo();
}
