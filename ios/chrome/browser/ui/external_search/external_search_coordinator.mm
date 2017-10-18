// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/external_search/external_search_coordinator.h"

#import "ios/chrome/browser/ui/commands/command_dispatcher.h"
#import "ios/chrome/browser/ui/commands/external_search_commands.h"
#include "ios/public/provider/chrome/browser/chrome_browser_provider.h"
#include "ios/public/provider/chrome/browser/external_search/external_search_provider.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@implementation ExternalSearchCoordinator

@synthesize application = _application;
@synthesize dispatcher = _dispatcher;

- (UIApplication*)application {
  return _application ?: UIApplication.sharedApplication;
}

- (void)setDispatcher:(CommandDispatcher*)dispatcher {
  if (dispatcher == self.dispatcher)
    return;
  if (self.dispatcher)
    [self.dispatcher stopDispatchingToTarget:self];
  [dispatcher startDispatchingToTarget:self
                           forProtocol:@protocol(ExternalSearchCommands)];
  _dispatcher = dispatcher;
}

- (void)disconnect {
  self.dispatcher = nil;
}

#pragma mark - ExternalSearchCommands

- (void)launchExternalSearch {
  NSURL* externalSearchAppLaunchURL = ios::GetChromeBrowserProvider()
                                          ->GetExternalSearchProvider()
                                          ->GetLaunchURL();
  if (@available(iOS 10, *)) {
    [self.application openURL:externalSearchAppLaunchURL
                      options:@{}
            completionHandler:nil];
  }
#if !defined(__IPHONE_10_0) || __IPHONE_OS_VERSION_MIN_REQUIRED < __IPHONE_10_0
  else {
    [self.application openURL:externalSearchAppLaunchURL];
  }
#endif
}

@end
