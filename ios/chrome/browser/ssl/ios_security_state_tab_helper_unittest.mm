// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ssl/ios_security_state_tab_helper.h"

#import "components/security_state/core/security_state.h"
#import "ios/web/public/navigation_item.h"
#import "ios/web/public/navigation_manager.h"
#import "ios/web/public/ssl_status.h"
#import "ios/web/public/test/web_test_with_web_state.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

typedef web::WebTestWithWebState IOSSecurityStateTabHelperTest;
TEST_F(IOSSecurityStateTabHelperTest, SecurityInfoAfterEditing) {
  AddPendingItem(GURL("http://chromium.test"), ui::PAGE_TRANSITION_TYPED);
  web::NavigationItem* item =
      web_state()->GetNavigationManager()->GetVisibleItem();
  item->GetSSL().security_style = web::SECURITY_STYLE_UNAUTHENTICATED;

  IOSSecurityStateTabHelper::CreateForWebState(web_state());

  // Verify insecure_field_edited is not set prematurely.
  security_state::SecurityInfo info;
  IOSSecurityStateTabHelper::FromWebState(web_state())->GetSecurityInfo(&info);
  EXPECT_FALSE(info.insecure_input_events.insecure_field_edited);

  // Simulate an edit and verify insecure_field_edited is noted in the
  // insecure_input_events.
  item->GetSSL().content_status |= web::SSLStatus::EDITED_FIELD_ON_HTTP;
  IOSSecurityStateTabHelper::FromWebState(web_state())->GetSecurityInfo(&info);
  EXPECT_TRUE(info.insecure_input_events.insecure_field_edited);
}
