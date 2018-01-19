// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_KEYBOARD_LOCK_ACTIVE_KEY_FILTER_H_
#define CONTENT_BROWSER_KEYBOARD_LOCK_ACTIVE_KEY_FILTER_H_

#include "base/macros.h"
#include "ui/events/event.h"
#include "ui/events/platform/key_event_filter.h"

namespace content {

class RenderWidgetHost;

// Filters out key events which have been requested by the KeyboardLock API.
// TODO(joedow): Implement selective kb lock, i.e. only trap specific keys.
class ActiveKeyFilter final : public ui::KeyEventFilter {
 public:
  explicit ActiveKeyFilter(RenderWidgetHost* host);
  ~ActiveKeyFilter() override;

  // ui::KeyEventFilter interface.
  bool OnKeyEvent(const ui::KeyEvent& event) override;

 private:
  RenderWidgetHost* const host_;

  DISALLOW_COPY_AND_ASSIGN(ActiveKeyFilter);
};

}  // namespace content

#endif  // CONTENT_BROWSER_KEYBOARD_LOCK_ACTIVE_KEY_FILTER_H_
