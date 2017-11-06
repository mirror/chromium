// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_VIEWS_TABS_TAB_STRIP_EXPERIMENTAL_H_
#define CHROME_BROWSER_UI_VIEWS_TABS_TAB_STRIP_EXPERIMENTAL_H_

#include "chrome/browser/ui/views/tabs/tab_strip.h"

class TabStripModel;
class TabStripModelExperimental;

class TabStripExperimental : public TabStrip {
 public:
  TabStripExperimental(TabStripModel* model);
  ~TabStripExperimental() override;

  // TabStripBase implementation:
  TabStripImpl* AsTabStripImpl() override;
  int GetMaxX() const override;
  void SetBackgroundOffset(const gfx::Point& offset) override;
  bool IsRectInWindowCaption(const gfx::Rect& rect) override;
  bool IsPositionInWindowCaption(const gfx::Point& point) override;
  bool IsTabStripCloseable() const override;
  bool IsTabStripEditable() const override;
  bool IsTabCrashed(int tab_index) const override;
  bool TabHasNetworkError(int tab_index) const override;
  TabAlertState GetTabAlertState(int tab_index) const override;
  void UpdateLoadingAnimations() override;

 private:
  // Non-owning.
  TabStripModelExperimental* model_;

  DISALLOW_COPY_AND_ASSIGN(TabStripExperimental);
};

#endif  // CHROME_BROWSER_UI_VIEWS_TABS_TAB_STRIP_EXPERIMENTAL_H_
