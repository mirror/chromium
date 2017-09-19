// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_COCOA_TAB_CONTENTS_CHROME_WEB_CONTENTS_VIEW_DELEGATE_MAC_H_
#define CHROME_BROWSER_UI_COCOA_TAB_CONTENTS_CHROME_WEB_CONTENTS_VIEW_DELEGATE_MAC_H_

#if defined(__OBJC__)

#include <memory>

#include "base/mac/scoped_nsobject.h"
#include "base/macros.h"
#include "components/renderer_context_menu/context_menu_delegate.h"
#include "content/public/browser/web_contents_view_delegate.h"

@class FocusTracker;

class RenderViewContextMenuBase;
class WebDragBookmarkHandlerMac;

namespace content {
class RenderWidgetHostView;
class WebContents;
}

// A chrome/ specific class that extends WebContentsViewMac with features that
// live in chrome/.
class ChromeWebContentsViewDelegateMac
    : public content::WebContentsViewDelegate,
      public ContextMenuDelegate {
 public:
  explicit ChromeWebContentsViewDelegateMac(content::WebContents* web_contents);
  ~ChromeWebContentsViewDelegateMac() override;

  // Overridden from WebContentsViewDelegate:
  gfx::NativeWindow GetNativeWindow() override;
  NSObject<RenderWidgetHostViewMacDelegate>* CreateRenderWidgetHostViewDelegate(
      content::RenderWidgetHost* render_widget_host) override;
  content::WebDragDestDelegate* GetDragDestDelegate() override;
  void ShowContextMenu(content::RenderFrameHost* render_frame_host,
                       const content::ContextMenuParams& params) override;
  bool Focus() override;
  void StoreFocus() override;
  void RestoreFocus() override;

  // Overridden from ContextMenuDelegate.
  std::unique_ptr<RenderViewContextMenuBase> BuildMenu(
      content::WebContents* web_contents,
      const content::ContextMenuParams& params) override;
  void ShowMenu(std::unique_ptr<RenderViewContextMenuBase> menu) override;

 private:
  content::RenderWidgetHostView* GetActiveRenderWidgetHostView();
  void SetInitialFocus();

  // Returns the fullscreen view, if one exists; otherwise, returns the content
  // native view. This ensures that the view currently attached to a NSWindow is
  // being used to query or set first responder state.
  gfx::NativeView GetNativeViewForFocus() const;

  // The context menu. Callbacks are asynchronous so we need to keep it around.
  std::unique_ptr<RenderViewContextMenuBase> context_menu_;

  // The chrome specific delegate that receives events from WebDragDestMac.
  std::unique_ptr<WebDragBookmarkHandlerMac> bookmark_handler_;

  // The WebContents that owns the view.
  content::WebContents* web_contents_;

  // Keeps track of which NSView has focus so we can restore the focus when
  // focus returns.
  base::scoped_nsobject<FocusTracker> focus_tracker_;

  DISALLOW_COPY_AND_ASSIGN(ChromeWebContentsViewDelegateMac);
};

#endif  // __OBJC__

#endif  // CHROME_BROWSER_UI_COCOA_TAB_CONTENTS_CHROME_WEB_CONTENTS_VIEW_DELEGATE_MAC_H_
