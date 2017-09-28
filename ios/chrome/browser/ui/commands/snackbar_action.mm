// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/commands/snackbar_action.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@implementation SnackbarAction
@synthesize title = _title;
@synthesize accessibilityIdentifier = _accessibilityIdentifier;
@synthesize accessibilityLabel = _accessibilityLabel;
@synthesize handler = _handler;
@end
