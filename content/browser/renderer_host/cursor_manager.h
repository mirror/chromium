// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_RENDERER_HOST_CURSOR_MANAGER_H_
#define CONTENT_BROWSER_RENDERER_HOST_CURSOR_MANAGER_H_

#include <unordered_map>

#include "content/common/content_export.h"
#include "content/common/cursors/webcursor.h"

namespace content {

class RenderWidgetHostViewBase;

class CONTENT_EXPORT CursorManager {
 public:
  CursorManager(RenderWidgetHostViewBase* root);
  ~CursorManager();

  void UpdateCursor(RenderWidgetHostViewBase*, const WebCursor&);
  void UpdateViewUnderCursor(RenderWidgetHostViewBase*);

  // Notification of a RenderWidgetHostView being destroyed, so that its
  // cursor map entry can be removed if it has one. If it is the current
  // view_under_cursor_, then the root_view_'s cursor will be displayed.
  void ViewBeingDestroyed(RenderWidgetHostViewBase*);

  // Accessor for browser tests, enabling verification of the cursor_map_.
  // Returns false if the provided View is not in the map, and outputs
  // the cursor otherwise.
  bool GetCursorForTesting(RenderWidgetHostViewBase*, WebCursor&);

 private:
  // Stores the last received cursor from each RenderWidgetHostView.
  std::unordered_map<RenderWidgetHostViewBase*, WebCursor> cursor_map_;

  // The view currently underneath the cursor, which corresponds to the cursor
  // currently displayed.
  RenderWidgetHostViewBase* view_under_cursor_;

  // The root view is the target for DisplayCursor calls whenever the active
  // cursor needs to change.
  RenderWidgetHostViewBase* root_view_;
};

}  // namespace content

#endif  // CONTENT_BROWSER_RENDERER_HOST_CURSOR_MANAGER_H_