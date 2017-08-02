// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/cocoa/touchbar_web_contents_manager.h"

#include <stddef.h>

#include <cmath>
#include <utility>
#include <vector>

#include "base/feature_list.h"
#include "build/build_config.h"
#include "content/public/common/content_features.h"

void TouchbarWebContentsManager::DisplayWebContentsInTouchbar(
    content::WebContents* popup) {
  if (IsTouchbarDinoGameEnabled() && web_contents()->IsErrorPage()) {
    content::WebContents* parent = static_cast<content::WebContents*>(
        WebContents::FromRenderFrameHost(popup->GetOpener()));

    if (parent) {
      web_contents()->SetPopupContents(popup);
      parent->GetDelegate()->DisplayWebContentsInTouchbar(popup);
    }
  }
}

bool TouchbarWebContentsManager::ShouldHidePopup(
    WindowOpenDisposition disposition) {
  return (disposition == WindowOpenDisposition::NEW_POPUP && web_contents() &&
          web_contents()->IsErrorPage() && IsTouchbarDinoGameEnabled());
}

void TouchbarWebContentsManager::DidFinishNavigation(
    content::NavigationHandle* navigation_handle) {
  content::WebContentsDelegate* delegate = web_contents()->GetDelegate();
  if (delegate && navigation_handle) {
    if (navigation_handle->IsErrorPage()) {
      web_contents()->SetIsErrorPage(true);
    } else {
      web_contents()->SetIsErrorPage(false);
      delegate->DisplayWebContentsInTouchbar(nullptr);
    }
  }
}

void TouchbarWebContentsManager::WasShown() {
  content::WebContentsDelegate* delegate = web_contents()->GetDelegate();

  if (delegate) {
    if (web_contents()->IsErrorPage() && web_contents()->GetPopupContents()) {
      delegate->DisplayWebContentsInTouchbar(
          web_contents()->GetPopupContents());
    } else {
      delegate->DisplayWebContentsInTouchbar(nullptr);
    }
  }
}

bool TouchbarWebContentsManager::IsTouchbarDinoGameEnabled() {
  return base::FeatureList::IsEnabled(features::kTouchbarDinoGame);
}
