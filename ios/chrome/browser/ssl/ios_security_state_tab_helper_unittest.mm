// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ssl/ios_security_state_tab_helper.h"

#include "components/security_state/core/security_state.h"
#import "ios/chrome/browser/ssl/insecure_input_tab_helper.h"
#import "ios/web/public/navigation_item.h"
#import "ios/web/public/navigation_manager.h"
#include "ios/web/public/ssl_status.h"
#import "ios/web/public/test/web_test_with_web_state.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

typedef web::WebTestWithWebState IOSSecurityStateTabHelperTest;

// Ensure that |insecure_field_edited| is set only when an editing event has
// been reported.
TEST_F(IOSSecurityStateTabHelperTest, SecurityInfoAfterEditing) {
  LoadHtml(@"<html><body></body></html>", GURL("http://chromium.test"));

  IOSSecurityStateTabHelper::CreateForWebState(web_state());
  InsecureInputTabHelper::CreateForWebState(web_state());
  InsecureInputTabHelper* insecure_input =
      InsecureInputTabHelper::FromWebState(web_state());
  ASSERT_TRUE(insecure_input);

  // Verify |insecure_field_edited| is not set prematurely.
  security_state::SecurityInfo info;
  IOSSecurityStateTabHelper::FromWebState(web_state())->GetSecurityInfo(&info);
  EXPECT_FALSE(info.insecure_input_events.insecure_field_edited);

  // Simulate an edit and verify |insecure_field_edited| is noted in the
  // insecure_input_events.
  insecure_input->DidEditFieldInInsecureContext();
  IOSSecurityStateTabHelper::FromWebState(web_state())->GetSecurityInfo(&info);
  EXPECT_TRUE(info.insecure_input_events.insecure_field_edited);
}

// Ensure that |password_field_shown| is set only when a password field has
// been reported.
TEST_F(IOSSecurityStateTabHelperTest, SecurityInfoWithInsecurePasswordField) {
  LoadHtml(@"<html><body></body></html>", GURL("http://chromium.test"));

  IOSSecurityStateTabHelper::CreateForWebState(web_state());
  InsecureInputTabHelper::CreateForWebState(web_state());
  InsecureInputTabHelper* insecure_input =
      InsecureInputTabHelper::FromWebState(web_state());
  ASSERT_TRUE(insecure_input);

  // Verify |password_field_shown| is not set prematurely.
  security_state::SecurityInfo info;
  IOSSecurityStateTabHelper::FromWebState(web_state())->GetSecurityInfo(&info);
  EXPECT_FALSE(info.insecure_input_events.password_field_shown);

  // Simulate a password field display and verify |password_field_shown|
  // is noted in the insecure_input_events.
  insecure_input->DidShowPasswordFieldInInsecureContext();
  IOSSecurityStateTabHelper::FromWebState(web_state())->GetSecurityInfo(&info);
  EXPECT_TRUE(info.insecure_input_events.password_field_shown);
}

// Ensure that |credit_card_field_edited| is set only when a credit card field
// interaction has been reported.
TEST_F(IOSSecurityStateTabHelperTest, SecurityInfoWithInsecureCreditCardField) {
  LoadHtml(@"<html><body></body></html>", GURL("http://chromium.test"));

  IOSSecurityStateTabHelper::CreateForWebState(web_state());
  InsecureInputTabHelper::CreateForWebState(web_state());
  InsecureInputTabHelper* insecure_input =
      InsecureInputTabHelper::FromWebState(web_state());
  ASSERT_NE(nullptr, insecure_input);

  // Verify |credit_card_field_edited| is not set prematurely.
  security_state::SecurityInfo info;
  IOSSecurityStateTabHelper::FromWebState(web_state())->GetSecurityInfo(&info);
  EXPECT_FALSE(info.insecure_input_events.credit_card_field_edited);

  // Simulate a credit card field display and verify |credit_card_field_edited|
  // is noted in the insecure_input_events.
  insecure_input->DidInteractWithNonsecureCreditCardInput();
  IOSSecurityStateTabHelper::FromWebState(web_state())->GetSecurityInfo(&info);
  EXPECT_TRUE(info.insecure_input_events.credit_card_field_edited);
}
