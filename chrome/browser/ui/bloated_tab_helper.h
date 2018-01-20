// Copyright (c) 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_BLOATED_TAB_HELPER_H_
#define CHROME_BROWSER_UI_BLOATED_TAB_HELPER_H_

#include <map>

#include "base/macros.h"
#include "base/memory/linked_ptr.h"
#include "base/strings/string16.h"
#include "base/time/time.h"
#include "base/timer/timer.h"
#include "components/infobars/core/confirm_infobar_delegate.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/browser/web_contents_user_data.h"

class BloatedTabInfoBarDelegate;

class BloatedTabHelper : public content::WebContentsObserver,
                         public content::WebContentsUserData<BloatedTabHelper> {
 public:
  enum class Mode { kForeground, kBackground, kNone };

  ~BloatedTabHelper() override;

  // content::WebContentsObserver:
  void OnRendererBloated(content::RenderFrameHost* render_frame_host) override;
  void DidStartLoading() override;
  void DidFinishNavigation(
      content::NavigationHandle* navigation_handle) override;
  void WasShown() override;
  void WasHidden() override;

  void ReloadTab();

  void ClosedInfoBar(BloatedTabInfoBarDelegate* InfoBar);

  void OnTimer();

 private:
  friend class content::WebContentsUserData<BloatedTabHelper>;

  explicit BloatedTabHelper(content::WebContents* contents);

  void ShowInfoBar(Mode mode);
  void CloseInfoBar();

  Mode mode_;

  BloatedTabInfoBarDelegate* infobar_;

  base::Timer timer_;

  DISALLOW_COPY_AND_ASSIGN(BloatedTabHelper);
};

#endif  // CHROME_BROWSER_UI_BLOATED_TAB_HELPER_H_
