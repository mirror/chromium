// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_TOUCHBAR_CONTENTS_MANAGER_H_
#define CHROME_BROWSER_TOUCHBAR_CONTENTS_MANAGER_H_

#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_contents_delegate.h"
#include "content/public/browser/web_contents_observer.h"

using content::NavigationHandle;
using content::WebContents;
using content::WebContentsObserver;

class TouchbarWebContentsManager : public WebContentsObserver {
 public:
  TouchbarWebContentsManager(WebContents* web_contents)
      : WebContentsObserver(web_contents) {}
  void DisplayWebContentsInTouchbar(WebContents* popup);
  // True if the popup is initiated from an error page and the Dino Game
  // feature flag is activated.
  bool ShouldHidePopup(WindowOpenDisposition disposition);
  // content::WebContentsObserver:
  void DidFinishNavigation(NavigationHandle* navigation_handle) override;
  void WasShown() override;

 private:
  // True if the Touchbar Dino Game feature flag is activated.
  bool IsTouchbarDinoGameEnabled();
};

#endif  // CHROME_BROWSER_TOUCHBAR_CONTENTS_MANAGER_H_
