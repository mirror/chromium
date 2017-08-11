// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_TOUCHBAR_CONTENTS_MANAGER_H_
#define CHROME_BROWSER_TOUCHBAR_CONTENTS_MANAGER_H_

#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_contents_delegate.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/browser/web_contents_user_data.h"

class TouchbarWebContentsManager
    : public content::WebContentsObserver,
      public content::WebContentsUserData<TouchbarWebContentsManager> {
 public:
  TouchbarWebContentsManager(content::WebContents* web_contents);
  ~TouchbarWebContentsManager() override;

  void DisplayWebContentsInTouchbar(content::WebContents* popup);

  // True if the popup is initiated from an error page and the Dino Game
  // feature flag is activated.
  bool ShouldHidePopup(WindowOpenDisposition disposition);

  // content::WebContentsObserver overrides.
  void DidFinishNavigation(
      content::NavigationHandle* navigation_handle) override;
  void WasShown() override;

  // Indicates/Sets whether the current page is an error page.
  bool IsErrorPage();
  void SetIsErrorPage(bool is_error_page);

  // Gets/Sets the popup's content::WebContents to display on the Touch Bar when
  // the page is an error page.
  content::WebContents* GetPopupContents();
  void SetPopupContents(content::WebContents* popup_contents);

  // True if the Touchbar Dino Game feature flag is activated.
  bool IsTouchbarDinoGameEnabled();

 private:
  friend class content::WebContentsUserData<TouchbarWebContentsManager>;

  // content::WebContents of the hidden popup created in the error page to be
  // displayed in the TouchBar.
  std::unique_ptr<content::WebContents> popup_contents_;
};

#endif  // CHROME_BROWSER_TOUCHBAR_CONTENTS_MANAGER_H_
