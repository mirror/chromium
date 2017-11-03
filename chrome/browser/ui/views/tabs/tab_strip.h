// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_VIEWS_TABS_TAB_STRIP_H_
#define CHROME_BROWSER_UI_VIEWS_TABS_TAB_STRIP_H_

#include "chrome/browser/ui/tabs/tab_utils.h"
#include "ui/views/view.h"

class TabStripObserver;

namespace gfx {
class Rect;
}

// A common base class for functions the BrowserView needs to call on the
// tab strip. This allows the normal tab strip to be swapped out for an
// experimental one.
class TabStrip : public views::View {
 public:
  // Add and remove observers to changes within this TabStrip.
  virtual void AddObserver(TabStripObserver* observer) = 0;
  virtual void RemoveObserver(TabStripObserver* observer) = 0;

  // Max x-coordinate the tabstrip draws at, which is the right edge of the new
  // tab button.
  virtual int GetMaxX() const = 0;

  // Set the background offset used by inactive tabs to match the frame image.
  virtual void SetBackgroundOffset(const gfx::Point& offset) = 0;

  // Returns true if the specified rect (in TabStrip coordinates) intersects
  // the window caption area of the browser window.
  virtual bool IsRectInWindowCaption(const gfx::Rect& rect) = 0;

  // Returns true if the specified point (in TabStrip coordinates) is in the
  // window caption area of the browser window.
  virtual bool IsPositionInWindowCaption(const gfx::Point& point) = 0;

  // Returns false when there is a drag operation in progress so that the frame
  // doesn't close.
  virtual bool IsTabStripCloseable() const = 0;

  // Returns true if the tab strip is editable. Returns false if the tab strip
  // is being dragged or animated to prevent extensions from messing things up
  // while that's happening.
  virtual bool IsTabStripEditable() const = 0;

  // Returns information about tabs at given indices.
  virtual bool IsTabCrashed(int tab_index) const = 0;
  virtual bool TabHasNetworkError(int tab_index) const = 0;
  virtual TabAlertState GetTabAlertState(int tab_index) const = 0;

  // Updates the loading animations displayed by tabs in the tabstrip to the
  // next frame.
  virtual void UpdateLoadingAnimations() = 0;
};

#endif  // CHROME_BROWSER_UI_VIEWS_TABS_TAB_STRIP_H_
