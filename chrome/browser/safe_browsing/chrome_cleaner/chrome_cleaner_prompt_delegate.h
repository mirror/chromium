// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
#ifndef CHROME_BROWSER_SAFE_BROWSING_CHROME_CLEANER_CHROME_CLEANER_PROMPT_DELEGATE_H_
#define CHROME_BROWSER_SAFE_BROWSING_CHROME_CLEANER_CHROME_CLEANER_PROMPT_DELEGATE_H_

#include "chrome/browser/safe_browsing/chrome_cleaner/chrome_cleaner_controller_win.h"
#include "chrome/browser/safe_browsing/chrome_cleaner/chrome_cleaner_dialog_controller_win.h"
#include "chrome/browser/ui/browser.h"

namespace safe_browsing {

class ChromeCleanerPromptDelegate {
 public:
  virtual void ShowChromeCleanerPrompt(
      Browser* browser,
      safe_browsing::ChromeCleanerDialogController* dialog_controller,
      safe_browsing::ChromeCleanerController* cleaner_controller) = 0;
};

class ChromeCleanerPromptDelegateOfficial : public ChromeCleanerPromptDelegate {
 public:
  void ShowChromeCleanerPrompt(
      Browser* browser,
      safe_browsing::ChromeCleanerDialogController* dialog_controller,
      safe_browsing::ChromeCleanerController* cleaner_controller) override;
};

}  // namespace safe_browsing

#endif  // CHROME_BROWSER_SAFE_BROWSING_CHROME_CLEANER_CHROME_CLEANER_PROMPT_DELEGATE_H_
