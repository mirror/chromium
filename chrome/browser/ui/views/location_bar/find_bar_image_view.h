// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_VIEWS_LOCATION_BAR_FIND_BAR_IMAGE_VIEW_H_
#define CHROME_BROWSER_UI_VIEWS_LOCATION_BAR_FIND_BAR_IMAGE_VIEW_H_

#include "base/macros.h"
#include "base/scoped_observer.h"
#include "chrome/browser/ui/views/location_bar/bubble_icon_view.h"

class Browser;
class CommandUpdater;

// The find icon to show when the find bar is visible.
class FindBarImageView : public BubbleIconView {
 public:
  FindBarImageView(CommandUpdater* command_updater, Browser* browser);
  ~FindBarImageView() override;

 protected:
  // BubbleIconView:
  void OnExecuting(BubbleIconView::ExecuteSource execute_source) override;
  void OnWidgetDestroying(views::Widget* widget) override;
  void ExecuteCommand(ExecuteSource source) override;
  views::BubbleDialogDelegateView* GetBubble() const override;
  const gfx::VectorIcon& GetVectorIcon() const override;

  // ui::View:
  void VisibilityChanged(views::View* starting_from, bool is_visible) override;

 private:
  DISALLOW_COPY_AND_ASSIGN(FindBarImageView);
};

#endif  // CHROME_BROWSER_UI_VIEWS_LOCATION_BAR_FIND_BAR_IMAGE_VIEW_H_
