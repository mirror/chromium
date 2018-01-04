// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/app_launcher/app_launcher_coordinator.h"

#import <UIKit/UIKit.h>

#include "base/logging.h"
#include "base/metrics/histogram_macros.h"
#include "components/strings/grit/components_strings.h"
#import "ios/chrome/browser/open_url_util.h"
#include "ios/chrome/browser/procedural_block_types.h"
#import "ios/chrome/browser/ui/app_launcher/app_launcher_util.h"
#import "ios/chrome/browser/ui/app_launcher/open_mail_handler_view_controller.h"
#import "ios/chrome/browser/web/mailto_handler.h"
#import "ios/chrome/browser/web/mailto_handler_manager.h"
#include "ios/chrome/grit/ios_strings.h"
#include "ios/third_party/material_components_ios/src/components/BottomSheet/src/MDCBottomSheetController.h"
#import "net/base/mac/url_conversions.h"
#include "ui/base/l10n/l10n_util.h"
#include "url/gurl.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {

// Launches app identified by |url| if |accept| is true.
void LaunchApp(NSURL* url, bool accept) {
  UMA_HISTOGRAM_BOOLEAN("Tab.ExternalApplicationOpened", accept);
  if (accept)
    OpenUrlWithCompletionHandler(url, nil);
}

// Launches the mail client app represented by |handler| and records metrics.
void LaunchMailClientApp(const GURL& url, MailtoHandler* handler) {
  NSString* launch_url = [handler rewriteMailtoURL:url];
  UMA_HISTOGRAM_BOOLEAN("IOS.MailtoURLRewritten", launch_url != nil);
  NSURL* url_to_open = launch_url.length ? [NSURL URLWithString:launch_url]
                                         : net::NSURLWithGURL(url);
  [[UIApplication sharedApplication] openURL:url_to_open
                                     options:@{}
                           completionHandler:nil];
}

}  // namespace

@implementation AppLauncherCoordinator
@synthesize baseViewController = _baseViewController;

- (instancetype)initWithBaseViewController:
    (UIViewController*)baseViewController {
  if (self = [super init]) {
    _baseViewController = baseViewController;
  }
  return self;
}

#pragma mark - Private methods

// Alerts the user with |message| and buttons with titles
// |acceptActionTitle| and |rejectActionTitle|. |completionHandler| is called
// with a BOOL indicating whether the user has tapped the accept button.
- (void)showAlertWithMessage:(NSString*)message
           acceptActionTitle:(NSString*)acceptActionTitle
           rejectActionTitle:(NSString*)rejectActionTitle
           completionHandler:(ProceduralBlockWithBool)completionHandler {
  UIAlertController* alertController =
      [UIAlertController alertControllerWithTitle:nil
                                          message:message
                                   preferredStyle:UIAlertControllerStyleAlert];
  UIAlertAction* acceptAction =
      [UIAlertAction actionWithTitle:acceptActionTitle
                               style:UIAlertActionStyleDefault
                             handler:^(UIAlertAction* action) {
                               completionHandler(YES);
                             }];
  UIAlertAction* rejectAction =
      [UIAlertAction actionWithTitle:rejectActionTitle
                               style:UIAlertActionStyleCancel
                             handler:^(UIAlertAction* action) {
                               completionHandler(NO);
                             }];
  [alertController addAction:rejectAction];
  [alertController addAction:acceptAction];

  [self.baseViewController presentViewController:alertController
                                        animated:YES
                                      completion:nil];
}

// Shows an alert that the app will open in another application. If the user
// accepts, the |gURL| is launched.
- (void)showAlertThenLaunchAppStoreURL:(const GURL&)gURL {
  DCHECK(UrlHasAppStoreScheme(gURL));
  NSURL* URL = net::NSURLWithGURL(gURL);
  NSString* prompt = l10n_util::GetNSString(IDS_IOS_OPEN_IN_ANOTHER_APP);
  NSString* openLabel =
      l10n_util::GetNSString(IDS_IOS_APP_LAUNCHER_OPEN_APP_BUTTON_LABEL);
  NSString* cancelLabel = l10n_util::GetNSString(IDS_CANCEL);
  [self showAlertWithMessage:prompt
           acceptActionTitle:openLabel
           rejectActionTitle:cancelLabel
           completionHandler:^(BOOL userAccepted) {
             LaunchApp(URL, userAccepted);
           }];
}

// Shows an alert to launch a phone call or Facetime. The |gURL| is launched if
// the user accepts.
- (void)showAlertThenLaunchPhoneCallOrFacetimeURL:(const GURL&)gURL {
  DCHECK(UrlHasPhoneCallScheme(gURL));
  NSURL* URL = net::NSURLWithGURL(gURL);
  NSString* prompt = GetFormattedAbsoluteUrlWithSchemeRemoved(URL);
  NSString* openLabel = GetPromptActionString(URL.scheme);
  NSString* cancelLabel = l10n_util::GetNSString(IDS_CANCEL);
  [self showAlertWithMessage:prompt
           acceptActionTitle:openLabel
           rejectActionTitle:cancelLabel
           completionHandler:^(BOOL userAccepted) {
             LaunchApp(URL, userAccepted);
           }];
}

// Launches |gURL| in a mailto handler. If a default mailto handler does not
// exist, then a mail handler chooser is launched before the mailto handler
// is launched.
- (void)showAlertIfNeededThenLaunchMailtoURL:(const GURL&)gURL {
  DCHECK(gURL.SchemeIs(url::kMailToScheme));
  MailtoHandlerManager* manager =
      [MailtoHandlerManager mailtoHandlerManagerWithStandardHandlers];
  NSString* handlerID = manager.defaultHandlerID;
  if (handlerID) {
    MailtoHandler* handler = [manager defaultHandlerByID:handlerID];
    LaunchMailClientApp(gURL, handler);
    return;
  }
  GURL copiedURLToOpen = gURL;
  OpenMailHandlerViewController* mailHandlerChooser =
      [[OpenMailHandlerViewController alloc]
          initWithManager:manager
          selectedHandler:^(MailtoHandler* _Nonnull handler) {
            LaunchMailClientApp(copiedURLToOpen, handler);
          }];
  MDCBottomSheetController* bottomSheet = [[MDCBottomSheetController alloc]
      initWithContentViewController:mailHandlerChooser];
  [self.baseViewController presentViewController:bottomSheet
                                        animated:YES
                                      completion:nil];
}

#pragma mark - AppLauncherPresenter

- (BOOL)openURL:(const GURL&)gURL linkTapped:(BOOL)linkTapped {
  // Don't open application if chrome is not active.
  if ([[UIApplication sharedApplication] applicationState] !=
      UIApplicationStateActive) {
    return NO;
  }
  NSURL* URL = net::NSURLWithGURL(gURL);
  if (@available(iOS 10.3, *)) {
    if (UrlHasAppStoreScheme(gURL)) {
      [self showAlertThenLaunchAppStoreURL:gURL];
      return YES;
    }
  } else {
    // Prior to iOS 10.3, iOS does not prompt user when facetime: and
    // facetime-audio: URL schemes are opened, so Chrome needs to present an
    // alert before placing a phone call.
    if (UrlHasPhoneCallScheme(gURL)) {
      [self showAlertThenLaunchPhoneCallOrFacetimeURL:gURL];
      return YES;
    }
    // Prior to iOS 10.3, Chrome prompts user with an alert before opening
    // App Store when user did not tap on any links and an iTunes app URL is
    // opened. This maintains parity with Safari in pre-10.3 environment.
    if (!linkTapped && UrlHasAppStoreScheme(gURL)) {
      [self showAlertThenLaunchAppStoreURL:gURL];
      return YES;
    }
  }
  // Replaces |URL| with a rewritten URL if it is of mailto: scheme.
  if (gURL.SchemeIs(url::kMailToScheme)) {
    [self showAlertIfNeededThenLaunchMailtoURL:gURL];
    return true;
  }
// If the following call returns YES, an application is about to be
// launched and Chrome will go into the background now.
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
  // TODO(crbug.com/774736): This call still needs to be
  // updated. It's heavily nested so some refactoring is needed.
  return [[UIApplication sharedApplication] openURL:URL];
#pragma clang diagnostic pop
}

- (void)showAlertOfRepeatedLaunchesWithCompletionHandler:
    (ProceduralBlockWithBool)completionHandler {
  NSString* message =
      l10n_util::GetNSString(IDS_IOS_OPEN_REPEATEDLY_ANOTHER_APP);
  NSString* allowLaunchTitle =
      l10n_util::GetNSString(IDS_IOS_OPEN_REPEATEDLY_ANOTHER_APP_ALLOW);
  NSString* blockLaunchTitle =
      l10n_util::GetNSString(IDS_IOS_OPEN_REPEATEDLY_ANOTHER_APP_BLOCK);
  [self showAlertWithMessage:message
           acceptActionTitle:allowLaunchTitle
           rejectActionTitle:blockLaunchTitle
           completionHandler:^(BOOL userAllowed) {
             UMA_HISTOGRAM_BOOLEAN("IOS.RepeatedExternalAppPromptResponse",
                                   userAllowed);
             completionHandler(userAllowed);
           }];
}

@end
