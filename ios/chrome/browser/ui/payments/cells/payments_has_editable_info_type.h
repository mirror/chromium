// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_PAYMENTS_CELLS_PAYMENTS_HAS_EDITABLE_INFO_TYPE_H_
#define IOS_CHROME_BROWSER_UI_PAYMENTS_CELLS_PAYMENTS_HAS_EDITABLE_INFO_TYPE_H_

#import "ios/chrome/browser/ui/payments/cells/payments_has_accessory_type.h"

// Protocol adopted by the payments collection view items that can be edited
// such as contact, address, and payment method.
@protocol PaymentsHasEditableInfoType<PaymentsHasAccessoryType>

// Whether or not the selectable cell has all of its required information.
@property(nonatomic, assign, getter=isComplete) BOOL complete;

@end

#endif  // IOS_CHROME_BROWSER_UI_PAYMENTS_CELLS_PAYMENTS_HAS_EDITABLE_INFO_H_
