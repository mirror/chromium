// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/settings/about_chrome_collection_view_controller.h"

#import "base/ios/block_types.h"
#include "base/logging.h"
#import "base/mac/foundation_util.h"
#include "base/strings/sys_string_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "components/version_info/version_info.h"
#include "ios/chrome/browser/chrome_url_constants.h"
#import "ios/chrome/browser/ui/collection_view/cells/collection_view_item.h"
#import "ios/chrome/browser/ui/collection_view/cells/collection_view_text_item.h"
#import "ios/chrome/browser/ui/collection_view/collection_view_model.h"
#import "ios/chrome/browser/ui/settings/cells/version_item.h"
#import "ios/chrome/browser/ui/settings/settings_utils.h"
#include "ios/chrome/browser/ui/uikit_ui_util.h"
#include "ios/chrome/common/channel_info.h"
#include "ios/chrome/grit/ios_chromium_strings.h"
#include "ios/chrome/grit/ios_strings.h"
#import "ios/third_party/material_components_ios/src/components/CollectionCells/src/MaterialCollectionCells.h"
#import "ios/third_party/material_components_ios/src/components/Snackbar/src/MaterialSnackbar.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/l10n/l10n_util_mac.h"
#include "url/gurl.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {

typedef NS_ENUM(NSInteger, SectionIdentifier) {
  SectionIdentifierLinks = kSectionIdentifierEnumZero,
  SectionIdentifierFooter,
};

typedef NS_ENUM(NSInteger, ItemType) {
  ItemTypeLinksCredits = kItemTypeEnumZero,
  ItemTypeLinksTerms,
  ItemTypeLinksPrivacy,
  ItemTypeVersion,
};

}  // namespace

@interface AboutChromeCollectionViewController ()
@property(strong, nonatomic)
    CollectionViewModel<CollectionViewItem*>* collectionViewModel;
@end

@implementation AboutChromeCollectionViewController
@synthesize collectionViewModel = _collectionViewModel;

#pragma mark Initialization

- (instancetype)init {
  self = [super initWithStyle:UITableViewStylePlain];
  if (self) {
    self.collectionViewModel = [[CollectionViewModel alloc] init];
    [self loadModel];
    self.title = l10n_util::GetNSString(IDS_IOS_ABOUT_PRODUCT_NAME);
  }
  return self;
}

#pragma mark SettingsRootCollectionViewController

- (void)loadModel {
  [self.collectionViewModel addSectionWithIdentifier:SectionIdentifierLinks];

  CollectionViewTextItem* credits =
      [[CollectionViewTextItem alloc] initWithType:ItemTypeLinksCredits];
  credits.text = l10n_util::GetNSString(IDS_IOS_OPEN_SOURCE_LICENSES);
  credits.accessoryType = MDCCollectionViewCellAccessoryDisclosureIndicator;
  credits.accessibilityTraits = UIAccessibilityTraitButton;
  [self.collectionViewModel addItem:credits
            toSectionWithIdentifier:SectionIdentifierLinks];

  CollectionViewTextItem* terms =
      [[CollectionViewTextItem alloc] initWithType:ItemTypeLinksTerms];
  terms.text = l10n_util::GetNSString(IDS_IOS_TERMS_OF_SERVICE);
  terms.accessoryType = MDCCollectionViewCellAccessoryDisclosureIndicator;
  terms.accessibilityTraits = UIAccessibilityTraitButton;
  [self.collectionViewModel addItem:terms
            toSectionWithIdentifier:SectionIdentifierLinks];

  CollectionViewTextItem* privacy =
      [[CollectionViewTextItem alloc] initWithType:ItemTypeLinksPrivacy];
  privacy.text = l10n_util::GetNSString(IDS_IOS_PRIVACY_POLICY);
  privacy.accessoryType = MDCCollectionViewCellAccessoryDisclosureIndicator;
  privacy.accessibilityTraits = UIAccessibilityTraitButton;
  [self.collectionViewModel addItem:privacy
            toSectionWithIdentifier:SectionIdentifierLinks];

  [self.collectionViewModel addSectionWithIdentifier:SectionIdentifierFooter];

  VersionItem* version = [[VersionItem alloc] initWithType:ItemTypeVersion];
  version.text = [self versionDescriptionString];
  version.accessibilityTraits = UIAccessibilityTraitButton;
  [self.collectionViewModel addItem:version
            toSectionWithIdentifier:SectionIdentifierFooter];
}

#pragma mark - UITableViewDataSource

- (UITableViewCell*)tableView:(UITableView*)tableView
        cellForRowAtIndexPath:(NSIndexPath*)indexPath {
  CollectionViewItem* item =
      [self.collectionViewModel itemAtIndexPath:indexPath];
  Class cellClass = [item tableViewCellClass];
  NSString* reuseIdentifier = NSStringFromClass(cellClass);
  [self.tableView registerClass:cellClass
         forCellReuseIdentifier:reuseIdentifier];
  UITableViewCell* cell =
      [self.tableView dequeueReusableCellWithIdentifier:reuseIdentifier
                                           forIndexPath:indexPath];
  [item configureTableViewCell:cell];

  return cell;
}

- (NSInteger)tableView:(UITableView*)tableView
    numberOfRowsInSection:(NSInteger)section {
  return [self.collectionViewModel numberOfItemsInSection:section];
}

- (NSInteger)numberOfSectionsInTableView:(UITableView*)tableView {
  return [self.collectionViewModel numberOfSections];
}

#pragma mark - UITableViewDelegate
- (void)tableView:(UITableView*)tableView
    didSelectRowAtIndexPath:(NSIndexPath*)indexPath {
  NSInteger itemType =
      [self.collectionViewModel itemTypeForIndexPath:indexPath];
  switch (itemType) {
    case ItemTypeLinksCredits:
      [self openURL:GURL(kChromeUICreditsURL)];
      break;
    case ItemTypeLinksTerms:
      [self openURL:GURL(kChromeUITermsURL)];
      break;
    case ItemTypeLinksPrivacy:
      [self openURL:GURL(l10n_util::GetStringUTF8(IDS_IOS_PRIVACY_POLICY_URL))];
      break;
    case ItemTypeVersion:
      [self copyVersionToPasteboard];
      break;
    default:
      NOTREACHED();
      break;
  }
}

#pragma mark Private methods

- (void)openURL:(GURL)URL {
  // BlockToOpenURL(self, self.dispatcher)(URL);
}

- (void)copyVersionToPasteboard {
  [[UIPasteboard generalPasteboard] setString:[self versionOnlyString]];
  TriggerHapticFeedbackForNotification(UINotificationFeedbackTypeSuccess);
  NSString* messageText = l10n_util::GetNSString(IDS_IOS_VERSION_COPIED);
  MDCSnackbarMessage* message =
      [MDCSnackbarMessage messageWithText:messageText];
  message.category = @"version copied";
  [MDCSnackbarManager showMessage:message];
}

- (std::string)versionString {
  std::string versionString = version_info::GetVersionNumber();
  std::string versionStringModifier = GetChannelString();
  if (!versionStringModifier.empty()) {
    versionString = versionString + " " + versionStringModifier;
  }
  return versionString;
}

- (NSString*)versionDescriptionString {
  return l10n_util::GetNSStringF(IDS_IOS_VERSION,
                                 base::UTF8ToUTF16([self versionString]));
}

- (NSString*)versionOnlyString {
  return base::SysUTF8ToNSString([self versionString]);
}

@end
