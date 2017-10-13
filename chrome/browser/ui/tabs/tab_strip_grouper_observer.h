// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_TABS_TAB_STRIP_GROUPER_OBSERVER_H_
#define CHROME_BROWSER_UI_TABS_TAB_STRIP_GROUPER_OBSERVER_H_

class TabStripGrouperObserver {
 public:

  virtual void ItemInsertedAt(TabStripGrouper* adapter,
                             

  virtual void ItemNeedsAttentionAt(int index);

 protected:
  TabStripGrouperObserver();
  virtual ~TabStripGrouperObserver();

 private:
  DISALLOW_COPY_AND_ASSIGN(TabStripGrouperObserver);
};

#endif  // CHROME_BROWSER_UI_TABS_TAB_STRIP_GROUPER_OBSERVER_H_
