// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/test_tab_dialogs.h"

TestTabDialogs::TestTabDialogs() {
  TabDialogs::SetTabDialogsForTest(this);
}

TestTabDialogs::~TestTabDialogs() {
  TabDialogs::SetTabDialogsForTest(nullptr);
}

gfx::NativeView TestTabDialogs::GetDialogParentView() const {
  return nullptr;
}

void TestTabDialogs::ShowCollectedCookies() {}

void TestTabDialogs::ShowHungRendererDialog(
    const content::WebContentsUnresponsiveState& unresponsive_state) {}

void TestTabDialogs::HideHungRendererDialog() {}

void TestTabDialogs::ShowProfileSigninConfirmation(
    Browser* browser,
    Profile* profile,
    const std::string& username,
    std::unique_ptr<ui::ProfileSigninConfirmationDelegate> delegate) {}

void TestTabDialogs::ShowManagePasswordsBubble(bool user_action) {}

void TestTabDialogs::HideManagePasswordsBubble() {}

base::WeakPtr<ValidationMessageBubble> TestTabDialogs::ShowValidationMessage(
    const gfx::Rect& anchor_in_root_view,
    const base::string16& main_text,
    const base::string16& sub_text) {
  return nullptr;
}

autofill::SaveCardBubbleView* TestTabDialogs::ShowSaveCardBubble(
    autofill::SaveCardBubbleController* controller,
    bool user_gesture) {
  return nullptr;
}
