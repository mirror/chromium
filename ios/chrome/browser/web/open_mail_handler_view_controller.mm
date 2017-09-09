// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/web/open_mail_handler_view_controller.h"

#import "ios/chrome/browser/ui/collection_view/cells/collection_view_switch_item.h"
#import "ios/chrome/browser/ui/collection_view/cells/collection_view_text_item.h"
#import "ios/chrome/browser/web/mailto_handler.h"
#import "ios/chrome/browser/web/mailto_url_rewriter.h"
#include "ios/chrome/grit/ios_strings.h"
#include "ui/base/l10n/l10n_util.h"

#import <UIKit/UIKit.h>

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {

typedef NS_ENUM(NSInteger, SectionIdentifier) {
  SectionIdentifierHeader = kSectionIdentifierEnumZero,
  SectionIdentifierApps,
  SectionIdentifierSwitch,
};

typedef NS_ENUM(NSInteger, ItemType) {
  ItemTypeTitle = kItemTypeEnumZero,
  ItemTypeApp,  // repeated item
  ItemTypeAlwaysAsk,
};

}  // namespace

@interface OpenMailHandlerViewController () {
  CollectionViewSwitchItem* _alwaysAskItem;
}
@property(nonatomic, strong) MailtoURLRewriter* rewriter;
@end

@implementation OpenMailHandlerViewController
@synthesize rewriter = _rewriter;

- (instancetype)initWithRewriter:(MailtoURLRewriter*)rewriter
                          layout:(UICollectionViewLayout*)layout
                           style:(CollectionViewControllerStyle)style {
  self = [self initWithLayout:layout style:style];
  if (self) {
    _rewriter = rewriter;
    [self loadModel];
  }
  return self;
}

- (void)loadModel {
  [super loadModel];
  CollectionViewModel* model = self.collectionViewModel;

  // Header
  [model addSectionWithIdentifier:SectionIdentifierHeader];
  CollectionViewTextItem* titleItem =
      [[CollectionViewTextItem alloc] initWithType:ItemTypeTitle];
  titleItem.text =
      l10n_util::GetNSString(IDS_IOS_CHOOSE_DEFAULT_EMAIL_CLIENT_APP);
  [model addItem:titleItem toSectionWithIdentifier:SectionIdentifierHeader];

  [model addSectionWithIdentifier:SectionIdentifierApps];
  for (MailtoHandler* handler in [_rewriter defaultHandlers]) {
    [model addItem:[self mailHandlerAppItem:ItemTypeApp label:[handler appName]]
        toSectionWithIdentifier:SectionIdentifierApps];
  }

  [model addSectionWithIdentifier:SectionIdentifierSwitch];
  _alwaysAskItem = [self alwaysAskSwitchItem];
  [model addItem:_alwaysAskItem
      toSectionWithIdentifier:SectionIdentifierSwitch];
}

- (CollectionViewTextItem*)mailHandlerAppItem:(NSInteger)type
                                        label:(NSString*)appName {
  CollectionViewTextItem* textItem =
      [[CollectionViewTextItem alloc] initWithType:type];
  textItem.text = appName;
  return textItem;
}

- (CollectionViewSwitchItem*)alwaysAskSwitchItem {
  CollectionViewSwitchItem* switchItem =
      [[CollectionViewSwitchItem alloc] initWithType:ItemTypeAlwaysAsk];
  switchItem.text = l10n_util::GetNSString(IDS_IOS_CHOOSE_EMAIL_ASK_TOGGLE);
  switchItem.on = YES;
  return switchItem;
}

#pragma mark - UICollectionViewDataSource

- (UICollectionViewCell*)collectionView:(UICollectionView*)collectionView
                 cellForItemAtIndexPath:(NSIndexPath*)indexPath {
  UICollectionViewCell* cell =
      [super collectionView:collectionView cellForItemAtIndexPath:indexPath];

  /*
  NSInteger itemType =
      [self.collectionViewModel itemTypeForIndexPath:indexPath];

  if (itemType == ItemTypeWebServicesShowSuggestions) {
    CollectionViewSwitchCell* switchCell =
        base::mac::ObjCCastStrict<CollectionViewSwitchCell>(cell);
    [switchCell.switchView addTarget:self
               action:@selector(showSuggestionsToggled:)
               forControlEvents:UIControlEventValueChanged];
  }
  */

  return cell;
}

@end
