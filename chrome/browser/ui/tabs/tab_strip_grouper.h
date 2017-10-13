// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_TABS_TAB_STRIP_GROUPER_H_
#define CHROME_BROWSER_UI_TABS_TAB_STRIP_GROUPER_H_

#include "base/macros.h"
#include "base/observer_list.h"

class TabStripGroupAdapterObserver;
class TabStripModel;

// An adapter that sits between a view and a tab strip model. It will generate
// groups of tabs on top of the underlying linear model, and expose this to the
// view as a new virtual model.
class TabStripGrouper {
 public:
  TabStripGroupAdapter(TabStripModel* model);
  ~TabStripGroupAdapter();

  TabStripModel* model() const { return model_; }

  void AddObserver(TabStripGroupAdapterObserver* observer);
  void RemoveObserver(TabStripGroupAdapterObserver* observer);

 private:
  TabStripModel* model_;

  base::ObserverList<TabStripModelObserver> observers_;

  DISALLOW_COPY_AND_ASSIGN(TabStripGroupAdapter);
};

#endif  // CHROME_BROWSER_UI_TABS_TAB_STRIP_GROUPER_H_
