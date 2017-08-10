// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ssl/captive_portal_detector_tab_helper.h"

#include "base/memory/ptr_util.h"
#include "components/captive_portal/captive_portal_detector.h"
#import "ios/chrome/browser/ssl/captive_portal_detector_tab_helper_delegate.h"
#include "ios/web/public/browser_state.h"
#import "ios/web/public/web_state/web_state.h"

DEFINE_WEB_STATE_USER_DATA_KEY(CaptivePortalDetectorTabHelper);

void CaptivePortalDetectorTabHelper::CreateForWebState(
    web::WebState* web_state,
    id<CaptivePortalDetectorTabHelperDelegate> delegate) {
  DCHECK(web_state);
  DCHECK(delegate);
  if (!FromWebState(web_state)) {
    web_state->SetUserData(UserDataKey(),
                           base::WrapUnique(new CaptivePortalDetectorTabHelper(
                               web_state, delegate)));
  }
}

captive_portal::CaptivePortalDetector*
CaptivePortalDetectorTabHelper::detector() {
  return detector_.get();
}

id<CaptivePortalDetectorTabHelperDelegate>
CaptivePortalDetectorTabHelper::delegate() {
  return delegate_;
}

CaptivePortalDetectorTabHelper::CaptivePortalDetectorTabHelper(
    web::WebState* web_state,
    id<CaptivePortalDetectorTabHelperDelegate> delegate)
    : detector_(base::MakeUnique<captive_portal::CaptivePortalDetector>(
          web_state->GetBrowserState()->GetRequestContext())),
      delegate_(delegate) {}

CaptivePortalDetectorTabHelper::~CaptivePortalDetectorTabHelper() = default;
