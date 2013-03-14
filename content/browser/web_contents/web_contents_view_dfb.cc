// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/web_contents/web_contents_view_dfb.h"

#include <algorithm>

#include "base/string_util.h"
#include "base/utf_string_conversions.h"
#include "build/build_config.h"
#include "content/browser/renderer_host/render_view_host_factory.h"
#include "content/browser/renderer_host/render_view_host_impl.h"
#include "content/browser/renderer_host/render_widget_host_view_dfb.h"
#include "content/browser/web_contents/interstitial_page_impl.h"
#include "content/browser/web_contents/web_contents_impl.h"
#include "content/public/browser/web_contents_delegate.h"
#include "content/public/browser/web_contents_view_delegate.h"

#include "ui/gfx/image/image_skia.h"
#include "ui/gfx/point.h"
#include "ui/gfx/rect.h"
#include "ui/gfx/size.h"
#include "webkit/glue/webdropdata.h"

using WebKit::WebDragOperation;
using WebKit::WebDragOperationsMask;

namespace content {

WebContentsView* CreateWebContentsView(
    WebContentsImpl* web_contents,
    WebContentsViewDelegate* delegate,
    RenderViewHostDelegateView** render_view_host_delegate_view) {
  WebContentsViewDfb* rv = new WebContentsViewDfb(web_contents, delegate);
  *render_view_host_delegate_view = rv;
  return rv;
}

WebContentsViewDfb::WebContentsViewDfb(
    WebContentsImpl* web_contents,
    WebContentsViewDelegate* delegate)
    : web_contents_(web_contents),
      delegate_(delegate) {
}

WebContentsViewDfb::~WebContentsViewDfb() {
}

void WebContentsViewDfb::CreateView(
    const gfx::Size& initial_size, gfx::NativeView context) {
  requested_size_ = initial_size;
}

RenderWidgetHostView* WebContentsViewDfb::CreateViewForWidget(
    RenderWidgetHost* render_widget_host) {
  if (render_widget_host->GetView()) {
    // During testing, the view will already be set up in most cases to the
    // test view, so we don't want to clobber it with a real one. To verify that
    // this actually is happening (and somebody isn't accidentally creating the
    // view twice), we check for the RVH Factory, which will be set when we're
    // making special ones (which go along with the special views).
    DCHECK(RenderViewHostFactory::has_factory());
    return render_widget_host->GetView();
  }

  RenderWidgetHostView* view =
      RenderWidgetHostView::CreateViewForWidget(render_widget_host);
  view->InitAsChild(NULL);

  return view;
}

RenderWidgetHostView* WebContentsViewDfb::CreateViewForPopupWidget(
    RenderWidgetHost* render_widget_host) {
  return RenderWidgetHostViewPort::CreateViewForWidget(render_widget_host);
}

gfx::NativeView WebContentsViewDfb::GetNativeView() const {
  NOTIMPLEMENTED();
  return NULL;
}

gfx::NativeView WebContentsViewDfb::GetContentNativeView() const {
  RenderWidgetHostView* rwhv = web_contents_->GetRenderWidgetHostView();
  if (!rwhv)
    return NULL;
  return rwhv->GetNativeView();
}

gfx::NativeWindow WebContentsViewDfb::GetTopLevelNativeWindow() const {
  NOTIMPLEMENTED();
  return NULL;
}

void WebContentsViewDfb::GetContainerBounds(gfx::Rect* out) const {
  NOTIMPLEMENTED();
}

void WebContentsViewDfb::SetPageTitle(const string16& title) {
  NOTIMPLEMENTED();
}

void WebContentsViewDfb::OnTabCrashed(base::TerminationStatus status,
                                      int error_code) {
}

void WebContentsViewDfb::SizeContents(const gfx::Size& size) {
  // We don't need to manually set the size of of widgets in DFB+, but we do
  // need to pass the sizing information on to the RWHV which will pass the
  // sizing information on to the renderer.
  requested_size_ = size;
  RenderWidgetHostView* rwhv = web_contents_->GetRenderWidgetHostView();
  if (rwhv)
    rwhv->SetSize(size);
}

void WebContentsViewDfb::RenderViewCreated(RenderViewHost* host) {
}

void WebContentsViewDfb::Focus() {
  NOTIMPLEMENTED();
}

void WebContentsViewDfb::SetInitialFocus() {
  NOTIMPLEMENTED();
}

void WebContentsViewDfb::StoreFocus() {
  NOTIMPLEMENTED();
}

void WebContentsViewDfb::RestoreFocus() {
  NOTIMPLEMENTED();
}

WebDropData* WebContentsViewDfb::GetDropData() const {
  NOTIMPLEMENTED();
  return NULL;
}

bool WebContentsViewDfb::IsEventTracking() const {
  return false;
}

void WebContentsViewDfb::CloseTabAfterEventTracking() {
}

gfx::Rect WebContentsViewDfb::GetViewBounds() const {
  gfx::Rect rect;
  rect.SetRect(0, 0, 1368, 768);
  return rect;
}

WebContents* WebContentsViewDfb::web_contents() {
  return web_contents_;
}

void WebContentsViewDfb::UpdateDragCursor(WebDragOperation operation) {
  NOTIMPLEMENTED();
}

void WebContentsViewDfb::GotFocus() {
  // This is only used in the views FocusManager stuff but it bleeds through
  // all subclasses. http://crbug.com/21875
}

// This is called when the renderer asks us to take focus back (i.e., it has
// iterated past the last focusable element on the page).
void WebContentsViewDfb::TakeFocus(bool reverse) {
  NOTIMPLEMENTED();
}

void WebContentsViewDfb::ShowContextMenu(
    const ContextMenuParams& params,
    ContextMenuSourceType type) {
  if (delegate_.get())
    delegate_->ShowContextMenu(params, type);
  else
    DLOG(ERROR) << "Cannot show context menus without a delegate.";
}

void WebContentsViewDfb::ShowPopupMenu(const gfx::Rect& bounds,
                                       int item_height,
                                       double item_font_size,
                                       int selected_item,
                                       const std::vector<WebMenuItem>& items,
                                       bool right_aligned,
                                       bool allow_multiple_selection) {
  // External popup menus are only used on Mac and Android.
  NOTIMPLEMENTED();
}

// Render view DnD -------------------------------------------------------------

void WebContentsViewDfb::StartDragging(const WebDropData& drop_data,
                                       WebDragOperationsMask ops,
                                       const gfx::ImageSkia& image,
                                       const gfx::Vector2d& image_offset,
                                       const DragEventSourceInfo& event_info) {
  NOTIMPLEMENTED();
}

}  // namespace content
