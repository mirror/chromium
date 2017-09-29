// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "headless/lib/browser/headless_browser_impl.h"

#include "content/public/browser/web_contents.h"
#include "ui/base/cocoa/base_view.h"
#include "ui/gfx/mac/coordinate_conversion.h"

namespace headless {

namespace {

NSString* const kActivityReason = @"Batch headless process";
const NSActivityOptions kActivityOptions =
    (NSActivityUserInitiatedAllowingIdleSystemSleep |
     NSActivityLatencyCritical) &
    ~(NSActivitySuddenTerminationDisabled |
      NSActivityAutomaticTerminationDisabled);

}  // namespace

void HeadlessBrowserImpl::PlatformInitialize() {}

void HeadlessBrowserImpl::PlatformStart() {
  // Disallow headless to be throttled as a background process.
  [[NSProcessInfo processInfo] beginActivityWithOptions:kActivityOptions
                                                 reason:kActivityReason];
}

void HeadlessBrowserImpl::PlatformInitializeWebContents(
    HeadlessWebContentsImpl* web_contents) {
  NSView* web_view = web_contents->web_contents()->GetNativeView();
  [web_view setAutoresizingMask:(NSViewWidthSizable | NSViewHeightSizable)];
  // TODO(eseckler): Support enabling BeginFrameControl on Mac. This is tricky
  // because it's a ui::Compositor startup setting and ui::Compositors are
  // recycled on Mac, see browser_compositor_view_mac.mm.
}

void HeadlessBrowserImpl::PlatformSetWebContentsBounds(
    HeadlessWebContentsImpl* web_contents,
    const gfx::Rect& bounds) {
  NSView* web_view = web_contents->web_contents()->GetNativeView();
  NSRect frame = gfx::ScreenRectToNSRect(bounds);
  [web_view setFrame:frame];
}

ui::Compositor* HeadlessBrowserImpl::PlatformGetCompositor(
    HeadlessWebContentsImpl* web_contents) {
  // TODO(eseckler): Support BeginFrameControl on Mac.
  return nullptr;
}

}  // namespace headless
