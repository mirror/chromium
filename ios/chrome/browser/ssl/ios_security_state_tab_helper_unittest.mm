// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ssl/ios_security_state_tab_helper.h"

#import "base/strings/sys_string_conversions.h"
#import "components/security_state/core/security_state.h"
#import "ios/chrome/browser/ssl/insecure_input_web_state_observer.h"
#import "ios/web/public/navigation_item.h"
#import "ios/web/public/navigation_manager.h"
#import "ios/web/public/ssl_status.h"
#import "ios/web/public/test/web_test_with_web_state.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

typedef web::WebTestWithWebState IOSSecurityStateTabHelperTest;
TEST_F(IOSSecurityStateTabHelperTest, SecurityInfoAfterEditing) {
  LoadHtml(base::SysUTF8ToNSString("<html><body></body></html>"),
           GURL("http://chromium.test"));

  IOSSecurityStateTabHelper::CreateForWebState(web_state());
  InsecureInputWebStateObserver::CreateForWebState(web_state());
  InsecureInputWebStateObserver* insecure_input_observer =
      InsecureInputWebStateObserver::FromWebState(web_state());
  ASSERT_NE(nullptr, insecure_input_observer);

  // Verify |insecure_field_edited| is not set prematurely.
  security_state::SecurityInfo info;
  IOSSecurityStateTabHelper::FromWebState(web_state())->GetSecurityInfo(&info);
  EXPECT_FALSE(info.insecure_input_events.insecure_field_edited);

  // Simulate an edit and verify |insecure_field_edited| is noted in the
  // insecure_input_events.
  insecure_input_observer->DidEditFieldInInsecureContext();
  IOSSecurityStateTabHelper::FromWebState(web_state())->GetSecurityInfo(&info);
  EXPECT_TRUE(info.insecure_input_events.insecure_field_edited);
}

TEST_F(IOSSecurityStateTabHelperTest, SecurityInfoWithInsecurePasswordField) {
  LoadHtml(base::SysUTF8ToNSString("<html><body></body></html>"),
           GURL("http://chromium.test"));

  IOSSecurityStateTabHelper::CreateForWebState(web_state());
  InsecureInputWebStateObserver::CreateForWebState(web_state());
  InsecureInputWebStateObserver* insecure_input_observer =
      InsecureInputWebStateObserver::FromWebState(web_state());
  ASSERT_NE(nullptr, insecure_input_observer);

  // Verify |password_field_shown| is not set prematurely.
  security_state::SecurityInfo info;
  IOSSecurityStateTabHelper::FromWebState(web_state())->GetSecurityInfo(&info);
  EXPECT_FALSE(info.insecure_input_events.password_field_shown);

  // Simulate a password field display and verify |password_field_shown|
  // is noted in the insecure_input_events.
  insecure_input_observer->DidShowPasswordFieldInInsecureContext();
  IOSSecurityStateTabHelper::FromWebState(web_state())->GetSecurityInfo(&info);
  EXPECT_TRUE(info.insecure_input_events.password_field_shown);
}

TEST_F(IOSSecurityStateTabHelperTest, SecurityInfoWithInsecureCreditCardField) {
  LoadHtml(base::SysUTF8ToNSString("<html><body></body></html>"),
           GURL("http://chromium.test"));

  IOSSecurityStateTabHelper::CreateForWebState(web_state());
  InsecureInputWebStateObserver::CreateForWebState(web_state());
  InsecureInputWebStateObserver* insecure_input_observer =
      InsecureInputWebStateObserver::FromWebState(web_state());
  ASSERT_NE(nullptr, insecure_input_observer);

  // Verify |credit_card_field_edited| is not set prematurely.
  security_state::SecurityInfo info;
  IOSSecurityStateTabHelper::FromWebState(web_state())->GetSecurityInfo(&info);
  EXPECT_FALSE(info.insecure_input_events.credit_card_field_edited);

  // Simulate a credit card field display and verify |credit_card_field_edited|
  // is noted in the insecure_input_events.
  insecure_input_observer->DidInteractWithNonsecureCreditCardInput();
  IOSSecurityStateTabHelper::FromWebState(web_state())->GetSecurityInfo(&info);
  EXPECT_TRUE(info.insecure_input_events.credit_card_field_edited);
}
