// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/safe_browsing/chrome_cleaner/chrome_cleaner_prompt_delegate.h"
#include "chrome/browser/ui/browser_dialogs.h"

namespace safe_browsing {

void ChromeCleanerPromptDelegateOfficial::ShowChromeCleanerPrompt(
    Browser* browser,
    safe_browsing::ChromeCleanerDialogController* dialog_controller,
    safe_browsing::ChromeCleanerController* cleaner_controller) {
  chrome::ShowChromeCleanerPrompt(browser, dialog_controller,
                                  cleaner_controller);
}

}  // namespace safe_browsing
