// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/tabs/new_tab_promo.h"
#include "base/logging.h"
#include "base/strings/utf_string_conversions.h"
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
  NewTabPromo::NotifyNewTabButtonOnClose();
}

NewTabPromo::NewTabPromo(NewTabButton* new_tab_button,
                         const gfx::Rect anchor_rect)
    : BubbleDialogDelegateView(),
      auto_close_duration_(kBubbleCloseDelayDefault),
      new_tab_button_(new_tab_button) {
  SetAnchorRect(anchor_rect);
  set_arrow(views::BubbleBorder::LEFT_TOP);

  // Owned by the NewTabPromo. Will be destroyed when the NewTabPromo is
  // destroyed. Also is destroyed by click to the screen due to the inherient
  // properties of a BubbleDialogDelegate.
  views::Widget* newtab_promo_widget =
      views::BubbleDialogDelegateView::CreateBubble(this);

  StartAutoCloseTimer();
  newtab_promo_widget->Show();
}

NewTabPromo::~NewTabPromo() {}

int NewTabPromo::GetDialogButtons() const {
  return ui::DIALOG_BUTTON_NONE;
}

void NewTabPromo::OnMouseEntered(const ui::MouseEvent& event) {
  StopAutoCloseTimer();
}

void NewTabPromo::OnMouseExited(const ui::MouseEvent& event) {
  auto_close_duration_ = kBubbleCloseDelayShort;
  StartAutoCloseTimer();
}

bool NewTabPromo::OnMousePressed(const ui::MouseEvent& event) {
  NewTabPromo::CloseBubble();
  return true;
}

void NewTabPromo::OnWidgetDestroying(views::Widget* widget) {
  BubbleDialogDelegateView::OnWidgetDestroying(widget);
  NewTabPromo::CloseBubble();
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
  // Release is called here because the NewtabPromo picks up ownership of
  // |box_layout|.
  SetLayoutManager(box_layout.release());

  std::unique_ptr<views::Label> label_(
      new views::Label(l10n_util::GetStringUTF16(IDS_ACCNAME_NEWTAB_PROMO)));
  // Release is called here because the NewtabPromo picks up ownership of
  // |label_|.
  AddChildView(label_.release());
}

void NewTabPromo::StartAutoCloseTimer() {
  timer_.Start(FROM_HERE, auto_close_duration_, this,
               &NewTabPromo::CloseBubble);
}

void NewTabPromo::StopAutoCloseTimer() {
  timer_.Stop();
}

void NewTabPromo::NotifyNewTabButtonOnClose() {
  new_tab_button_->ClosePromo();
}
