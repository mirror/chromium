// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/gr_search/gr_search_coordinator.h"

#import "ios/chrome/browser/ui/commands/command_dispatcher.h"
#import "ios/chrome/browser/ui/commands/gr_search_commands.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@implementation GRSearchCoordinator

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
                           forProtocol:@protocol(GRSearchCommands)];
  _dispatcher = dispatcher;
}

- (void)disconnect {
  [self.dispatcher stopDispatchingToTarget:self];
  self.dispatcher = nil;
}

#pragma mark - GRSearchCommands

- (void)launchGRSearch {
  NSURL* grSearchAppLaunchURL = [NSURL URLWithString:@"gr://"];
  if (@available(iOS 10, *)) {
    [self.application openURL:grSearchAppLaunchURL
                      options:@{}
            completionHandler:nil];
  } else {
    [UIApplication.sharedApplication openURL:grSearchAppLaunchURL];
  }
}

@end
