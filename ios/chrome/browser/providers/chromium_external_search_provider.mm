// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/providers/chromium_external_search_provider.h"

#include "ios/chrome/grit/ios_strings.h"

NSURL* ChromiumExternalSearchProvider::GetLaunchURL() {
  return nil;
}

NSString* ChromiumExternalSearchProvider::GetButtonImageName() {
  return @"keyboard_accessory_external_search";
}

int ChromiumExternalSearchProvider::GetButtonIdsAccessibilityLabel() {
  return IDS_IOS_KEYBOARD_ACCESSORY_VIEW_EXTERNAL_SEARCH;
}

NSString* ChromiumExternalSearchProvider::GetButtonAccessibilityIdentifier() {
  return @"External Search";
}
