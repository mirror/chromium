// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/payments/payment_request_view_controller.h"

#import "base/ios/weak_nsobject.h"
#include "base/mac/foundation_util.h"
#include "base/mac/objc_property_releaser.h"
#include "base/mac/scoped_nsobject.h"
#include "base/strings/sys_string_conversions.h"
#include "components/autofill/core/browser/autofill_data_util.h"
#include "components/autofill/core/browser/autofill_profile.h"
#include "components/autofill/core/browser/credit_card.h"
#include "components/autofill/core/browser/field_types.h"
#include "components/autofill/core/browser/personal_data_manager.h"
#include "ios/chrome/browser/application_context.h"
#import "ios/chrome/browser/payments/cells/page_info_item.h"
#import "ios/chrome/browser/payments/cells/payment_method_item.h"
#import "ios/chrome/browser/payments/cells/shipping_address_item.h"
#import "ios/chrome/browser/payments/payment_request_utils.h"
#import "ios/chrome/browser/ui/collection_view/cells/MDCCollectionViewCell+Chrome.h"
#import "ios/chrome/browser/ui/collection_view/cells/collection_view_detail_item.h"
#import "ios/chrome/browser/ui/collection_view/cells/collection_view_item.h"
#import "ios/chrome/browser/ui/collection_view/cells/collection_view_text_item.h"
#import "ios/chrome/browser/ui/collection_view/collection_view_model.h"
#import "ios/chrome/browser/ui/colors/MDCPalette+CrAdditions.h"
#include "ios/chrome/browser/ui/rtl_geometry.h"
#include "ios/chrome/grit/ios_strings.h"
#import "ios/third_party/material_components_ios/src/components/Buttons/src/MaterialButtons.h"
#import "ios/third_party/material_components_ios/src/components/CollectionCells/src/MaterialCollectionCells.h"
#import "ios/third_party/material_roboto_font_loader_ios/src/src/MaterialRobotoFontLoader.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/resource/resource_bundle.h"

NSString* const kPaymentRequestCollectionViewId =
    @"kPaymentRequestCollectionViewId";

namespace {

const CGFloat kButtonEdgeInset = 9;
const CGFloat kSeparatorEdgeInset = 14;

typedef NS_ENUM(NSInteger, SectionIdentifier) {
  SectionIdentifierSummary = kSectionIdentifierEnumZero,
  SectionIdentifierShipping,
  SectionIdentifierPayment,

};

typedef NS_ENUM(NSInteger, ItemType) {
  ItemTypeSummaryPageInfo = kItemTypeEnumZero,
  ItemTypeSummaryTotal,
  ItemTypeShippingTitle,
  ItemTypeShippingAddress,
  ItemTypeAddShippingAddress,
  ItemTypePaymentTitle,
  ItemTypePaymentMethod,
};

}  // namespace

@interface PaymentRequestViewController () {
  base::WeakNSProtocol<id<PaymentRequestViewControllerDelegate>> _delegate;
  base::scoped_nsobject<UIBarButtonItem> _cancelButton;
  base::scoped_nsobject<MDCFlatButton> _payButton;

  ShippingAddressItem* _selectedShippingAddressItem;

  base::mac::ObjCPropertyReleaser
      _propertyReleaser_PaymentRequestViewController;
}

// Called when the user presses the cancel button.
- (void)onCancel;

// Called when the user presses the confirm button.
- (void)onConfirm;

@end

@implementation PaymentRequestViewController

@synthesize paymentRequest = _paymentRequest;
@synthesize pageFavicon = _pageFavicon;
@synthesize pageTitle = _pageTitle;
@synthesize pageHost = _pageHost;
@synthesize selectedPaymentMethod = _selectedPaymentMethod;
@synthesize selectedShippingAddress = _selectedShippingAddress;

- (instancetype)init {
  if ((self = [super initWithStyle:CollectionViewControllerStyleAppBar])) {
    _propertyReleaser_PaymentRequestViewController.Init(
        self, [PaymentRequestViewController class]);

    [self setTitle:l10n_util::GetNSString(IDS_IOS_PAYMENT_REQUEST_TITLE)];

    // Set up left (cancel) button.
    _cancelButton.reset([[UIBarButtonItem alloc]
        initWithTitle:l10n_util::GetNSString(
                          IDS_IOS_PAYMENT_REQUEST_CANCEL_BUTTON)
                style:UIBarButtonItemStylePlain
               target:nil
               action:@selector(onCancel)]);
    [_cancelButton setTitleTextAttributes:@{
      NSForegroundColorAttributeName : [UIColor lightGrayColor]
    }
                                 forState:UIControlStateDisabled];
    [self navigationItem].leftBarButtonItem = _cancelButton;

    // Set up right (pay) button.
    _payButton.reset([[MDCFlatButton alloc] init]);
    [_payButton
        setTitle:l10n_util::GetNSString(IDS_IOS_PAYMENT_REQUEST_PAY_BUTTON)
        forState:UIControlStateNormal];
    [_payButton setBackgroundColor:[[MDCPalette cr_bluePalette] tint500]
                          forState:UIControlStateNormal];
    [_payButton setInkColor:[UIColor colorWithWhite:1 alpha:0.2]];
    [_payButton setBackgroundColor:[UIColor grayColor]
                          forState:UIControlStateDisabled];
    [_payButton addTarget:nil
                   action:@selector(onConfirm)
         forControlEvents:UIControlEventTouchUpInside];
    [_payButton sizeToFit];
    [_payButton setEnabled:NO];
    [_payButton setAutoresizingMask:UIViewAutoresizingFlexibleTrailingMargin() |
                                    UIViewAutoresizingFlexibleTopMargin |
                                    UIViewAutoresizingFlexibleBottomMargin];

    // The navigation bar will set the rightBarButtonItem's height to the full
    // height of the bar. We don't want that for the button so we use a UIView
    // here to contain the button instead and the button is vertically centered
    // inside the full bar height.
    UIView* buttonView =
        [[[UIView alloc] initWithFrame:CGRectZero] autorelease];
    [buttonView addSubview:_payButton];
    // Navigation bar button items are aligned with the trailing edge of the
    // screen. Make the enclosing view larger here. The pay button will be
    // aligned with the leading edge of the enclosing view leaving an inset on
    // the trailing edge.
    CGRect buttonViewBounds = buttonView.bounds;
    buttonViewBounds.size.width =
        [_payButton frame].size.width + kButtonEdgeInset;
    buttonView.bounds = buttonViewBounds;

    UIBarButtonItem* payButtonItem =
        [[[UIBarButtonItem alloc] initWithCustomView:buttonView] autorelease];
    [self navigationItem].rightBarButtonItem = payButtonItem;
  }
  return self;
}

- (id<PaymentRequestViewControllerDelegate>)delegate {
  return _delegate.get();
}

- (void)setDelegate:(id<PaymentRequestViewControllerDelegate>)delegate {
  _delegate.reset(delegate);
}

- (void)setSelectedPaymentMethod:(autofill::CreditCard*)method {
  _selectedPaymentMethod = method;
  [_payButton setEnabled:(method != nil)];
}

- (void)onCancel {
  [_cancelButton setEnabled:NO];
  [_payButton setEnabled:NO];

  [_delegate paymentRequestViewControllerDidCancel];
}

- (void)onConfirm {
  [_cancelButton setEnabled:NO];
  [_payButton setEnabled:NO];

  [_delegate paymentRequestViewControllerDidConfirm];
}

#pragma mark - CollectionViewController methods

- (void)loadModel {
  [super loadModel];
  CollectionViewModel* model = self.collectionViewModel;

  // Summary section.
  [model addSectionWithIdentifier:SectionIdentifierSummary];

  PageInfoItem* pageInfo =
      [[[PageInfoItem alloc] initWithType:ItemTypeSummaryPageInfo] autorelease];
  pageInfo.pageFavicon = _pageFavicon;
  pageInfo.pageTitle = _pageTitle;
  pageInfo.pageHost = _pageHost;
  [model setHeader:pageInfo forSectionWithIdentifier:SectionIdentifierSummary];

  CollectionViewDetailItem* total = [[[CollectionViewDetailItem alloc]
      initWithType:ItemTypeSummaryTotal] autorelease];
  total.text = l10n_util::GetNSString(IDS_IOS_PAYMENT_REQUEST_TOTAL_HEADER);
  NSString* currencyCode =
      base::SysUTF16ToNSString(_paymentRequest.details.total.amount.currency);
  NSDecimalNumber* value = [NSDecimalNumber
      decimalNumberWithString:SysUTF16ToNSString(
                                  _paymentRequest.details.total.amount.value)];
  NSNumberFormatter* currencyFormatter =
      [[[NSNumberFormatter alloc] init] autorelease];
  [currencyFormatter setNumberStyle:NSNumberFormatterCurrencyStyle];
  [currencyFormatter setCurrencyCode:currencyCode];
  total.detailText = [currencyFormatter stringFromNumber:value];
  if (!_paymentRequest.details.display_items.empty()) {
    total.accessoryType = MDCCollectionViewCellAccessoryDisclosureIndicator;
    total.accessibilityTraits |= UIAccessibilityTraitButton;
  }
  [model addItem:total toSectionWithIdentifier:SectionIdentifierSummary];

  // Shipping section.
  [model addSectionWithIdentifier:SectionIdentifierShipping];
  CollectionViewTextItem* shippingTitle = [[[CollectionViewTextItem alloc]
      initWithType:ItemTypeShippingTitle] autorelease];
  shippingTitle.text =
      l10n_util::GetNSString(IDS_IOS_PAYMENT_REQUEST_SHIPPING_ADDRESS_HEADER);
  [model setHeader:shippingTitle
      forSectionWithIdentifier:SectionIdentifierShipping];

  CollectionViewItem* shippingItem = nil;
  if (_selectedShippingAddress) {
    _selectedShippingAddressItem = [[[ShippingAddressItem alloc]
        initWithType:ItemTypeShippingAddress] autorelease];
    shippingItem = _selectedShippingAddressItem;
    [self fillShippingAddressItem:_selectedShippingAddressItem
                      withAddress:_selectedShippingAddress];
    _selectedShippingAddressItem.accessoryType =
        MDCCollectionViewCellAccessoryDisclosureIndicator;

  } else {
    CollectionViewDetailItem* addAddressItem =
        [[[CollectionViewDetailItem alloc]
            initWithType:ItemTypeAddShippingAddress] autorelease];
    shippingItem = addAddressItem;
    addAddressItem.text = l10n_util::GetNSString(
        IDS_IOS_PAYMENT_REQUEST_SHIPPING_ADDRESS_SELECTION_TITLE);
    addAddressItem.detailText = [l10n_util::GetNSString(
        IDS_IOS_PAYMENT_REQUEST_ADD_SHIPPING_ADDRESS_BUTTON)
        uppercaseStringWithLocale:[NSLocale currentLocale]];
  }
  shippingItem.accessibilityTraits |= UIAccessibilityTraitButton;
  [model addItem:shippingItem
      toSectionWithIdentifier:SectionIdentifierShipping];

  // Payment method section.
  [model addSectionWithIdentifier:SectionIdentifierPayment];

  CollectionViewTextItem* paymentTitle = [[[CollectionViewTextItem alloc]
      initWithType:ItemTypePaymentTitle] autorelease];
  paymentTitle.text =
      l10n_util::GetNSString(IDS_IOS_PAYMENT_REQUEST_PAYMENT_METHOD_HEADER);
  [model setHeader:paymentTitle
      forSectionWithIdentifier:SectionIdentifierPayment];

  if (_selectedPaymentMethod) {
    PaymentMethodItem* paymentMethod = [[[PaymentMethodItem alloc]
        initWithType:ItemTypePaymentMethod] autorelease];

    paymentMethod.methodID = base::SysUTF16ToNSString(
        _selectedPaymentMethod->TypeAndLastFourDigits());
    paymentMethod.methodDetail = base::SysUTF16ToNSString(
        _selectedPaymentMethod->GetRawInfo(autofill::CREDIT_CARD_NAME_FULL));

    int selectedMethodCardTypeIconID =
        autofill::data_util::GetPaymentRequestData(
            _selectedPaymentMethod->type())
            .icon_resource_id;
    ui::ResourceBundle& rb = ui::ResourceBundle::GetSharedInstance();
    paymentMethod.methodTypeIcon =
        rb.GetNativeImageNamed(selectedMethodCardTypeIconID).ToUIImage();

    paymentMethod.accessoryType =
        MDCCollectionViewCellAccessoryDisclosureIndicator;
    paymentMethod.accessibilityTraits |= UIAccessibilityTraitButton;
    [model addItem:paymentMethod
        toSectionWithIdentifier:SectionIdentifierPayment];
  } else {
    CollectionViewTextItem* paymentMethod = [[[CollectionViewTextItem alloc]
        initWithType:ItemTypePaymentMethod] autorelease];
    NSString* selectText =
        l10n_util::GetNSString(IDS_IOS_PAYMENT_REQUEST_METHOD_SELECTION_BUTTON);
    paymentMethod.text =
        [selectText uppercaseStringWithLocale:[NSLocale currentLocale]];
    paymentMethod.accessibilityTraits |= UIAccessibilityTraitButton;
    [model addItem:paymentMethod
        toSectionWithIdentifier:SectionIdentifierPayment];
  }
}

- (void)viewDidLoad {
  [super viewDidLoad];
  self.collectionView.accessibilityIdentifier = kPaymentRequestCollectionViewId;

  // Customize collection view settings.
  self.styler.cellStyle = MDCCollectionViewCellStyleCard;
  self.styler.separatorInset =
      UIEdgeInsetsMake(0, kSeparatorEdgeInset, 0, kSeparatorEdgeInset);
}

- (void)updateSelectedShippingAddress:
    (autofill::AutofillProfile*)shippingAddress {
  [self setSelectedShippingAddress:shippingAddress];
  [self fillShippingAddressItem:_selectedShippingAddressItem
                    withAddress:shippingAddress];
  NSIndexPath* indexPath =
      [self.collectionViewModel indexPathForItem:_selectedShippingAddressItem
                         inSectionWithIdentifier:SectionIdentifierShipping];
  [self.collectionView reloadItemsAtIndexPaths:@[ indexPath ]];
}

#pragma mark - Helper methods

- (void)fillShippingAddressItem:(ShippingAddressItem*)item
                    withAddress:(autofill::AutofillProfile*)address {
  item.name =
      base::SysUTF16ToNSString(address->GetRawInfo(autofill::NAME_FULL));
  item.address =
      payment_request_utils::AddressLabelFromAutofillProfile(address);
  item.phoneNumber = base::SysUTF16ToNSString(
      address->GetRawInfo(autofill::PHONE_HOME_WHOLE_NUMBER));
}

#pragma mark UICollectionViewDataSource
- (UICollectionViewCell*)collectionView:(UICollectionView*)collectionView
                 cellForItemAtIndexPath:(nonnull NSIndexPath*)indexPath {
  UICollectionViewCell* cell =
      [super collectionView:collectionView cellForItemAtIndexPath:indexPath];

  NSInteger itemType =
      [self.collectionViewModel itemTypeForIndexPath:indexPath];
  switch (itemType) {
    case ItemTypeAddShippingAddress: {
      CollectionViewDetailCell* detailCell =
          base::mac::ObjCCastStrict<CollectionViewDetailCell>(cell);
      detailCell.detailTextLabel.font =
          [[MDFRobotoFontLoader sharedInstance] mediumFontOfSize:14];
      detailCell.detailTextLabel.textColor =
          [[MDCPalette cr_bluePalette] tint700];
      break;
    }
    default:
      break;
  }
  return cell;
}

#pragma mark UICollectionViewDelegate

- (void)collectionView:(UICollectionView*)collectionView
    didSelectItemAtIndexPath:(NSIndexPath*)indexPath {
  [super collectionView:collectionView didSelectItemAtIndexPath:indexPath];

  NSInteger itemType =
      [self.collectionViewModel itemTypeForIndexPath:indexPath];
  switch (itemType) {
    case ItemTypeSummaryTotal:
      if (!_paymentRequest.details.display_items.empty())
        [_delegate paymentRequestViewControllerDisplayPaymentItems];
      break;
    case ItemTypeShippingAddress:
    case ItemTypeAddShippingAddress:
      [_delegate paymentRequestViewControllerSelectShippingAddress];
      break;
    case ItemTypePaymentMethod:
      [_delegate paymentRequestViewControllerSelectPaymentMethod];
      break;
    default:
      NOTREACHED();
      break;
  }
}

#pragma mark MDCCollectionViewStylingDelegate

- (CGFloat)collectionView:(UICollectionView*)collectionView
    cellHeightAtIndexPath:(NSIndexPath*)indexPath {
  NSInteger type = [self.collectionViewModel itemTypeForIndexPath:indexPath];
  if ((type == ItemTypePaymentMethod && _selectedPaymentMethod) ||
      (type == ItemTypeShippingAddress)) {
    CollectionViewItem* item =
        [self.collectionViewModel itemAtIndexPath:indexPath];
    return [MDCCollectionViewCell
        cr_preferredHeightForWidth:CGRectGetWidth(collectionView.bounds)
                           forItem:item];
  } else {
    return MDCCellDefaultOneLineHeight;
  }
}

// If there are no payment items to display, there is no effect from touching
// the total so there should not be an ink ripple.
- (BOOL)collectionView:(UICollectionView*)collectionView
    hidesInkViewAtIndexPath:(NSIndexPath*)indexPath {
  NSInteger type = [self.collectionViewModel itemTypeForIndexPath:indexPath];
  if (type == ItemTypeSummaryTotal &&
      _paymentRequest.details.display_items.empty()) {
    return YES;
  } else {
    return NO;
  }
}

@end
