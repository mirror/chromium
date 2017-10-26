// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_WEB_CONTENTS_WEB_CONTENTS_VIEW_GUEST_MUS_H_
#define CONTENT_BROWSER_WEB_CONTENTS_WEB_CONTENTS_VIEW_GUEST_MUS_H_

#include "base/macros.h"
#include "content/browser/web_contents/web_contents_view.h"

namespace content {

class WebContentsImpl;
class WebContentsViewDelegate;

// Implementation of WebContentsView used when a WebContents is used for
// browser-plugin (aka guest) in mus. This class largely does nothing. This
// class is necessary as WebContentsView normally creates an aura::Window,
// which in the case of a browser plugin we don't want. Instead the renderer
// embedding the plugin creates the mus window (see BrowserPlugin).
class WebContentsViewGuestMus : public WebContentsView {
 public:
  WebContentsViewGuestMus(WebContentsImpl* web_contents,
                          WebContentsViewDelegate* delegate);
  ~WebContentsViewGuestMus() override;

  // WebContentsView implementation --------------------------------------------
  gfx::NativeView GetNativeView() const override;
  gfx::NativeView GetContentNativeView() const override;
  gfx::NativeWindow GetTopLevelNativeWindow() const override;
  void GetScreenInfo(ScreenInfo* screen_info) const override;
  void GetContainerBounds(gfx::Rect* out) const override;
  void SizeContents(const gfx::Size& size) override;
  void Focus() override;
  void SetInitialFocus() override;
  void StoreFocus() override;
  void RestoreFocus() override;
  DropData* GetDropData() const override;
  gfx::Rect GetViewBounds() const override;
  void CreateView(const gfx::Size& initial_size,
                  gfx::NativeView context) override;
  RenderWidgetHostViewBase* CreateViewForWidget(
      RenderWidgetHost* render_widget_host,
      bool is_guest_view_hack) override;
  RenderWidgetHostViewBase* CreateViewForPopupWidget(
      RenderWidgetHost* render_widget_host) override;
  void SetPageTitle(const base::string16& title) override;
  void RenderViewCreated(RenderViewHost* host) override;
  void RenderViewSwappedIn(RenderViewHost* host) override;
  void SetOverscrollControllerEnabled(bool enabled) override;

 private:
  // The WebContentsImpl whose contents we display.
  WebContentsImpl* web_contents_;

  WebContentsViewDelegate* delegate_;

  DISALLOW_COPY_AND_ASSIGN(WebContentsViewGuestMus);
};

}  // namespace content

#endif  // CONTENT_BROWSER_WEB_CONTENTS_WEB_CONTENTS_VIEW_GUEST_MUS_H_
