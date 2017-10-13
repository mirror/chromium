// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_PAYMENTS_PAYMENT_REQUEST_COLLECTION_VIEW_DETAIL_ITEM_H_
#define IOS_CHROME_BROWSER_UI_PAYMENTS_PAYMENT_REQUEST_COLLECTION_VIEW_DETAIL_ITEM_H_

#import <UIKit/UIKit.h>

#import "ios/chrome/browser/ui/collection_view/cells/collection_view_detail_item.h"

// CollectionViewDetailItem for the Payment Request UI. Allows specifying
// whether an item is a call to action (will have a different color).
// TODO(crbug.com/774499): add a property for setting the accessory image.
@interface PaymentRequestCollectionViewDetailItem : CollectionViewDetailItem

// Whether this item is a call to action or not.
@property(nonatomic) BOOL isCallToAction;

@end

#endif  // IOS_CHROME_BROWSER_UI_PAYMENTS_PAYMENT_REQUEST_COLLECTION_VIEW_DETAIL_ITEM_H_
