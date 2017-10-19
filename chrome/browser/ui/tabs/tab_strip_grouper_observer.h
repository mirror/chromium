// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_TABS_TAB_STRIP_GROUPER_OBSERVER_H_
#define CHROME_BROWSER_UI_TABS_TAB_STRIP_GROUPER_OBSERVER_H_

#include "base/macros.h"

class TabStripGrouper;

namespace ui {
class ListSelectionModel;
}

class TabStripGrouperObserver {
 public:
  // Notification that tabs or groups have been added or removed. The removed
  // list contains the indices in the PREVIOUS tab model of the removed items.
  // The added list contains the indices relative to the NEW model.
  //
  // Both arrays will be sorted.
  virtual void ItemsChanged(const std::vector<int>& removed,
                            const std::vector<int>& added);

  virtual void ItemSelectionChanged(const ui::ListSelectionModel& old_model);

  virtual void ItemNeedsAttentionAt(int index);

 protected:
  TabStripGrouperObserver();
  virtual ~TabStripGrouperObserver();

 private:
  DISALLOW_COPY_AND_ASSIGN(TabStripGrouperObserver);
};

#endif  // CHROME_BROWSER_UI_TABS_TAB_STRIP_GROUPER_OBSERVER_H_
