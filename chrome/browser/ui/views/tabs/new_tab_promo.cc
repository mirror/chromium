// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/tabs/new_tab_promo.h"

#include "base/logging.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/ui/views/tabs/new_tab_button.h"
#include "chrome/grit/generated_resources.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/gfx/geometry/insets.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/text_utils.h"
#include "ui/views/controls/label.h"
#include "ui/views/event_monitor.h"
#include "ui/views/layout/box_layout.h"
#include "ui/views/layout/fill_layout.h"

namespace {

// The amount of time the promo should stay onscreen if it is never hovered.
constexpr base::TimeDelta kBubbleCloseDelayDefault =
    base::TimeDelta::FromSeconds(5);

// The amount of time the promo should stay onscreen after the user stops
// hovering it.
constexpr base::TimeDelta kBubbleCloseDelayShort =
    base::TimeDelta::FromSecondsD(2.5);

}  // namespace

NewTabPromo::NewTabPromo(NewTabButton* new_tab_button, gfx::Rect anchor_rect)
    : BubbleDialogDelegateView(),
      auto_close_duration_(kBubbleCloseDelayDefault),
      new_tab_button_(new_tab_button) {
  SetAnchorRect(anchor_rect);
  set_arrow(views::BubbleBorder::LEFT_TOP);

  // Owned by the NewTabPromo. Will be destroyed when the NewTabPromo is
  // destroyed.
  views::Widget* newtab_promo_widget =
      views::BubbleDialogDelegateView::CreateBubble(this);
  // newtab_promo_widget->AddObserver(new_tab_button_);

  StartAutoCloseTimer();
  UseCompactMargins();
  newtab_promo_widget->Show();
}

NewTabPromo::~NewTabPromo() {}

int NewTabPromo::GetDialogButtons() const {
  return ui::DIALOG_BUTTON_NONE;
}

void NewTabPromo::Init() {
  // Set up the layout of the promo bubble.
  std::unique_ptr<views::BoxLayout> box_layout =
      base::MakeUnique<views::BoxLayout>(views::BoxLayout::kVertical,
                                         gfx::Insets(), 0);
  box_layout->set_main_axis_alignment(
      views::BoxLayout::MAIN_AXIS_ALIGNMENT_CENTER);
  box_layout->set_cross_axis_alignment(
      views::BoxLayout::CROSS_AXIS_ALIGNMENT_CENTER);
  SetLayoutManager(box_layout.release());
  AddChildView(new views::Label(l10n_util::GetStringUTF16(IDS_NEWTAB_PROMO)));
}

void NewTabPromo::OnMouseEntered(const ui::MouseEvent& event) {
  timer_.Stop();
}

void NewTabPromo::OnMouseExited(const ui::MouseEvent& event) {
  auto_close_duration_ = kBubbleCloseDelayShort;
  StartAutoCloseTimer();
}

bool NewTabPromo::OnMousePressed(const ui::MouseEvent& event) {
  NewTabPromo::CloseBubble();
  return true;
}

void NewTabPromo::OnWidgetClosing(views::Widget* widget) {
  BubbleDialogDelegateView::OnWidgetClosing(widget);
  new_tab_button_->OnPromoClosed();
}

void NewTabPromo::CloseBubble() {
  GetWidget()->Close();
}

void NewTabPromo::StartAutoCloseTimer() {
  timer_.Start(FROM_HERE, auto_close_duration_, this,
               &NewTabPromo::CloseBubble);
}
