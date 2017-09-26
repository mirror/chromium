// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/location_bar/find_bar_image_view.h"

#include "base/metrics/histogram_macros.h"
#include "chrome/app/chrome_command_ids.h"
#include "chrome/browser/bookmarks/bookmark_stats.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_commands.h"
#include "chrome/browser/ui/view_ids.h"
#include "chrome/browser/ui/views/bookmarks/bookmark_bubble_view.h"
#include "chrome/browser/ui/views/feature_promos/bookmark_promo_bubble_view.h"
#include "chrome/grit/generated_resources.h"
#include "components/strings/grit/components_strings.h"
#include "components/toolbar/vector_icons.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/gfx/color_utils.h"
#include "ui/gfx/paint_vector_icon.h"
#include "ui/views/widget/widget_observer.h"

FindBarImageView::FindBarImageView(CommandUpdater* command_updater,
                                   Browser* browser)
    : BubbleIconView(command_updater, IDC_BOOKMARK_PAGE) {
  set_id(VIEW_ID_STAR_BUTTON);
}

FindBarImageView::~FindBarImageView() {}

void FindBarImageView::OnExecuting(
    BubbleIconView::ExecuteSource execute_source) {
  //  BookmarkEntryPoint entry_point = BOOKMARK_ENTRY_POINT_STAR_MOUSE;
  //  switch (execute_source) {
  //    case EXECUTE_SOURCE_MOUSE:
  //      entry_point = BOOKMARK_ENTRY_POINT_STAR_MOUSE;
  //      break;
  //    case EXECUTE_SOURCE_KEYBOARD:
  //      entry_point = BOOKMARK_ENTRY_POINT_STAR_KEY;
  //      break;
  //    case EXECUTE_SOURCE_GESTURE:
  //      entry_point = BOOKMARK_ENTRY_POINT_STAR_GESTURE;
  //      break;
  //  }
  //  UMA_HISTOGRAM_ENUMERATION("Bookmarks.EntryPoint",
  //                            entry_point,
  //                            BOOKMARK_ENTRY_POINT_LIMIT);
}

void FindBarImageView::ExecuteCommand(ExecuteSource source) {
  //  if (browser_) {
  //    OnExecuting(source);
  //    chrome::BookmarkCurrentPageIgnoringExtensionOverrides(browser_);
  //  } else {
  //    BubbleIconView::ExecuteCommand(source);
  //  }
}

views::BubbleDialogDelegateView* FindBarImageView::GetBubble() const {
  return BookmarkBubbleView::bookmark_bubble();
}

const gfx::VectorIcon& FindBarImageView::GetVectorIcon() const {
  return active() ? toolbar::kStarActiveIcon : toolbar::kStarIcon;
}

void FindBarImageView::VisibilityChanged(views::View* starting_from,
                                         bool is_visible) {
  SetInkDropMode(is_visible ? views::InkDropHostView::InkDropMode::ON
                            : views::InkDropHostView::InkDropMode::OFF);
}

void FindBarImageView::OnWidgetDestroying(views::Widget* widget) {
  //  if (bookmark_promo_observer_.IsObserving(widget)) {
  //    bookmark_promo_observer_.Remove(widget);
  //    AnimateInkDrop(views::InkDropState::DEACTIVATED, nullptr);
  //    SetActiveInternal(false);
  //    UpdateIcon();
  //  }
}
