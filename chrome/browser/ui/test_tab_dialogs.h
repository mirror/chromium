// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_TEST_TAB_DIALOGS_H_
#define CHROME_BROWSER_UI_TEST_TAB_DIALOGS_H_

#include "chrome/browser/ui/tab_dialogs.h"

// Testing subclass of TabDialogs. This class provides empty implementations of
// all of TabDialogs' methods. It also installs itself as the TabDialogs
// implementation when constructed, replacing any existing implementation.
class TestTabDialogs : public TabDialogs {
 public:
  TestTabDialogs();
  ~TestTabDialogs() override;
  gfx::NativeView GetDialogParentView() const override;
  void ShowCollectedCookies() override;
  void ShowHungRendererDialog(
      const content::WebContentsUnresponsiveState& unresponsive_state) override;
  void HideHungRendererDialog() override;
  void ShowProfileSigninConfirmation(
      Browser* browser,
      Profile* profile,
      const std::string& username,
      std::unique_ptr<ui::ProfileSigninConfirmationDelegate> delegate) override;
  void ShowManagePasswordsBubble(bool user_action) override;
  void HideManagePasswordsBubble() override;
  base::WeakPtr<ValidationMessageBubble> ShowValidationMessage(
      const gfx::Rect& anchor_in_root_view,
      const base::string16& main_text,
      const base::string16& sub_text) override;
  autofill::SaveCardBubbleView* ShowSaveCardBubble(
      autofill::SaveCardBubbleController* controller,
      bool user_gesture) override;
};

#endif  // CHROME_BROWSER_UI_TEST_TAB_DIALOGS_H_
