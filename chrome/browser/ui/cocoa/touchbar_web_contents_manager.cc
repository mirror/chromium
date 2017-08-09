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
#include "chrome/common/chrome_features.h"

DEFINE_WEB_CONTENTS_USER_DATA_KEY(TouchbarWebContentsManager);

TouchbarWebContentsManager::TouchbarWebContentsManager(
    WebContents* web_contents)
    : WebContentsObserver(web_contents) {}

TouchbarWebContentsManager::~TouchbarWebContentsManager() {}

void TouchbarWebContentsManager::DisplayWebContentsInTouchbar(
    content::WebContents* popup) {
  if (IsTouchbarDinoGameEnabled() && IsErrorPage()) {
    content::WebContents* parent = static_cast<content::WebContents*>(
        WebContents::FromRenderFrameHost(popup->GetOpener()));

    if (parent) {
      SetPopupContents(popup);
      parent->GetDelegate()->DisplayWebContentsInTouchbar(popup);
    }
  }
}

bool TouchbarWebContentsManager::ShouldHidePopup(
    WindowOpenDisposition disposition) {
  return (disposition == WindowOpenDisposition::NEW_POPUP && web_contents() &&
          IsErrorPage() && IsTouchbarDinoGameEnabled());
}

void TouchbarWebContentsManager::DidFinishNavigation(
    content::NavigationHandle* navigation_handle) {
  content::WebContentsDelegate* delegate = web_contents()->GetDelegate();
  if (delegate && navigation_handle) {
    if (navigation_handle->IsErrorPage()) {
      SetIsErrorPage(true);
    } else {
      SetIsErrorPage(false);
      delegate->DisplayWebContentsInTouchbar(nullptr);
    }
  }
}

void TouchbarWebContentsManager::WasShown() {
  content::WebContentsDelegate* delegate = web_contents()->GetDelegate();
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
  return is_error_page_;
}

void TouchbarWebContentsManager::SetIsErrorPage(bool is_error_page) {
  is_error_page_ = is_error_page;
}

WebContents* TouchbarWebContentsManager::GetPopupContents() {
  return popup_contents_.get();
}

void TouchbarWebContentsManager::SetPopupContents(WebContents* popup_contents) {
  popup_contents_.reset(popup_contents);
}
