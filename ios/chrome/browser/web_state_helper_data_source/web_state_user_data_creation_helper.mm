// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/web_state_helper_data_source/web_state_user_data_creation_helper.h"

#import "ios/chrome/browser/find_in_page/find_tab_helper.h"
#import "ios/chrome/browser/sessions/ios_chrome_session_tab_helper.h"
#import "ios/chrome/browser/ssl/ios_security_state_tab_helper.h"
#import "ios/chrome/browser/web/tab_id_tab_helper.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

void WebStateUserDataCreationHelper::OnWebStateAdded(web::WebState* web_state) {
  FindTabHelper::CreateForWebState(web_state, nil);
  IOSChromeSessionTabHelper::CreateForWebState(web_state);
  IOSSecurityStateTabHelper::CreateForWebState(web_state);
  TabIdTabHelper::CreateForWebState(web_state);
}
