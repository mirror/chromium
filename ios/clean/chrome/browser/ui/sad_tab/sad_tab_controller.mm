// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/clean/chrome/browser/ui/sad_tab/sad_tab_controller.h"

#import "ios/chrome/browser/ui/sad_tab/sad_tab_view.h"
#import "ios/web/public/web_state/ui/crw_generic_content_view.h"
#include "ios/web/public/web_state/web_state.h"

@interface SadTabController ()
// The web state this SadTabController is handling.
@property(nonatomic, assign) web::WebState* webState;
@end

@implementation SadTabController
@synthesize dispatcher = _dispatacher;
@synthesize webState = _webState;

- (instancetype)initWithWebState:(web::WebState*)webState {
  self = [super init];
  if (self) {
    self.webState = webState;
  }
  return self;
}

- (void)presentSadTabForRepeatedFailure:(BOOL)repeatedFailure {
  // Create a SadTabView so |webstate| presents it.
  SadTabView* sadTabview = [[SadTabView alloc]
           initWithMode:repeatedFailure ? SadTabViewMode::FEEDBACK
                                        : SadTabViewMode::RELOAD
      navigationManager:self.webState->GetNavigationManager()];
  sadTabview.dispatcher = self.dispatcher;
  CRWContentView* contentView =
      [[CRWGenericContentView alloc] initWithView:sadTabview];
  self.webState->ShowTransientContentView(contentView);
}

@end
