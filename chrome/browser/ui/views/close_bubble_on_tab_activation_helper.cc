// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/close_bubble_on_tab_activation_helper.h"
#include "chrome/browser/ui/browser.h"
#include "ui/views/bubble/bubble_dialog_delegate.h"
#include "ui/views/widget/widget.h"

CloseBubbleOnTabActivationHelper::CloseBubbleOnTabActivationHelper(
    views::BubbleDialogDelegateView* bubble,
    Browser* browser)
    : bubble_(bubble), browser_(browser), tab_strip_observer_(this) {
  DCHECK(bubble_);
  DCHECK(browser_);
  tab_strip_observer_.Add(browser_->tab_strip_model());
}

CloseBubbleOnTabActivationHelper::~CloseBubbleOnTabActivationHelper() = default;

void CloseBubbleOnTabActivationHelper::ActiveTabChanged(
    content::WebContents* old_contents,
    content::WebContents* new_contents,
    int index,
    int reason) {
  if (bubble_) {
    // On macOS, the profile chooser bubble must be closed every time the user
    // adds, closes or changes the active tab using keyboard shortcuts.
    views::Widget* bubble_widget = bubble_->GetWidget();
    if (bubble_widget)
      bubble_widget->Close();
    bubble_ = nullptr;
  }
}
