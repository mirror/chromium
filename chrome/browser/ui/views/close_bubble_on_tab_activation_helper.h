// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_VIEWS_CLOSE_BUBBLE_ON_TAB_ACTIVATION_HELPER_H_
#define CHROME_BROWSER_UI_VIEWS_CLOSE_BUBBLE_ON_TAB_ACTIVATION_HELPER_H_

#include "base/scoped_observer.h"
#include "chrome/browser/ui/tabs/tab_strip_model_observer.h"

class Browser;

namespace views {
class BubbleDialogDelegateView;
}

class CloseBubbleOnTabActivationHelper : public TabStripModelObserver {
 public:
  CloseBubbleOnTabActivationHelper(views::BubbleDialogDelegateView* bubble,
                                   Browser* browser);
  ~CloseBubbleOnTabActivationHelper() override;

  // TabStripModelObserver:
  void ActiveTabChanged(content::WebContents* old_contents,
                        content::WebContents* new_contents,
                        int index,
                        int reason) override;

 private:
  views::BubbleDialogDelegateView* bubble_;  // weak, owns me
  Browser* browser_;  // weak, owns the browser window, which owns all bubbles
  ScopedObserver<TabStripModel, CloseBubbleOnTabActivationHelper>
      tab_strip_observer_;
};

#endif  // CHROME_BROWSER_UI_VIEWS_CLOSE_BUBBLE_ON_TAB_ACTIVATION_HELPER_H_
