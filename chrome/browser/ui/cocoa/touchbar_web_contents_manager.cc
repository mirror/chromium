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
#include "chrome/browser/ui/browser.h"
#include "chrome/common/chrome_features.h"
#include "content/public/browser/navigation_entry.h"

DEFINE_WEB_CONTENTS_USER_DATA_KEY(TouchbarWebContentsManager);

TouchbarWebContentsManager::TouchbarWebContentsManager(
    content::WebContents* web_contents)
    : content::WebContentsObserver(web_contents) {}

TouchbarWebContentsManager::~TouchbarWebContentsManager() {}

void TouchbarWebContentsManager::DisplayWebContentsInTouchbar(
    content::WebContents* popup) {
  Browser* delegate = static_cast<Browser*>(web_contents()->GetDelegate());
  if (IsTouchbarDinoGameEnabled() && IsErrorPage())
    // web_contents() is the parent of the popup.
    delegate->DisplayWebContentsInTouchbar(popup);
}

bool TouchbarWebContentsManager::ShouldHidePopup(
    WindowOpenDisposition disposition) {
  return (disposition == WindowOpenDisposition::NEW_POPUP && web_contents() &&
          IsErrorPage() && IsTouchbarDinoGameEnabled());
}

void TouchbarWebContentsManager::DidFinishNavigation(
    content::NavigationHandle* navigation_handle) {
  Browser* delegate = static_cast<Browser*>(web_contents()->GetDelegate());
  if (delegate && navigation_handle && !navigation_handle->IsErrorPage())
    delegate->DisplayWebContentsInTouchbar(nullptr);
}

void TouchbarWebContentsManager::WasShown() {
  Browser* delegate = static_cast<Browser*>(web_contents()->GetDelegate());
  if (delegate) {
    if (IsErrorPage() && GetPopupContents())
      delegate->DisplayWebContentsInTouchbar(GetPopupContents());
    else
      delegate->DisplayWebContentsInTouchbar(nullptr);
  }
}

bool TouchbarWebContentsManager::IsTouchbarDinoGameEnabled() {
  return base::FeatureList::IsEnabled(features::kTouchbarDinoGame);
}

bool TouchbarWebContentsManager::IsErrorPage() {
  return web_contents()
             ->GetController()
             .GetLastCommittedEntry()
             ->GetPageType() == content::PAGE_TYPE_ERROR;
}

void TouchbarWebContentsManager::SetIsErrorPage(bool is_error_page) {
  return;
}

content::WebContents* TouchbarWebContentsManager::GetPopupContents() {
  return popup_contents_.get();
}

void TouchbarWebContentsManager::SetPopupContents(
    content::WebContents* popup_contents) {
  popup_contents_.reset(popup_contents);
}
