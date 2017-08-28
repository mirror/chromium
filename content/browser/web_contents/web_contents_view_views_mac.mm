// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "content/browser/web_contents/web_contents_view_views_mac.h"

#include "content/browser/web_contents/web_contents_impl.h"
#include "content/public/browser/web_contents_delegate.h"
#include "ui/base/ui_features.h"
#import "ui/gfx/mac/coordinate_conversion.h"

using content::WebContentsImpl;
using content::WebContentsViewMac;

@interface WebContentsViewViewsCocoa (Private)
- (id)initWithWebContentsViewMac:(WebContentsViewMac*)w;
@end

namespace content {

#if BUILDFLAG(MAC_VIEWS_BROWSER)
WebContentsView* CreateWebContentsView(
    WebContentsImpl* web_contents,
    WebContentsViewDelegate* delegate,
    RenderViewHostDelegateView** render_view_host_delegate_view) {
  WebContentsViewMac* rv = new WebContentsViewViewsMac(web_contents, delegate);
  *render_view_host_delegate_view = rv;
  return rv;
}
#endif

WebContentsViewViewsMac::WebContentsViewViewsMac(
    WebContentsImpl* web_contents,
    WebContentsViewDelegate* delegate)
    : WebContentsViewMac(web_contents, delegate) {}

WebContentsViewViewsMac::~WebContentsViewViewsMac() {}

void WebContentsViewViewsMac::CreateView(const gfx::Size& initial_size,
                                         gfx::NativeView context) {
  WebContentsViewCocoa* view =
      [[WebContentsViewViewsCocoa alloc] initWithWebContentsViewMac:this];
  cocoa_view_.reset(view);
}

}  // namespace content

@implementation WebContentsViewViewsCocoa

- (id)initWithWebContentsViewMac:(WebContentsViewMac*)w {
  return [super initWithWebContentsViewMac:w];
}

- (void)mouseEvent:(NSEvent*)theEvent {
  WebContentsImpl* webContents = [self webContents];
  if (webContents && webContents->GetDelegate()) {
    NSPoint location = [NSEvent mouseLocation];
    webContents->GetDelegate()->ContentsMouseEvent(
        webContents, gfx::ScreenPointFromNSPoint(location),
        [theEvent type] == NSMouseMoved, [theEvent type] == NSMouseExited);
  }
}

@end
