// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/renderer_host/render_widget_host_view.h"

#include "base/logging.h"
#include "content/browser/accessibility/browser_accessibility_manager.h"
#include "third_party/WebKit/Source/WebKit/chromium/public/WebScreenInfo.h"

#if defined(TOOLKIT_USES_GTK)
#include <gdk/gdkx.h>
#include <gtk/gtk.h>

#include "content/browser/renderer_host/gtk_window_utils.h"
#endif

RenderWidgetHostView::RenderWidgetHostView() {
}

RenderWidgetHostView::~RenderWidgetHostView() {
}

RenderWidgetHostViewBase::RenderWidgetHostViewBase()
    : popup_type_(WebKit::WebPopupTypeNone),
      mouse_locked_(false),
      showing_context_menu_(false),
      selection_text_offset_(0),
      selection_range_(ui::Range::InvalidRange()) {
}

RenderWidgetHostViewBase::~RenderWidgetHostViewBase() {
  DCHECK(!mouse_locked_);
}

// static
RenderWidgetHostViewBase* RenderWidgetHostViewBase::FromRWHV(
    RenderWidgetHostView* rwhv) {
  return static_cast<RenderWidgetHostViewBase*>(rwhv);
}

// static
RenderWidgetHostViewBase* RenderWidgetHostViewBase::CreateViewForWidget(
    RenderWidgetHost* widget) {
  return FromRWHV(RenderWidgetHostView::CreateViewForWidget(widget));
}

void RenderWidgetHostViewBase::SetBackground(const SkBitmap& background) {
  background_ = background;
}

const SkBitmap& RenderWidgetHostViewBase::GetBackground() {
  return background_;
}

BrowserAccessibilityManager*
    RenderWidgetHostViewBase::GetBrowserAccessibilityManager() const {
  return browser_accessibility_manager_.get();
}

void RenderWidgetHostViewBase::SetBrowserAccessibilityManager(
    BrowserAccessibilityManager* manager) {
  browser_accessibility_manager_.reset(manager);
}

void RenderWidgetHostViewBase::SelectionChanged(const string16& text,
                                                size_t offset,
                                                const ui::Range& range) {
  selection_text_ = text;
  selection_text_offset_ = offset;
  selection_range_.set_start(range.start());
  selection_range_.set_end(range.end());
}

bool RenderWidgetHostViewBase::IsShowingContextMenu() const {
  return showing_context_menu_;
}

void RenderWidgetHostViewBase::SetShowingContextMenu(bool showing) {
  DCHECK_NE(showing_context_menu_, showing);
  showing_context_menu_ = showing;
}
