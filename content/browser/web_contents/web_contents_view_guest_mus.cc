// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/web_contents/web_contents_view_guest_mus.h"

#include "base/command_line.h"
#include "components/viz/common/switches.h"
#include "content/browser/renderer_host/render_widget_host_view_aura.h"
#include "content/browser/web_contents/web_contents_impl.h"
#include "content/public/browser/web_contents_view_delegate.h"
#include "ui/gfx/geometry/rect.h"

namespace content {

// NOTE: WebContentsViewGuestMus is *not* set as the WebContentsView of a
// WebContents, rather WebContentsViewGuestMus is held in
// WebContentsViewGuest::platform_view_. WebContentsViewGuest is installed as
// the WebContentsView on the WebContents and WebContentsViewGuest calls to
// WebContentsViewGuestMus for a handful of functions. Because of this, many
// of the WebContentsView functions here are not called at all and are
// NOTREACHED.

WebContentsViewGuestMus::WebContentsViewGuestMus(
    WebContentsImpl* web_contents,
    WebContentsViewDelegate* delegate)
    : web_contents_(web_contents), delegate_(delegate) {}

WebContentsViewGuestMus::~WebContentsViewGuestMus() {}

gfx::NativeView WebContentsViewGuestMus::GetNativeView() const {
  DCHECK(web_contents_->GetOuterWebContents());
  return web_contents_->GetOuterWebContents()->GetView()->GetNativeView();
}

gfx::NativeView WebContentsViewGuestMus::GetContentNativeView() const {
  DCHECK(web_contents_->GetOuterWebContents());
  return web_contents_->GetOuterWebContents()
      ->GetView()
      ->GetContentNativeView();
}

gfx::NativeWindow WebContentsViewGuestMus::GetTopLevelNativeWindow() const {
  DCHECK(web_contents_->GetOuterWebContents());
  return web_contents_->GetOuterWebContents()
      ->GetView()
      ->GetTopLevelNativeWindow();
}

void WebContentsViewGuestMus::GetScreenInfo(ScreenInfo* screen_info) const {
  DCHECK(web_contents_->GetOuterWebContents());
  return web_contents_->GetOuterWebContents()->GetScreenInfo(screen_info);
}

void WebContentsViewGuestMus::GetContainerBounds(gfx::Rect* out) const {
  DCHECK(web_contents_->GetOuterWebContents());
  return web_contents_->GetOuterWebContents()->GetView()->GetContainerBounds(
      out);
}

void WebContentsViewGuestMus::SizeContents(const gfx::Size& size) {
  NOTREACHED();
}

void WebContentsViewGuestMus::SetInitialFocus() {
  if (web_contents_->FocusLocationBarByDefault())
    web_contents_->SetFocusToLocationBar(false);
  else
    Focus();
}

void WebContentsViewGuestMus::Focus() {
  // TODO(sky): this needs to forward to BrowserPlugin asking it to focus.
  NOTIMPLEMENTED();
}

void WebContentsViewGuestMus::StoreFocus() {
  if (delegate_)
    delegate_->StoreFocus();
}

void WebContentsViewGuestMus::RestoreFocus() {
  if (delegate_)
    delegate_->RestoreFocus();
}

DropData* WebContentsViewGuestMus::GetDropData() const {
  NOTREACHED();
  return nullptr;
}

gfx::Rect WebContentsViewGuestMus::GetViewBounds() const {
  NOTREACHED();
  return gfx::Rect();
}

void WebContentsViewGuestMus::CreateView(const gfx::Size& initial_size,
                                         gfx::NativeView context) {
  // This does nothing, creation is handled entirely in the renderer.
}

RenderWidgetHostViewBase* WebContentsViewGuestMus::CreateViewForWidget(
    RenderWidgetHost* render_widget_host,
    bool is_guest_view_hack) {
  NOTREACHED();
  return nullptr;
}

RenderWidgetHostViewBase* WebContentsViewGuestMus::CreateViewForPopupWidget(
    RenderWidgetHost* render_widget_host) {
  const bool enable_surface_synchronization =
      base::CommandLine::ForCurrentProcess()->HasSwitch(
          switches::kEnableSurfaceSynchronization);
  const bool is_guest_view_hack = false;
  return new RenderWidgetHostViewAura(render_widget_host, is_guest_view_hack,
                                      enable_surface_synchronization);
}

void WebContentsViewGuestMus::SetPageTitle(const base::string16& title) {}

void WebContentsViewGuestMus::RenderViewCreated(RenderViewHost* host) {}

void WebContentsViewGuestMus::RenderViewSwappedIn(RenderViewHost* host) {}

void WebContentsViewGuestMus::SetOverscrollControllerEnabled(bool enabled) {}

}  // namespace content
