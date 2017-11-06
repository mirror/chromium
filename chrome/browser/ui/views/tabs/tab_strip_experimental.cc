// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/tabs/tab_strip_experimental.h"

#include "base/logging.h"
#include "chrome/browser/ui/tabs/tab_strip_model_experimental.h"

TabStripExperimental::TabStripExperimental(TabStripModel* model)
    : model_(static_cast<TabStripModelExperimental*>(model)) {}

TabStripExperimental::~TabStripExperimental() {}

TabStripImpl* TabStripExperimental::AsTabStripImpl() {
  return nullptr;
}

int TabStripExperimental::GetMaxX() const {
  NOTIMPLEMENTED();
  return 100;
}

void TabStripExperimental::SetBackgroundOffset(const gfx::Point& offset) {
  NOTIMPLEMENTED();
}

bool TabStripExperimental::IsRectInWindowCaption(const gfx::Rect& rect) {
  NOTIMPLEMENTED();
  return false;
}

bool TabStripExperimental::IsPositionInWindowCaption(const gfx::Point& point) {
  NOTIMPLEMENTED();
  return false;
}

bool TabStripExperimental::IsTabStripCloseable() const {
  NOTIMPLEMENTED();
  return true;
}

bool TabStripExperimental::IsTabStripEditable() const {
  NOTIMPLEMENTED();
  return true;
}

bool TabStripExperimental::IsTabCrashed(int tab_index) const {
  NOTIMPLEMENTED();
  return false;
}

bool TabStripExperimental::TabHasNetworkError(int tab_index) const {
  NOTIMPLEMENTED();
  return false;
}

TabAlertState TabStripExperimental::GetTabAlertState(int tab_index) const {
  NOTIMPLEMENTED();
  return TabAlertState::NONE;
}

void TabStripExperimental::UpdateLoadingAnimations() {
  NOTIMPLEMENTED();
}
