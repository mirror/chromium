// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/cocoa/overlay/overlay_window_cocoa.h"

// #include "ui/views/widget/widget.h"
#include "ui/gfx/geometry/rect.h"

#import <AppKit/AppKit.h>

// static
std::unique_ptr<OverlayWindow> OverlayWindow::Create() {
  return base::WrapUnique(new OverlayWindowCocoa());
}

OverlayWindowCocoa::OverlayWindowCocoa() {
  NSRect frame = NSMakeRect(0, 0, 200, 200);
  NSWindow* window =
      [[[NSWindow alloc] initWithContentRect:frame
                                   styleMask:NSBorderlessWindowMask
                                     backing:NSBackingStoreBuffered
                                       defer:NO] autorelease];
  NSWindowController* windowController =
      [[NSWindowController alloc] initWithWindow:window];
  [windowController autorelease];

  [window setBackgroundColor:[NSColor blueColor]];
  // [window makeKeyAndOrderFront:window];

  [window orderFrontRegardless];
  [NSApp run];
  // widget_.reset(new views::Widget());
}

OverlayWindowCocoa::~OverlayWindowCocoa() = default;

void OverlayWindowCocoa::Init() {}

bool OverlayWindowCocoa::IsActive() const {
  return true;  // widget_->IsActive();
}

void OverlayWindowCocoa::Show() {
  // widget_->Show();
}

void OverlayWindowCocoa::Hide() {
  // widget_->Hide();
}

void OverlayWindowCocoa::Close() {
  // widget_->Close();
}

void OverlayWindowCocoa::Activate() {
  // widget_->Activate();
}

bool OverlayWindowCocoa::IsAlwaysOnTop() const {
  return true;
}

ui::Layer* OverlayWindowCocoa::GetLayer() {
  // return widget_->GetLayer();
  return nullptr;
}

gfx::NativeWindow OverlayWindowCocoa::GetNativeWindow() const {
  // return widget_->GetNativeWindow();
  return gfx::NativeWindow();
}

gfx::Rect OverlayWindowCocoa::GetBounds() const {
  // return widget_->GetRestoredBounds();
  return gfx::Rect();
}