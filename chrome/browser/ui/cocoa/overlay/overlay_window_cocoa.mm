// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/cocoa/overlay/overlay_window_cocoa.h"

#include "ui/gfx/geometry/rect.h"

#import <AppKit/AppKit.h>

// static
std::unique_ptr<OverlayWindow> OverlayWindow::Create() {
  return base::WrapUnique(new OverlayWindowCocoa());
}

OverlayWindowCocoa::OverlayWindowCocoa() {
  // TODO(apacible): Update preferred sizing and positioning.
  // http://crbug/726621
  NSRect frame = NSMakeRect(100, 100, 700, 500);
  window_ =
      [[[NSWindow alloc] initWithContentRect:frame
                                   styleMask:NSBorderlessWindowMask
                                     backing:NSBackingStoreBuffered
                                       defer:NO] autorelease];
  controller_ =
      [[NSWindowController alloc] initWithWindow:window_];
  [controller_ autorelease];
}

OverlayWindowCocoa::~OverlayWindowCocoa() = default;

void OverlayWindowCocoa::Init() {
  [window_ setBackgroundColor:[NSColor blackColor]];
  [window_ makeKeyAndOrderFront:controller_];
  [NSApp run];
}

bool OverlayWindowCocoa::IsActive() const {
  return [window_ isKeyWindow];
}

void OverlayWindowCocoa::Show() {
  [window_ makeKeyAndOrderFront:controller_];
}

void OverlayWindowCocoa::Hide() {
  [window_ orderOut:window_];
}

void OverlayWindowCocoa::Close() {
  // Hide the window before calling close.
  [window_ orderOut:window_];
  [window_ close];
}

void OverlayWindowCocoa::Activate() {
  NOTIMPLEMENTED();
}

bool OverlayWindowCocoa::IsAlwaysOnTop() const {
  return true;
}

ui::Layer* OverlayWindowCocoa::GetLayer() {
  NOTIMPLEMENTED();
  return nullptr;
}

gfx::NativeWindow OverlayWindowCocoa::GetNativeWindow() const {
  return window_;
}

gfx::Rect OverlayWindowCocoa::GetBounds() const {
  NOTIMPLEMENTED();
  return gfx::Rect();
}