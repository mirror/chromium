// Copyright 2017 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/tabs/newtab_promo.h"
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
    base::TimeDelta::FromMilliseconds(1000);

}  // namespace

// static
NewTabPromo* NewTabPromo::newtab_promo_ = nullptr;

// static
void NewTabPromo::ShowBubble(NewTabButton* newtab_button) {
  newtab_promo_ = new NewTabPromo(newtab_button);
  views::Widget* newtab_promo_widget =
      views::BubbleDialogDelegateView::CreateBubble(newtab_promo_);
  newtab_promo_widget->Show();
}

// static
bool NewTabPromo::ShowingPromo() {
  if (newtab_promo_)
    return true;
  return false;
}

// static
void NewTabPromo::CloseCurrentBubble() {
  if (newtab_promo_)
    newtab_promo_->CloseBubble();
}

// static
NewTabPromo* NewTabPromo::GetNewTabPromo() {
  return newtab_promo_;
}

NewTabPromo::WebContentMouseHandler::WebContentMouseHandler(
    NewTabPromo* bubble,
    content::WebContents* web_contents)
    : bubble_(bubble), web_contents_(web_contents) {
  DCHECK(bubble_);
  DCHECK(web_contents_);
  event_monitor_ = views::EventMonitor::CreateWindowMonitor(
      this, web_contents_->GetTopLevelNativeWindow());
}

NewTabPromo::WebContentMouseHandler::~WebContentMouseHandler() {}

void NewTabPromo::WebContentMouseHandler::OnMouseEvent(ui::MouseEvent* event) {
  if (event->type() == ui::ET_MOUSE_PRESSED)
    bubble_->CloseBubble();
}

void NewTabPromo::WebContentMouseHandler::OnTouchEvent(ui::TouchEvent* event) {
  if (event->type() == ui::ET_TOUCH_PRESSED)
    bubble_->CloseBubble();
}

NewTabPromo::NewTabPromo(NewTabButton* newtab_button)
    : label_(new views::Label(
          l10n_util::GetStringUTF16(IDS_ACCNAME_NEWTAB_PROMO))),
      auto_close_duration_(kBubbleCloseDelayDefault),
      BubbleDialogDelegateView((View*)newtab_button,
                               views::BubbleBorder::LEFT_TOP),
      newtab_button_(newtab_button) {
  // mouse_handler_.reset(new WebContentMouseHandler(this, web_contents));
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

void NewTabPromo::OnMouseReleased(const ui::MouseEvent& event) {
  if (event.IsLeftMouseButton())
    CloseBubble();
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

  AddChildView(label_);
  newtab_promo_->StartTimer();
}

void NewTabPromo::CloseBubble() {
  newtab_promo_ = nullptr;
  GetWidget()->Close();
  // newtab_button_->tab_strip_.PaintChildren();
}

void NewTabPromo::StartTimer() {
  timer_.Start(FROM_HERE, auto_close_duration_, this,
               &NewTabPromo::CloseBubble);
}

void NewTabPromo::StopTimer() {
  timer_.Stop();
}
