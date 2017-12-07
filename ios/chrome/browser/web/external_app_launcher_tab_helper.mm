// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/web/external_app_launcher_tab_helper.h"

#include "base/ios/ios_util.h"
#include "base/logging.h"
#include "base/metrics/histogram_macros.h"
#include "base/strings/sys_string_conversions.h"
#include "components/strings/grit/components_strings.h"
#import "ios/chrome/browser/open_url_util.h"
#include "ios/chrome/browser/procedural_block_types.h"
#import "ios/chrome/browser/ui/external_app/open_mail_handler_view_controller.h"
#import "ios/chrome/browser/web/external_apps_launch_policy_decider.h"
#import "ios/chrome/browser/web/mailto_handler.h"
#import "ios/chrome/browser/web/mailto_url_rewriter.h"
#import "ios/chrome/browser/web/nullable_mailto_url_rewriter.h"
#include "ios/chrome/grit/ios_strings.h"
#include "ios/third_party/material_components_ios/src/components/BottomSheet/src/MDCBottomSheetController.h"
#import "net/base/mac/url_conversions.h"
#include "ui/base/l10n/l10n_util.h"
#include "url/gurl.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

DEFINE_WEB_STATE_USER_DATA_KEY(ExternalAppLauncherTabHelper);

namespace {

// Returns a set of NSStrings that are URL schemes for iTunes stores.
NSSet<NSString*>* ItmsSchemes() {
  static NSSet<NSString*>* schemes;
  static dispatch_once_t once;
  dispatch_once(&once, ^{
    schemes = [NSSet<NSString*>
        setWithObjects:@"itms", @"itmss", @"itms-apps", @"itms-appss",
                       // There's no evidence that itms-bookss is actually
                       // supported, but over-inclusion costs less than
                       // under-inclusion.
                       @"itms-books", @"itms-bookss", nil];
  });
  return schemes;
}

// Returns whether |url| has a scheme for the iTunes stores.
bool UrlHasAppStoreScheme(const GURL& url) {
  std::string scheme = url.scheme();
  return [ItmsSchemes() containsObject:base::SysUTF8ToNSString(scheme)];
}

// Returns whether |url| has the scheme of a URL that initiates a call.
bool UrlHasPhoneCallScheme(const GURL& url) {
  return url.SchemeIs("tel") || url.SchemeIs("facetime") ||
         url.SchemeIs("facetime-audio");
}

// Returns a string to be used as the label for the prompt's action button.
NSString* PromptActionString(NSString* scheme) {
  if ([scheme isEqualToString:@"facetime"])
    return l10n_util::GetNSString(IDS_IOS_FACETIME_BUTTON);
  else if ([scheme isEqualToString:@"tel"] ||
           [scheme isEqualToString:@"facetime-audio"])
    return l10n_util::GetNSString(IDS_IOS_PHONE_CALL_BUTTON);
  NOTREACHED();
  return @"";
}

// Launches the mail client app represented by |handler| and records metrics.
void LaunchMailClientApp(const GURL& url, MailtoHandler* handler) {
  NSString* launch_url = [handler rewriteMailtoURL:url];
  UMA_HISTOGRAM_BOOLEAN("IOS.MailtoURLRewritten", launch_url != nil);
  NSURL* url_to_open = [launch_url length] ? [NSURL URLWithString:launch_url]
                                           : net::NSURLWithGURL(url);
  if (@available(iOS 10, *)) {
    [[UIApplication sharedApplication] openURL:url_to_open
                                       options:@{}
                             completionHandler:nil];
  }
#if !defined(__IPHONE_10_0) || __IPHONE_OS_VERSION_MIN_REQUIRED < __IPHONE_10_0
  else {
    [[UIApplication sharedApplication] openURL:url_to_open];
  }
#endif
}

// Returns the Phone/FaceTime call argument from |url|.
NSString* FormatCallArgument(NSURL* url) {
  NSCharacterSet* char_set =
      [NSCharacterSet characterSetWithCharactersInString:@"/"];
  NSString* scheme = [NSString stringWithFormat:@"%@:", [url scheme]];
  NSString* url_string = [url absoluteString];
  if ([url_string length] <= [scheme length])
    return url_string;
  NSString* prompt = [[[[url absoluteString] substringFromIndex:[scheme length]]
      stringByTrimmingCharactersInSet:char_set]
      stringByRemovingPercentEncoding];
  // Returns original URL string if there's nothing interesting to display
  // other than the scheme itself.
  if (![prompt length])
    return url_string;
  return prompt;
}

// TBD
void PromptForMailClientWithUrl(const GURL& url, MailtoURLRewriter* rewriter) {
  GURL copied_url_to_open = url;
  OpenMailHandlerViewController* mail_handler_chooser =
      [[OpenMailHandlerViewController alloc]
          initWithRewriter:rewriter
           selectedHandler:^(MailtoHandler* _Nonnull handler) {
             LaunchMailClientApp(copied_url_to_open, handler);
           }];
  MDCBottomSheetController* bottom_sheet = [[MDCBottomSheetController alloc]
      initWithContentViewController:mail_handler_chooser];
  [[[[UIApplication sharedApplication] keyWindow] rootViewController]
      presentViewController:bottom_sheet
                   animated:YES
                 completion:nil];
}

// TBD
void ShowExternalAppLauncherPrompt(NSString* prompt,
                                   NSString* accept_label,
                                   NSString* reject_label,
                                   ProceduralBlockWithBool response_handler) {
  UIAlertController* alert_controller =
      [UIAlertController alertControllerWithTitle:nil
                                          message:prompt
                                   preferredStyle:UIAlertControllerStyleAlert];
  UIAlertAction* accept_action =
      [UIAlertAction actionWithTitle:accept_label
                               style:UIAlertActionStyleDefault
                             handler:^(UIAlertAction* action) {
                               response_handler(YES);
                             }];
  UIAlertAction* reject_action =
      [UIAlertAction actionWithTitle:reject_label
                               style:UIAlertActionStyleCancel
                             handler:^(UIAlertAction* action) {
                               response_handler(NO);
                             }];
  [alert_controller addAction:reject_action];
  [alert_controller addAction:accept_action];

  [[[[UIApplication sharedApplication] keyWindow] rootViewController]
      presentViewController:alert_controller
                   animated:YES
                 completion:nil];
}

// TBD
void OpenExternalAppWithUrl(NSURL* url,
                            NSString* prompt,
                            NSString* open_label) {
  ShowExternalAppLauncherPrompt(
      prompt, open_label, l10n_util::GetNSString(IDS_CANCEL), ^(BOOL accept) {
        if (accept)
          OpenUrlWithCompletionHandler(url, nil);
        UMA_HISTOGRAM_BOOLEAN("Tab.ExternalApplicationOpened", accept);
      });
}

// TBD
BOOL OpenUrl(const GURL& gurl, BOOL link_clicked) {
  // Don't open external application if chrome is not active.
  if ([[UIApplication sharedApplication] applicationState] !=
      UIApplicationStateActive) {
    return NO;
  }

  NSURL* url = net::NSURLWithGURL(gurl);
  if (base::ios::IsRunningOnOrLater(10, 3, 0)) {
    if (UrlHasAppStoreScheme(gurl)) {
      NSString* prompt = l10n_util::GetNSString(IDS_IOS_OPEN_IN_ANOTHER_APP);
      NSString* open_label =
          l10n_util::GetNSString(IDS_IOS_APP_LAUNCHER_OPEN_APP_BUTTON_LABEL);
      OpenExternalAppWithUrl(url, prompt, open_label);
      return YES;
    }
  } else {
    // Prior to iOS 10.3, iOS does not prompt user when facetime: and
    // facetime-audio: URL schemes are opened, so Chrome needs to present an
    // alert before placing a phone call.
    if (UrlHasPhoneCallScheme(gurl)) {
      OpenExternalAppWithUrl(url, FormatCallArgument(url),
                             PromptActionString([url scheme]));
      return YES;
    }
    // Prior to iOS 10.3, Chrome prompts user with an alert before opening
    // App Store when user did not tap on any links and an iTunes app URL is
    // opened. This maintains parity with Safari in pre-10.3 environment.
    if (!link_clicked && UrlHasAppStoreScheme(gurl)) {
      NSString* prompt = l10n_util::GetNSString(IDS_IOS_OPEN_IN_ANOTHER_APP);
      NSString* open_label =
          l10n_util::GetNSString(IDS_IOS_APP_LAUNCHER_OPEN_APP_BUTTON_LABEL);
      OpenExternalAppWithUrl(url, prompt, open_label);
      return YES;
    }
  }

  // Replaces |gurl| with a rewritten URL if it is of mailto: scheme.
  if (gurl.SchemeIs(url::kMailToScheme)) {
    MailtoURLRewriter* rewriter =
        [NullableMailtoURLRewriter mailtoURLRewriterWithStandardHandlers];
    NSString* handler_id = [rewriter defaultHandlerID];
    if (!handler_id) {
      PromptForMailClientWithUrl(gurl, rewriter);
      return YES;
    }
    MailtoHandler* handler = [rewriter defaultHandlerByID:handler_id];
    LaunchMailClientApp(gurl, handler);
    return YES;
  }

// If the following call returns YES, an external application is about to be
// launched and Chrome will go into the background now.
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
  // TODO(crbug.com/774736): This call still needs to be
  // updated. It's heavily nested so some refactoring is needed.
  return [[UIApplication sharedApplication] openURL:url];
#pragma clang diagnostic pop
}

}  // namespace

ExternalAppLauncherTabHelper::ExternalAppLauncherTabHelper(
    web::WebState* web_state) {
  policy_decider_ = [[ExternalAppsLaunchPolicyDecider alloc] init];
}

ExternalAppLauncherTabHelper::~ExternalAppLauncherTabHelper() = default;

BOOL ExternalAppLauncherTabHelper::RequestToOpenUrl(const GURL& url,
                                                    const GURL& source_page_url,
                                                    BOOL link_clicked) {
  if (!url.is_valid() || !url.has_scheme())
    return NO;

  // Don't open external application if chrome is not active.
  if ([[UIApplication sharedApplication] applicationState] !=
      UIApplicationStateActive) {
    return NO;
  }

  // Don't try to open external application if a prompt is already active.
  if (prompt_active_)
    return NO;

  [policy_decider_ didRequestLaunchExternalAppURL:url
                                fromSourcePageURL:source_page_url];
  ExternalAppLaunchPolicy policy =
      [policy_decider_ launchPolicyForURL:url
                        fromSourcePageURL:source_page_url];
  switch (policy) {
    case ExternalAppLaunchPolicyBlock: {
      return NO;
    }
    case ExternalAppLaunchPolicyAllow: {
      return OpenUrl(url, link_clicked);
    }
    case ExternalAppLaunchPolicyPrompt: {
      prompt_active_ = YES;
      NSString* prompt_body =
          l10n_util::GetNSString(IDS_IOS_OPEN_REPEATEDLY_ANOTHER_APP);
      NSString* allow_label =
          l10n_util::GetNSString(IDS_IOS_OPEN_REPEATEDLY_ANOTHER_APP_ALLOW);
      NSString* block_label =
          l10n_util::GetNSString(IDS_IOS_OPEN_REPEATEDLY_ANOTHER_APP_BLOCK);

      ShowExternalAppLauncherPrompt(
          prompt_body, allow_label, block_label, ^(BOOL allowed) {
            if (allowed) {
              // By confirming that the user wants to launch the application,
              // there is no need to check for |link_clicked|.
              OpenUrl(url, YES /* link_clicked */);
            } else {
              // TODO(crbug.com/674649): Once non-modal dialogs are implemented,
              // update this to always prompt instead of blocking the app.
              [policy_decider_ blockLaunchingAppURL:url
                                  fromSourcePageURL:source_page_url];
            }
            UMA_HISTOGRAM_BOOLEAN("IOS.RepeatedExternalAppPromptResponse",
                                  allowed);
            prompt_active_ = NO;
          });
      return YES;
    }
  }
}
