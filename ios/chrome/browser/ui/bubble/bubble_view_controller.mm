// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/bubble/bubble_view_controller.h"

#include "base/logging.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@implementation BubbleViewController

- (instancetype)initWithText:(NSString*)text
                   direction:(BubbleArrowDirection)arrowDirection
                   alignment:(BubbleAlignment)alignment {
  self = [super initWithNibName:nil bundle:nil];
  self.view = [[BubbleView alloc] initWithText:text
                                     direction:arrowDirection
                                     alignment:alignment];
  return self;
}

- (void)animateContentIn {
  NOTIMPLEMENTED();
}

- (void)dismissAnimated:(BOOL)animated {
  NOTIMPLEMENTED();
}

@end
