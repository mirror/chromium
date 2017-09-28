// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/passwords/notify_auto_signin_view_controller.h"

#import "base/mac/bind_objc_block.h"
#include "base/strings/sys_string_conversions.h"
#include "components/image_fetcher/core/image_fetcher_impl.h"
#include "components/image_fetcher/ios/ios_image_decoder_impl.h"
#include "components/strings/grit/components_strings.h"
#import "ios/chrome/browser/ui/rtl_geometry.h"
#include "ios/chrome/browser/ui/uikit_ui_util.h"
#import "ios/chrome/browser/ui/util/constraints_ui_util.h"
#import "ios/third_party/material_components_ios/src/components/Typography/src/MaterialTypography.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/gfx/image/image.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {

constexpr int kBackgroundColor = 0x4285F4;
constexpr int kViewHeight = 48;
constexpr double kAnimationDuration = 0.5;  // seconds

// NetworkTrafficAnnotationTag for fetching avatar.
const net::NetworkTrafficAnnotationTag kTrafficAnnotation =
    net::DefineNetworkTrafficAnnotation("credential_avatar",
                                        R"(
        semantics {
          sender: "Chrome Password Manager"
          description:
            "Every credential saved in Chromium via the Credential Management "
            "API can have an avatar URL. The URL is essentially provided by "
            "the site calling the API. The avatar is used in the account "
            "chooser UI and auto signin toast which appear when a site calls "
            "navigator.credentials.get(). The avatar is retrieved before "
            "showing the UI."
          trigger:
            "User visits a site that calls navigator.credentials.get(). "
            "Assuming there are matching credentials in the Chromium password "
            "store, the avatars are retrieved."
          data: "Only avatar URL, no user data."
          destination: WEBSITE
        }
        policy {
          cookies_allowed: NO
          setting:
            "One can disable saving new credentials in the settings (see "
            "'Passwords and forms'). There is no setting to disable the API."
          chrome_policy {
            PasswordManagerEnabled {
              PasswordManagerEnabled: false
            }
          }
        })");

}  // namespace

@interface NotifyUserAutoSigninViewController () {
  std::unique_ptr<image_fetcher::ImageFetcher> _imageFetcher;
}

// Username, corresponding to Credential.id field in JS.
@property(copy, nonatomic) NSString* username;
// URL to user's avatar, corresponding to Credential.iconURL field in JS.
@property(assign, nonatomic) GURL iconURL;
// Image view displaying user's avatar (if fetched) or placeholder icon.
@property(nonatomic, strong) UIImageView* avatarView;
// Bottom anchor constraint is stored for convenience, to animate appearance and
// disappearance.
@property(nonatomic, strong) NSLayoutConstraint* bottomConstraint;
// Boolean indicating whether the snackbar is currently being displayed. It is
// used to prevent didMoveToParentViewController: being called twice when
// animating appearance. That way, constraints are not duplicated.
@property(nonatomic, assign, getter=isPresented) BOOL presented;

@end

@implementation NotifyUserAutoSigninViewController

@synthesize avatarView = _avatarView;
@synthesize bottomConstraint = _bottomConstraint;
@synthesize iconURL = _iconURL;
@synthesize username = _username;
@synthesize presented = _presented;

#pragma mark - Public

- (instancetype)initWithUsername:(NSString*)username
                         iconURL:(GURL)iconURL
                   contextGetter:(net::URLRequestContextGetter*)contextGetter {
  self = [super initWithNibName:nil bundle:nil];
  if (self) {
    _username = username;
    _iconURL = iconURL;
    _imageFetcher = std::make_unique<image_fetcher::ImageFetcherImpl>(
        image_fetcher::CreateIOSImageDecoder(), contextGetter);
    _presented = NO;
  }
  return self;
}

- (void)viewDidLoad {
  [super viewDidLoad];

  self.view.backgroundColor = UIColorFromRGB(kBackgroundColor);
  self.view.translatesAutoresizingMaskIntoConstraints = NO;

  // Blue background view for notification.
  UIView* contentView = [[UIView alloc] init];
  contentView.translatesAutoresizingMaskIntoConstraints = NO;
  contentView.userInteractionEnabled = NO;

  // View containing "Signing in as ..." text.
  UILabel* textView = [[UILabel alloc] init];
  textView.translatesAutoresizingMaskIntoConstraints = NO;
  UIFont* font = [MDCTypography body1Font];
  textView.text =
      l10n_util::GetNSStringF(IDS_MANAGE_PASSWORDS_AUTO_SIGNIN_TITLE,
                              base::SysNSStringToUTF16(_username));
  textView.font = font;
  textView.textColor = [UIColor whiteColor];
  textView.userInteractionEnabled = NO;

  // Load the placeholder user's avatar.
  UIImage* placeholderAvatar = [UIImage imageNamed:@"ic_account_circle"];
  // View containing user's avatar.
  self.avatarView = [[UIImageView alloc] initWithImage:placeholderAvatar];
  self.avatarView.translatesAutoresizingMaskIntoConstraints = NO;
  self.avatarView.userInteractionEnabled = NO;

  // Add subiews.
  [contentView addSubview:textView];
  [contentView addSubview:self.avatarView];
  [self.view addSubview:contentView];

  // Text view must leave 48pt on the left for user's avatar. Set the
  // constraints.
  NSDictionary* childrenViewsDictionary = @{
    @"text" : textView,
    @"avatar" : self.avatarView,
  };
  NSArray* childrenConstraints = @[
    @"V:|[text]|",
    @"V:|-12-[avatar(==24)]-12-|",
    @"H:|-12-[avatar(==24)]-12-[text]-12-|",
  ];
  ApplyVisualConstraints(childrenConstraints, childrenViewsDictionary);

  PinToSafeArea(contentView, self.view);

  // Fetch user's avatar and update displayed image.
  if (self.iconURL.is_valid()) {
    __weak NotifyUserAutoSigninViewController* weakSelf = self;
    _imageFetcher->StartOrQueueNetworkRequest(
        _iconURL.spec(), _iconURL,
        base::BindBlockArc(^(const std::string& id, const gfx::Image& image,
                             const image_fetcher::RequestMetadata& metadata) {
          if (!image.IsEmpty()) {
            weakSelf.avatarView.image = [image.ToUIImage() copy];
          }
        }),
        kTrafficAnnotation);
  }
}

- (void)beginDisappearanceTransition {
  self.bottomConstraint.constant = kViewHeight;
  [UIView animateWithDuration:kAnimationDuration
      animations:^{
        [self.view.superview layoutIfNeeded];
      }
      completion:^(BOOL finished) {
        // Remove view and its controller after the animation.
        [self willMoveToParentViewController:nil];
        [self.view removeFromSuperview];
        [self removeFromParentViewController];
      }];
}

- (void)didMoveToParentViewController:(UIViewController*)parent {
  if (parent != nil && self.presented) {
    // If the view is already presented, don't try to animate it second time.
    return;
  }
  [super didMoveToParentViewController:parent];
  if (parent == nil) {
    self.presented = NO;
    return;
  }

  self.presented = YES;
  [self beginAppearanceTransition];
}

#pragma mark - Private

// Animates the snackbar appearance.
- (void)beginAppearanceTransition {
  // At first self.view will be hidden under the screen and then animate to
  // appear from below.
  self.bottomConstraint = [self.view.bottomAnchor
      constraintEqualToAnchor:self.view.superview.bottomAnchor
                     constant:kViewHeight];
  // Set constraints for blue background.
  [NSLayoutConstraint activateConstraints:@[
    self.bottomConstraint,
    [self.view.leadingAnchor
        constraintEqualToAnchor:self.view.superview.leadingAnchor],
    [self.view.trailingAnchor
        constraintEqualToAnchor:self.view.superview.trailingAnchor],
  ]];

  // Commit all pending layout changes.
  [self.view layoutIfNeeded];
  [self.view.superview layoutIfNeeded];

  // Animate bottomAnchor offset.
  self.bottomConstraint.constant = 0;
  [UIView animateWithDuration:kAnimationDuration
                   animations:^{
                     [self.view.superview layoutIfNeeded];
                   }];
}

@end
