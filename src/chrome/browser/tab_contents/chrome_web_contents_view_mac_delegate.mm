// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/tab_contents/chrome_web_contents_view_mac_delegate.h"

#import "chrome/browser/renderer_host/chrome_render_widget_host_view_mac_delegate.h"
#include "chrome/browser/tab_contents/render_view_context_menu_mac.h"
#include "chrome/browser/tab_contents/web_drag_bookmark_handler_mac.h"
#import "chrome/browser/ui/cocoa/view_id_util.h"
#include "content/browser/renderer_host/render_widget_host_view.h"
#include "content/public/browser/web_contents.h"

namespace chrome_web_contents_view_mac_delegate {
content::WebContentsViewMacDelegate* CreateWebContentsViewMacDelegate(
    content::WebContents* web_contents) {
  return new ChromeWebContentsViewMacDelegate(web_contents);
}
}

ChromeWebContentsViewMacDelegate::ChromeWebContentsViewMacDelegate(
    content::WebContents* web_contents)
    : bookmark_handler_(new WebDragBookmarkHandlerMac),
      web_contents_(web_contents) {
}

ChromeWebContentsViewMacDelegate::~ChromeWebContentsViewMacDelegate() {
}

NSObject<RenderWidgetHostViewMacDelegate>*
ChromeWebContentsViewMacDelegate::CreateRenderWidgetHostViewDelegate(
    RenderWidgetHost* render_widget_host) {
  return [[ChromeRenderWidgetHostViewMacDelegate alloc]
      initWithRenderWidgetHost:render_widget_host];
}

content::WebDragDestDelegate* ChromeWebContentsViewMacDelegate::DragDelegate() {
  return static_cast<content::WebDragDestDelegate*>(bookmark_handler_.get());
}

void ChromeWebContentsViewMacDelegate::ShowContextMenu(
    const content::ContextMenuParams& params) {
  // The renderer may send the "show context menu" message multiple times, one
  // for each right click mouse event it receives. Normally, this doesn't happen
  // because mouse events are not forwarded once the context menu is showing.
  // However, there's a race - the context menu may not yet be showing when
  // the second mouse event arrives. In this case, |ShowContextMenu()| will
  // get called multiple times - if so, don't create another context menu.
  // TODO(asvitkine): Fix the renderer so that it doesn't do this.
  RenderWidgetHostView* widget_view = web_contents_->GetRenderWidgetHostView();
  if (widget_view && widget_view->IsShowingContextMenu())
    return;

  context_menu_.reset(
      new RenderViewContextMenuMac(web_contents_,
                                   params,
                                   web_contents_->GetContentNativeView()));
  context_menu_->Init();
}

void ChromeWebContentsViewMacDelegate::NativeViewCreated(NSView* view) {
  view_id_util::SetID(view, VIEW_ID_TAB_CONTAINER);
}

void ChromeWebContentsViewMacDelegate::NativeViewDestroyed(NSView* view) {
  view_id_util::UnsetID(view);
}
