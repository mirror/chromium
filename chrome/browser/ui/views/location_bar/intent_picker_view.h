// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_VIEWS_LOCATION_BAR_INTENT_PICKER_VIEW_H_
#define CHROME_BROWSER_UI_VIEWS_LOCATION_BAR_INTENT_PICKER_VIEW_H_

#include "base/macros.h"
#include "chrome/browser/ui/views/location_bar/bubble_icon_view.h"

class Browser;
class CommandUpdater;

// The entry point for the intent picker.
class IntentPickerView : public BubbleIconView {
 public:
  IntentPickerView(CommandUpdater* command_updater, Browser* browser);
  ~IntentPickerView() override;

 protected:
  // BubbleIconView:
  void OnExecuting(BubbleIconView::ExecuteSource execute_source) override;
  views::BubbleDialogDelegateView* GetBubble() const override;
  const gfx::VectorIcon& GetVectorIcon() const override;

 private:
  bool IsIncognitoMode();
  Browser* browser_;

  DISALLOW_COPY_AND_ASSIGN(IntentPickerView);
};

#endif  // CHROME_BROWSER_UI_VIEWS_LOCATION_BAR_INTENT_PICKER_VIEW_H_
