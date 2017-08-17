// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import <AppKit/AppKit.h>

#import "ui/base/cocoa/hover_button.h"
#include "ui/gfx/vector_icon_types.h"

@interface MDHoverButton : HoverButton
@property(nonatomic) const gfx::VectorIcon* icon;
@property(nonatomic) int iconSize;
@property(nonatomic) BOOL hoverSuppressed;
@end
