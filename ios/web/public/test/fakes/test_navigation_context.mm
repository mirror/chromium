// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/web/public/test/fakes/test_navigation_context.h"

#include "url/gurl.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace web {

TestNavigationContext::TestNavigationContext() = default;
TestNavigationContext::~TestNavigationContext() = default;

WebState* TestNavigationContext::GetWebState() {
  return nullptr;
}

const GURL& TestNavigationContext::GetUrl() const {
  return GURL::EmptyGURL();
}

ui::PageTransition TestNavigationContext::GetPageTransition() const {
  return ui::PAGE_TRANSITION_LINK;
}

bool TestNavigationContext::IsSameDocument() const {
  return false;
}

bool TestNavigationContext::IsPost() const {
  return false;
}

NSError* TestNavigationContext::GetError() const {
  return nil;
}

net::HttpResponseHeaders* TestNavigationContext::GetResponseHeaders() const {
  return nullptr;
}

bool TestNavigationContext::IsRendererInitiated() const {
  return false;
}

}  // namespace web
