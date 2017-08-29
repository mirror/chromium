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
  // TODO(djacobo): Remove this and control initial visibility via
  // LocationBarView. Set tooltip text too.
  SetVisible(false);
}

IntentPickerView::~IntentPickerView() = default;

void IntentPickerView::OnExecuting(
    BubbleIconView::ExecuteSource execute_source) {
  if (browser_ && !IsIncognitoMode()) {
    // TODO(djacobo): Launch ARC apps here.
  }
}

views::BubbleDialogDelegateView* IntentPickerView::GetBubble() const {
  // TODO(djacobo): Return the bubble once the UI is refactored.
  return nullptr;
}

bool IntentPickerView::IsIncognitoMode() {
  // TODO(djacobo): Remove this restriction when the feature is compatible w/
  // incognito mode.
  return browser_->tab_strip_model()
      ->GetActiveWebContents()
      ->GetBrowserContext()
      ->IsOffTheRecord();
}

const gfx::VectorIcon& IntentPickerView::GetVectorIcon() const {
  return toolbar::kOpenInNewIcon;
}
