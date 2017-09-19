// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/views/controls/native/native_view_host_mac.h"

#import <Cocoa/Cocoa.h>

#include "base/mac/foundation_util.h"
#import "ui/views/cocoa/bridged_native_widget.h"
#include "ui/views/controls/native/native_view_host.h"
#include "ui/views/widget/native_widget_mac.h"
#include "ui/views/widget/widget.h"

// An NSView that allows the rendering size of its child subview to be different
// than its own frame size, which will result in showing a scaled version of the
// child.
@interface NativeViewHostMacScalingView : NSView {
}

- (id)init;
- (void)attach:(NSView*)subview;
- (BOOL)hasAttachedSubview:(NSView*)subview;
- (void)showWithFrame:(NSRect)frame andSize:(NSSize)size;
@end

@implementation NativeViewHostMacScalingView

- (id)init {
  if (self = [super initWithFrame:NSZeroRect]) {
    // NativeViewHostMac::ShowWidget() provides manual layout.
    [self setAutoresizingMask:NSViewNotSizable];
  }
  return self;
}

- (void)attach:(NSView*)subview {
  if ([[self subviews] count] == 0) {
    [self addSubview:subview];
  } else if ([[self subviews] objectAtIndex:0] != subview) {
    [self replaceSubview:[[self subviews] objectAtIndex:0] with:subview];
  }
}

- (BOOL)hasAttachedSubview:(NSView*)subview {
  if ([[self subviews] count] > 0 &&
      [[self subviews] objectAtIndex:0] == subview) {
    return YES;
  }
  return NO;
}

- (void)showWithFrame:(NSRect)frame andSize:(NSSize)size {
  [self setFrame:frame];
  const NSRect bounds = NSMakeRect(0, 0, size.width, size.height);
  [self setBounds:bounds];
  DCHECK([[self subviews] count] > 0);
  [[[self subviews] objectAtIndex:0] setFrame:bounds];
}

@end  // @implementation NativeViewHostMacScalingView

namespace views {
namespace {

void EnsureNativeViewHasNoChildWidgets(NSView* native_view) {
  DCHECK(native_view);
  // Mac's NativeViewHost has no support for hosting its own child widgets.
  // This check is probably overly restrictive, since the Widget containing the
  // NativeViewHost _is_ allowed child Widgets. However, we don't know yet
  // whether those child Widgets need to be distinguished from Widgets that code
  // might want to associate with the hosted NSView instead.
  {
    Widget::Widgets child_widgets;
    Widget::GetAllChildWidgets(native_view, &child_widgets);
    CHECK_GE(1u, child_widgets.size());  // 1 (itself) or 0 if detached.
  }
}

}  // namespace

NativeViewHostMac::NativeViewHostMac(NativeViewHost* host) : host_(host) {
}

NativeViewHostMac::~NativeViewHostMac() {
}

////////////////////////////////////////////////////////////////////////////////
// NativeViewHostMac, NativeViewHostWrapper implementation:

void NativeViewHostMac::AttachNativeView() {
  NSView* const native_view = host_->native_view();
  DCHECK(native_view);
  DCHECK(!scaling_view_);
  scaling_view_.reset([[NativeViewHostMacScalingView alloc] init]);
  [scaling_view_ attach:native_view];

  EnsureNativeViewHasNoChildWidgets(native_view);
  BridgedNativeWidget* bridge = NativeWidgetMac::GetBridgeForNativeWindow(
      host_->GetWidget()->GetNativeWindow());
  DCHECK(bridge);

  [bridge->ns_view() addSubview:scaling_view_];
  bridge->SetAssociationForView(host_, scaling_view_);
}

void NativeViewHostMac::NativeViewDetaching(bool destroyed) {
  // |destroyed| is only true if this class calls host_->NativeViewDestroyed().
  // Aura does this after observing an aura OnWindowDestroying, but NSViews
  // are reference counted so there isn't a reliable signal. Instead, a
  // reference is retained until the NativeViewHost is detached.
  DCHECK(!destroyed);

  // |scaling_view_| can be nil here if RemovedFromWidget() is called before
  // NativeViewHost::Detach().
  if (!scaling_view_) {
    DCHECK(![host_->native_view() superview]);
    return;
  }

  DCHECK([scaling_view_ hasAttachedSubview:host_->native_view()]);
  [host_->native_view() setHidden:YES];
  // Retain the native view for the remainder of this scope, since removing it
  // from its superview could cause it to be released.
  const base::scoped_nsobject<NSView> retained_native_view(
      [host_->native_view() retain]);
  [scaling_view_ removeFromSuperview];

  EnsureNativeViewHasNoChildWidgets(host_->native_view());
  BridgedNativeWidget* bridge = NativeWidgetMac::GetBridgeForNativeWindow(
      host_->GetWidget()->GetNativeWindow());
  // BridgedNativeWidget can be null when Widget is closing.
  if (bridge)
    bridge->ClearAssociationForView(host_);

  scaling_view_.reset();
}

void NativeViewHostMac::AddedToWidget() {
  if (!host_->native_view())
    return;

  AttachNativeView();
  host_->Layout();
}

void NativeViewHostMac::RemovedFromWidget() {
  if (!host_->native_view())
    return;

  NativeViewDetaching(false);
}

bool NativeViewHostMac::SetCornerRadius(int corner_radius) {
  NOTIMPLEMENTED();
  return false;
}

void NativeViewHostMac::InstallClip(int x, int y, int w, int h) {
  NOTIMPLEMENTED();
}

bool NativeViewHostMac::HasInstalledClip() {
  return false;
}

void NativeViewHostMac::UninstallClip() {
  NOTIMPLEMENTED();
}

void NativeViewHostMac::ShowWidget(int x,
                                   int y,
                                   int w,
                                   int h,
                                   int render_w,
                                   int render_h) {
  DCHECK(scaling_view_);  // AttachNativeView() should have been called.

  if (host_->fast_resize())
    NOTIMPLEMENTED();

  // Coordinates will be from the top left of the parent Widget. The NativeView
  // is already in the same NSWindow, so just flip to get Cooca coordinates and
  // then convert to the containing view.
  NSRect window_rect = NSMakeRect(
      x,
      host_->GetWidget()->GetClientAreaBoundsInScreen().height() - y - h,
      w,
      h);

  // Convert window coordinates to the hosted view's superview, since that's how
  // coordinates of the hosted view's frame is based.
  NSRect container_rect =
      [[scaling_view_ superview] convertRect:window_rect fromView:nil];
  [scaling_view_ showWithFrame:container_rect
                       andSize:NSMakeSize(render_w, render_h)];
  [host_->native_view() setHidden:NO];
}

void NativeViewHostMac::HideWidget() {
  [host_->native_view() setHidden:YES];
}

void NativeViewHostMac::SetFocus() {
  if ([host_->native_view() acceptsFirstResponder])
    [[host_->native_view() window] makeFirstResponder:host_->native_view()];
}

gfx::NativeViewAccessible NativeViewHostMac::GetNativeViewAccessible() {
  return NULL;
}

gfx::NativeCursor NativeViewHostMac::GetCursor(int x, int y) {
  // Intentionally not implemented: Not required on non-aura Mac because OSX
  // will query the native view for the cursor directly. For NativeViewHostMac
  // in practice, OSX will retrieve the cursor that was last set by
  // -[RenderWidgetHostViewCocoa updateCursor:] whenever the pointer is over the
  // hosted view. With some plumbing, NativeViewHostMac could return that same
  // cursor here, but it doesn't achieve anything. The implications of returning
  // null simply mean that the "fallback" cursor on the window itself will be
  // cleared (see -[NativeWidgetMacNSWindow cursorUpdate:]). However, while the
  // pointer is over a RenderWidgetHostViewCocoa, OSX won't ask for the fallback
  // cursor.
  return gfx::kNullCursor;
}

// static
NativeViewHostWrapper* NativeViewHostWrapper::CreateWrapper(
    NativeViewHost* host) {
  return new NativeViewHostMac(host);
}

}  // namespace views
