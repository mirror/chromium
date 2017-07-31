// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/bookmarks/bookmark_promo_bubble_view.h"

#include "base/memory/ptr_util.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/ui/bookmarks/bookmark_bubble_observer.h"
#include "chrome/browser/ui/view_ids.h"
#include "chrome/grit/generated_resources.h"
#include "components/variations/variations_associated_data.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/gfx/geometry/insets.h"
#include "ui/views/controls/label.h"
#include "ui/views/layout/box_layout.h"
#include "ui/views/widget/widget.h"

namespace {

// The amount of time the promo should stay onscreen if it is never hovered.
// Constant subject to change after interactive tests.
constexpr base::TimeDelta kBubbleCloseDelayDefault =
    base::TimeDelta::FromSeconds(5);

// The amount of time the promo should stay onscreen after the user stops
// hovering it. Constant subject to change after interactive tests.
constexpr base::TimeDelta kBubbleCloseDelayShort =
    base::TimeDelta::FromSecondsD(2.5);

}  // namespace

BookmarkPromoBubbleView* BookmarkPromoBubbleView::Create(
    views::View* anchor_view) {
  return new BookmarkPromoBubbleView(anchor_view);
}

BookmarkPromoBubbleView::BookmarkPromoBubbleView(views::View* anchor_view)
    : LocationBarBubbleDelegateView(anchor_view, nullptr) {
  set_arrow(views::BubbleBorder::TOP_RIGHT);
  auto box_layout = base::MakeUnique<views::BoxLayout>(
      views::BoxLayout::kVertical, gfx::Insets(), 0);
  box_layout->set_main_axis_alignment(
      views::BoxLayout::MAIN_AXIS_ALIGNMENT_CENTER);
  box_layout->set_cross_axis_alignment(
      views::BoxLayout::CROSS_AXIS_ALIGNMENT_CENTER);
  SetLayoutManager(box_layout.release());

  static constexpr int kTextIds[] = {IDS_BOOKMARK_PROMO_0, IDS_BOOKMARK_PROMO_1,
                                     IDS_BOOKMARK_PROMO_2};
  const std::string& str = variations::GetVariationParamValue(
      "BookmarkInProductHelp", "x_promo_string");
  size_t text_specifier;
  if (!base::StringToSizeT(str, &text_specifier) ||
      text_specifier >= arraysize(kTextIds)) {
    text_specifier = 0;
  }
  AddChildView(
      new views::Label(l10n_util::GetStringUTF16(kTextIds[text_specifier])));
  StartAutoCloseTimer(kBubbleCloseDelayDefault);
}

BookmarkPromoBubbleView::~BookmarkPromoBubbleView() = default;

int BookmarkPromoBubbleView::GetDialogButtons() const {
  return ui::DIALOG_BUTTON_NONE;
}

bool BookmarkPromoBubbleView::OnMousePressed(const ui::MouseEvent& event) {
  CloseBubble();
  return true;
}

void BookmarkPromoBubbleView::OnMouseEntered(const ui::MouseEvent& event) {
  timer_.Stop();
}

void BookmarkPromoBubbleView::OnMouseExited(const ui::MouseEvent& event) {
  StartAutoCloseTimer(kBubbleCloseDelayShort);
}

void BookmarkPromoBubbleView::CloseBubble() {
  GetWidget()->Close();
}

void BookmarkPromoBubbleView::StartAutoCloseTimer(
    base::TimeDelta auto_close_duration) {
  timer_.Start(FROM_HERE, auto_close_duration, this,
               &BookmarkPromoBubbleView::CloseBubble);
}
