// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_TABS_TAB_STRIP_GROUPER_H_
#define CHROME_BROWSER_UI_TABS_TAB_STRIP_GROUPER_H_

#include <vector>

#include "base/macros.h"
#include "base/observer_list.h"
#include "chrome/browser/ui/tabs/tab_strip_grouper_item.h"
#include "chrome/browser/ui/tabs/tab_strip_model_observer.h"

class TabStripGrouperObserver;
class TabStripModel;

// An adapter that sits between a view and a tab strip model. It will generate
// groups of tabs on top of the underlying linear model, and expose this to the
// view as a new virtual model.
class TabStripGrouper : public TabStripModelObserver {
 public:
  struct ViewParams {
    // Width of the space available for tabs.
    int width = 0;

    // Minimum width of visible tabs before grouping kicks in.
    int min_tab_width = 0;
  };

  TabStripGrouper(TabStripModel* model);
  ~TabStripGrouper();

  TabStripModel* model() const { return model_; }

  int count() const { return static_cast<int>(items_.size()); }

  // The returned pointer will not be valid across any changes to the grouper
  // (or underlying model, whcih will trigger updates in the grouper).
  TabStripGrouperItem* GetItemAt(int index);

  void SetViewParams(const ViewParams& params);

  void AddObserver(TabStripGrouperObserver* observer);
  void RemoveObserver(TabStripGrouperObserver* observer);

 private:
  void Regroup();

  std::vector<TabStripGrouperItem> ComputeGroups() const;

  // Computes the index of the item that includes the given index in the model.
  int ItemIndexForModelIndex(int model_index) const;

  // Adds "delta" to all model indices >= "starting_at". This will update the
  // items_ list when an item is added or removed from the mode.
  void ShiftModelIndices(int starting_at, int delta);

  // TabStripModelObserver implementation:
  void TabInsertedAt(TabStripModel* tab_strip_model,
                     content::WebContents* contents,
                     int model_index,
                     bool foreground) override;
  void TabClosingAt(TabStripModel* tab_strip_model,
                    content::WebContents* contents,
                    int index) override;
  void TabDetachedAt(content::WebContents* contents, int index) override;
  void TabDeactivated(content::WebContents* contents) override;
  void ActiveTabChanged(content::WebContents* old_contents,
                        content::WebContents* new_contents,
                        int index,
                        int reason) override;
  void TabSelectionChanged(TabStripModel* tab_strip_model,
                           const ui::ListSelectionModel& old_model) override;
  void TabMoved(content::WebContents* contents,
                int from_index,
                int to_index) override;
  void TabChangedAt(content::WebContents* contents,
                    int index,
                    TabChangeType change_type) override;
  void TabReplacedAt(TabStripModel* tab_strip_model,
                     content::WebContents* old_contents,
                     content::WebContents* new_contents,
                     int index) override;
  void TabPinnedStateChanged(TabStripModel* tab_strip_model,
                             content::WebContents* contents,
                             int index) override;
  void TabBlockedStateChanged(content::WebContents* contents,
                              int index) override;
  void TabStripEmpty() override;
  void WillCloseAllTabs() override;
  void CloseAllTabsCanceled() override;
  void TabNeedsAttentionAt(int index) override;

  TabStripModel* model_;
  ViewParams view_params_;

  std::vector<TabStripGrouperItem> items_;

  base::ObserverList<TabStripGrouperObserver> observers_;

  DISALLOW_COPY_AND_ASSIGN(TabStripGrouper);
};

#endif  // CHROME_BROWSER_UI_TABS_TAB_STRIP_GROUPER_H_
