// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/web/web_state/ui/html_element_fetch_request.h"

#include "base/time/time.h"
#include "ios/web/web_state/context_menu_constants.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "testing/gtest_mac.h"
#include "testing/platform_test.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace web {

using HtmlElementFetchRequestTest = PlatformTest;

// Tests that |creationTime| is set at HtmlElementFetchRequest object creation.
TEST_F(HtmlElementFetchRequestTest, CreationTime) {
  HTMLElementFetchRequest* request =
      [[HTMLElementFetchRequest alloc] initWithFoundElementHandler:nil];
  base::TimeDelta delta = base::TimeTicks::Now() - request.creationTime;
  // Validate that |request.creationTime| is "now", but only use second
  // precision to avoid performance induced test flake.
  EXPECT_GT(1, delta.InSeconds());
}

// Tests that |runHandlerWithResponse:| runs the handler from the object's
// initializer with the expected |response|.
TEST_F(HtmlElementFetchRequestTest, RunHandler) {
  __block bool handler_called = false;
  __block NSDictionary* received_response = nil;
  void (^handler)(NSDictionary*) = ^(NSDictionary* response) {
    handler_called = true;
    received_response = response;
  };
  HTMLElementFetchRequest* request =
      [[HTMLElementFetchRequest alloc] initWithFoundElementHandler:handler];
  NSDictionary* response = @{kContextMenuElementInnerText : @"text"};
  [request runHandlerWithResponse:response];
  EXPECT_TRUE(handler_called);
  EXPECT_NSEQ(response, received_response);
}

// Tests that two HTMLElementFetchRequest objects have unique identifiers.
TEST_F(HtmlElementFetchRequestTest, UniqueIdentifier) {
  HTMLElementFetchRequest* request1 =
      [[HTMLElementFetchRequest alloc] initWithFoundElementHandler:nil];
  EXPECT_TRUE(request1.identifier.length > 0);
  HTMLElementFetchRequest* request2 =
      [[HTMLElementFetchRequest alloc] initWithFoundElementHandler:nil];
  EXPECT_TRUE(request2.identifier.length > 0);
  EXPECT_NSNE(request1.identifier, request2.identifier);
}

}  // namespace web
