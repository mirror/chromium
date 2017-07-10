// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

#import "remoting/ios/app/user_status_presenter.h"

#import "ios/third_party/material_components_ios/src/components/Snackbar/src/MaterialSnackbar.h"
#import "remoting/ios/facade/remoting_authentication.h"
#import "remoting/ios/facade/remoting_service.h"

static UserStatusPresenter* g_userStatusPresenter;

@implementation UserStatusPresenter

#pragma mark - Public Static

+ (void)start {
  g_userStatusPresenter = [[UserStatusPresenter alloc] init];
}

+ (void)stop {
  g_userStatusPresenter = nil;
}

#pragma mark - Private

- (instancetype)init {
  if ([RemotingService.instance.authentication.user isAuthenticated]) {
    [self presentUserStatus];
  }
  [[NSNotificationCenter defaultCenter]
      addObserver:self
         selector:@selector(userDidUpdateNotification:)
             name:kUserDidUpdate
           object:nil];
  return self;
}

- (void)dealloc {
  [[NSNotificationCenter defaultCenter] removeObserver:self];
}

- (void)userDidUpdateNotification:(NSNotification*)notification {
  [self presentUserStatus];
}

- (void)presentUserStatus {
  UserInfo* user = RemotingService.instance.authentication.user;
  if (![user isAuthenticated]) {
    return;
  }
  MDCSnackbarMessage* message = [[MDCSnackbarMessage alloc] init];
  message.text =
      [NSString stringWithFormat:@"Currently signed in as %@.", user.userEmail];
  [MDCSnackbarManager showMessage:message];
}

@end
