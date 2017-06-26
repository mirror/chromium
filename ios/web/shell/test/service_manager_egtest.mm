// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import <EarlGrey/EarlGrey.h>

#include "base/message_loop/message_loop.h"
#include "base/run_loop.h"
#import "ios/testing/wait_util.h"
#include "ios/web/public/service_manager_connection.h"
#import "ios/web/shell/test/earl_grey/shell_earl_grey.h"
#import "ios/web/shell/test/earl_grey/web_shell_test_case.h"
#include "services/service_manager/public/cpp/connector.h"
#include "services/test/echo/public/interfaces/echo.mojom.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {
// TODO(blundell): Use this.
const char* kTestInput = "Livingston, I presume";
bool gEchoStringCallbackCalled = false;

void OnEchoString(echo::mojom::EchoPtr echo, const std::string& echoed_input) {
  GREYAssert(kTestInput == echoed_input,
             @"Unexpected string passed to echo callback: %s",
             echoed_input.c_str());
  gEchoStringCallbackCalled = true;
}

void WaitForEchoStringCallback(std::string text) {
  GREYCondition* condition =
      [GREYCondition conditionWithName:@"Wait for echo string callback"
                                 block:^BOOL {
                                   return gEchoStringCallbackCalled;
                                 }];
  GREYAssert([condition waitWithTimeout:testing::kWaitForUIElementTimeout],
             @"Failed waiting for web view containing %s", text.c_str());
}
}

// Service Manager test cases for the web shell.
@interface ServiceManagerTestCase : WebShellTestCase
@end

@implementation ServiceManagerTestCase

// Tests MIME type of the loaded PDF document.
- (void)testConnectionToEmbeddedService {
  echo::mojom::EchoPtr echo;
  web::ServiceManagerConnection* connection =
      web::ServiceManagerConnection::GetForProcess();
  DCHECK(connection);
  connection->GetConnector()->BindInterface("echo", mojo::MakeRequest(&echo));
  echo::mojom::Echo* raw_echo = echo.get();

  raw_echo->EchoString(kTestInput,
                       base::BindOnce(&OnEchoString, base::Passed(&echo)));
  WaitForEchoStringCallback("foo");
}

@end
