// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/location_bar/intent_picker_view.h"

#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_commands.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/browser/ui/views/intent_picker_bubble_view.h"
#include "components/toolbar/vector_icons.h"
#include "content/public/browser/browser_context.h"

IntentPickerView::IntentPickerView(CommandUpdater* command_updater,
                                   Browser* browser)
    : BubbleIconView(nullptr, 0), browser_(browser) {
  IntentPickerBubbleView::CloseCurrentBubble();
}

IntentPickerView::~IntentPickerView() = default;

void IntentPickerView::OnExecuting(
    BubbleIconView::ExecuteSource execute_source) {
  if (browser_ && !IsIncognitoMode()) {
    SetVisible(true);
    chrome::QueryAndDisplayArcApps(browser_, GURL());
  } else {
    SetVisible(false);
  }
}

views::BubbleDialogDelegateView* IntentPickerView::GetBubble() const {
  return IntentPickerBubbleView::intent_picker_bubble();
}

bool IntentPickerView::IsIncognitoMode() {
  return browser_->tab_strip_model()
      ->GetActiveWebContents()
      ->GetBrowserContext()
      ->IsOffTheRecord();
}

const gfx::VectorIcon& IntentPickerView::GetVectorIcon() const {
  return toolbar::kOpenInNewIcon;
}
