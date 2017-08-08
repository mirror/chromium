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

using content::NavigationHandle;
using content::WebContents;
using content::WebContentsObserver;

class TouchbarWebContentsManager
    : public WebContentsObserver,
      public content::WebContentsUserData<TouchbarWebContentsManager> {
 public:
  TouchbarWebContentsManager(WebContents* web_contents);
  ~TouchbarWebContentsManager() override;

  void DisplayWebContentsInTouchbar(WebContents* popup);
  // True if the popup is initiated from an error page and the Dino Game
  // feature flag is activated.
  bool ShouldHidePopup(WindowOpenDisposition disposition);
  // content::WebContentsObserver overrides.
  void DidFinishNavigation(NavigationHandle* navigation_handle) override;
  void WasShown() override;

  // Indicates/Sets whether the current page is an error page.
  bool IsErrorPage();
  void SetIsErrorPage(bool is_error_page);

  // Gets/Sets the popup's WebContents to display on the Touch Bar when the
  // page is an error page.
  WebContents* GetPopupContents();
  void SetPopupContents(WebContents* popup_contents);

  // True if the Touchbar Dino Game feature flag is activated.
  bool IsTouchbarDinoGameEnabled();

 private:
  friend class content::WebContentsUserData<TouchbarWebContentsManager>;

  // content::WebContents of the hidden popup created in the error page to be
  // displayed in the TouchBar.
  std::unique_ptr<WebContents> popup_contents_;

  // Tells whether the current page is an error page or not, which is needed to
  // know in which content::WebContents to have popup_contents as a nullptr
  // (normal pages), and where to set it to the hidden popup's (error page)
  // content::WebContents.
  bool is_error_page_;
};

#endif  // CHROME_BROWSER_TOUCHBAR_CONTENTS_MANAGER_H_
