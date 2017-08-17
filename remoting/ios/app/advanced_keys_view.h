// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef REMOTING_IOS_APP_ADVANCED_KEYS_VIEW_H_
#define REMOTING_IOS_APP_ADVANCED_KEYS_VIEW_H_

#import <UIKit/UIKit.h>

#include <vector>

@protocol AdvancedKeysViewDelegate<NSObject>

- (void)onKeyCombination:(const std::vector<uint32_t>&)combination;

@end

@interface AdvancedKeysView : UIView

- (void)releaseAllModifiers;

@property(nonatomic, readonly) std::vector<uint32_t> activeModifiers;
@property(nonatomic, weak) id<AdvancedKeysViewDelegate> delegate;

@end

#endif  // REMOTING_IOS_APP_ADVANCED_KEYS_VIEW_H_
