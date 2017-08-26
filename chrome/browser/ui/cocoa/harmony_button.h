// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import <AppKit/AppKit.h>

#import "ui/base/cocoa/hover_button.h"

// HarmonyButton follows the Harmony design spec. It has slightly rounded
// corners, a border, and a shadow that appears on hover. Its color scheme
// tracks the active theme. Not every part of the spec has been implemented: it
// doesn't look different if it's the default button, or if it's disabled, for
// instance. Anyone who needs these things is encouraged to find joy in adding
// support for them.

@interface HarmonyButton : HoverButton
@end
