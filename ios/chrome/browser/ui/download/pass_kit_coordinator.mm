// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/download/pass_kit_coordinator.h"

#include "components/infobars/core/infobar_manager.h"
#include "components/infobars/core/simple_alert_infobar_delegate.h"
#include "ios/chrome/browser/infobars/infobar_manager_impl.h"
#include "ios/chrome/grit/ios_strings.h"
#include "ui/base/l10n/l10n_util.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@interface PassKitCoordinator () {
  UIViewController* _addPassViewController;
}
@end

@implementation PassKitCoordinator
@synthesize pass = _pass;
@synthesize webState = _webState;

- (void)start {
  if (self.pass) {
    [self presentAddPassUI];
  } else {
    [self presentErrorUI];
  }
}

#pragma mark - Private

// Presents PKAddPassesViewController.
- (void)presentAddPassUI {
  _addPassViewController =
      [[PKAddPassesViewController alloc] initWithPass:self.pass];
  [self.baseViewController presentViewController:_addPassViewController
                                        animated:YES
                                      completion:nil];
}

// Presents "failed to add pkpass" infobar.
- (void)presentErrorUI {
  infobars::InfoBarManager* infoBarManager =
      InfoBarManagerImpl::FromWebState(_webState);
  // TODO(crbug.com/227994): Infobar cleanup (infoBarManager should never be
  // NULL, replace if with DCHECK).
  if (!infoBarManager)
    return;

  SimpleAlertInfoBarDelegate::Create(
      infoBarManager,
      infobars::InfoBarDelegate::SHOW_PASSKIT_INFOBAR_ERROR_DELEGATE,
      /*vector_icon=*/nullptr,
      l10n_util::GetStringUTF16(IDS_IOS_GENERIC_PASSKIT_ERROR),
      /*auto_expire=*/true);
}

#pragma mark - PassKitTabHelperDelegate

- (void)passKitTabHelper:(PassKitTabHelper*)tabHelper
    presentDialogForPass:(PKPass*)pass
                webState:(web::WebState*)webState {
  self.pass = pass;
  self.webState = webState;
  [self start];
}

@end
