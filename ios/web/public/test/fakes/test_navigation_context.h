// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_WEB_PUBLIC_TEST_FAKES_TEST_NAVIGATION_CONTEXT_H_
#define IOS_WEB_PUBLIC_TEST_FAKES_TEST_NAVIGATION_CONTEXT_H_

#import "ios/web/public/web_state/navigation_context.h"

namespace web {

// Minimal implementation of NavigationContext for use in tests.
class TestNavigationContext : public web::NavigationContext {
 public:
  TestNavigationContext();
  virtual ~TestNavigationContext();

  // NavigationContext:
  WebState* GetWebState() override;
  const GURL& GetUrl() const override;
  ui::PageTransition GetPageTransition() const override;
  bool IsSameDocument() const override;
  bool IsPost() const override;
  NSError* GetError() const override;
  net::HttpResponseHeaders* GetResponseHeaders() const override;
  bool IsRendererInitiated() const override;
};

}  // namespace web

#endif  // IOS_WEB_PUBLIC_TEST_FAKES_TEST_NAVIGATION_CONTEXT_H_
