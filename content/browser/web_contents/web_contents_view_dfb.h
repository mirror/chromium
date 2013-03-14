// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_WEB_CONTENTS_WEB_CONTENTS_VIEW_DFB_H_
#define CONTENT_BROWSER_WEB_CONTENTS_WEB_CONTENTS_VIEW_DFB_H_

#include <vector>

#include "base/memory/scoped_ptr.h"
#include "content/common/content_export.h"
#include "content/common/drag_event_source_info.h"
#include "content/port/browser/render_view_host_delegate_view.h"
#include "content/public/browser/web_contents_view.h"

namespace content {

class WebContents;
class WebContentsImpl;
class WebContentsViewDelegate;

class CONTENT_EXPORT WebContentsViewDfb
    : public WebContentsView,
      public RenderViewHostDelegateView {
 public:
  // The corresponding WebContentsImpl is passed in the constructor, and manages
  // our lifetime. This doesn't need to be the case, but is this way currently
  // because that's what was easiest when they were split. We optionally take
  // |wrapper| which creates an intermediary widget layer for features from the
  // Embedding layer that lives with the WebContentsView.
  WebContentsViewDfb(WebContentsImpl* web_contents,
                     WebContentsViewDelegate* delegate);
  virtual ~WebContentsViewDfb();

  WebContentsViewDelegate* delegate() const { return delegate_.get(); }
  WebContents* web_contents();

  // WebContentsView implementation --------------------------------------------

  virtual void CreateView(
      const gfx::Size& initial_size, gfx::NativeView context) OVERRIDE;
  virtual RenderWidgetHostView* CreateViewForWidget(
      RenderWidgetHost* render_widget_host) OVERRIDE;
  virtual RenderWidgetHostView* CreateViewForPopupWidget(
      RenderWidgetHost* render_widget_host) OVERRIDE;
  virtual gfx::NativeView GetNativeView() const OVERRIDE;
  virtual gfx::NativeView GetContentNativeView() const OVERRIDE;
  virtual gfx::NativeWindow GetTopLevelNativeWindow() const OVERRIDE;
  virtual void GetContainerBounds(gfx::Rect* out) const OVERRIDE;
  virtual void SetPageTitle(const string16& title) OVERRIDE;
  virtual void OnTabCrashed(base::TerminationStatus status,
                            int error_code) OVERRIDE;
  virtual void SizeContents(const gfx::Size& size) OVERRIDE;
  virtual void RenderViewCreated(RenderViewHost* host) OVERRIDE;
  virtual void Focus() OVERRIDE;
  virtual void SetInitialFocus() OVERRIDE;
  virtual void StoreFocus() OVERRIDE;
  virtual void RestoreFocus() OVERRIDE;
  virtual WebDropData* GetDropData() const OVERRIDE;
  virtual bool IsEventTracking() const OVERRIDE;
  virtual void CloseTabAfterEventTracking() OVERRIDE;
  virtual gfx::Rect GetViewBounds() const OVERRIDE;

  // Backend implementation of RenderViewHostDelegateView.
  virtual void ShowContextMenu(
      const ContextMenuParams& params,
      ContextMenuSourceType type) OVERRIDE;
  virtual void ShowPopupMenu(const gfx::Rect& bounds,
                             int item_height,
                             double item_font_size,
                             int selected_item,
                             const std::vector<WebMenuItem>& items,
                             bool right_aligned,
                             bool allow_multiple_selection) OVERRIDE;
  virtual void StartDragging(const WebDropData& drop_data,
                             WebKit::WebDragOperationsMask allowed_ops,
                             const gfx::ImageSkia& image,
                             const gfx::Vector2d& image_offset,
                             const DragEventSourceInfo& event_info) OVERRIDE;
  virtual void UpdateDragCursor(WebKit::WebDragOperation operation) OVERRIDE;
  virtual void GotFocus() OVERRIDE;
  virtual void TakeFocus(bool reverse) OVERRIDE;

 private:
  // The WebContentsImpl whose contents we display.
  WebContentsImpl* web_contents_;

  // Our optional views wrapper. If non-NULL, we return this widget as our
  // GetNativeView() and insert |expanded_| as its child in the DfbWidget
  // hierarchy.
  scoped_ptr<WebContentsViewDelegate> delegate_;

  // The size we want the view to be.  We keep this in a separate variable
  // because resizing in DFB+ is async.
  gfx::Size requested_size_;

  DISALLOW_COPY_AND_ASSIGN(WebContentsViewDfb);
};

}  // namespace content

#endif  // CONTENT_BROWSER_WEB_CONTENTS_WEB_CONTENTS_VIEW_DFB_H_
