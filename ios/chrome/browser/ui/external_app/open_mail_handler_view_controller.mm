// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/external_app/open_mail_handler_view_controller.h"

#include "base/logging.h"
#include "base/mac/foundation_util.h"
#import "ios/chrome/browser/ui/collection_view/cells/collection_view_switch_item.h"
#import "ios/chrome/browser/ui/collection_view/cells/collection_view_text_item.h"
#import "ios/chrome/browser/web/mailto_handler.h"
#import "ios/chrome/browser/web/mailto_url_rewriter.h"
#include "ios/chrome/grit/ios_strings.h"
#include "ui/base/l10n/l10n_util.h"
#include "url/gurl.h"

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
  // Item with the UISwitch toggle for user to choose whether a mailto://
  // app selection should be remembered for future use.
  CollectionViewSwitchItem* _alwaysAskItem;
  // An array of apps that can handle mailto:// URLs. The |rewriter| object
  // lists all supported mailto:// handlers, but not all of them would be
  // installed.
  NSMutableArray<MailtoHandler*>* _availableHandlers;
}

@property(nonatomic, strong) MailtoURLRewriter* rewriter;
@end

@implementation OpenMailHandlerViewController
@synthesize rewriter = _rewriter;
@synthesize onMailtoHandlerSelected = _onMailtoHandlerSelected;

- (instancetype)initWithRewriter:(MailtoURLRewriter*)rewriter {
  self = [self initWithLayout:[[MDCCollectionViewFlowLayout alloc] init]
                        style:CollectionViewControllerStyleDefault];
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

  // Adds list of available mailto:// handler apps.
  [model addSectionWithIdentifier:SectionIdentifierApps];
  _availableHandlers = [NSMutableArray array];
  for (MailtoHandler* handler in [_rewriter defaultHandlers]) {
    if ([handler isAvailable]) {
      [_availableHandlers addObject:handler];
      CollectionViewTextItem* item =
          [self mailHandlerAppItem:ItemTypeApp label:[handler appName]];
      [model addItem:item toSectionWithIdentifier:SectionIdentifierApps];
    }
  }

  // The footer is a row with a toggle switch. The default should be always ask
  // for which app to choose, i.e. ON.
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

  NSInteger itemType =
      [self.collectionViewModel itemTypeForIndexPath:indexPath];
  if (itemType == ItemTypeAlwaysAsk) {
    CollectionViewSwitchCell* switchCell =
        base::mac::ObjCCastStrict<CollectionViewSwitchCell>(cell);
    [switchCell.switchView addTarget:self
                              action:@selector(alwaysAsk:)
                    forControlEvents:UIControlEventValueChanged];
  }

  return cell;
}

- (void)alwaysAsk:(UISwitch*)sender {
  BOOL isOn = [sender isOn];
  [_alwaysAskItem setOn:isOn];
}

#pragma mark - UICollectionViewDelegate

- (BOOL)collectionView:(nonnull UICollectionView*)collectionView
    shouldHighlightItemAtIndexPath:(nonnull NSIndexPath*)indexPath {
  BOOL result = [super collectionView:collectionView
       shouldHighlightItemAtIndexPath:indexPath];
  NSInteger itemType =
      [self.collectionViewModel itemTypeForIndexPath:indexPath];
  return itemType == ItemTypeApp ? result : NO;
}

- (void)collectionView:(UICollectionView*)collectionView
    didSelectItemAtIndexPath:(NSIndexPath*)indexPath {
  [super collectionView:collectionView didSelectItemAtIndexPath:indexPath];

  NSInteger itemType =
      [self.collectionViewModel itemTypeForIndexPath:indexPath];
  switch (itemType) {
    case ItemTypeTitle:
      break;
    case ItemTypeApp: {
      NSUInteger row = indexPath.row;
      DCHECK(0U <= row && row < [_availableHandlers count]);
      MailtoHandler* handler = _availableHandlers[row];
      if (![_alwaysAskItem isOn]) {
        [_rewriter setDefaultHandlerID:[handler appStoreID]];
      }
      [self.presentingViewController
          dismissViewControllerAnimated:YES
                             completion:^{
                               if (_onMailtoHandlerSelected)
                                 _onMailtoHandlerSelected(handler);
                             }];
      break;
    }
    case ItemTypeAlwaysAsk:
      break;
  }
}
@end
