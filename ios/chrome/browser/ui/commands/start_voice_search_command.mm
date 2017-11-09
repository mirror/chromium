// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/commands/start_voice_search_command.h"

#include <memory>

#include "base/memory/ptr_util.h"
#import "ios/chrome/browser/ui/util/view_info.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@interface StartVoiceSearchCommand () {
  // The origin view's info.
  std::unique_ptr<ViewInfo> _originViewInfo;
}
@end

@implementation StartVoiceSearchCommand

- (instancetype)initWithOriginView:(UIView*)originView {
  if ((self = [super initWithTag:0])) {
    _originViewInfo = base::MakeUnique<ViewInfo>(originView);
  }
  return self;
}

#pragma mark - Accessors

- (const ViewInfo*)originViewInfo {
  return _originViewInfo.get();
}

@end
