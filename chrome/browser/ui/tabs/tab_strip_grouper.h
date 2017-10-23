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
#include "ui/base/models/list_selection_model.h"

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

  explicit TabStripGrouper(TabStripModel* model);
  ~TabStripGrouper() override;

  TabStripModel* model() const { return model_; }

  // This selection model will mirror that of the TabStripModel but take into
  // account item rather than model indices.
  const ui::ListSelectionModel& selection_model() const {
    return selection_model_;
  }

  int count() const { return static_cast<int>(items_.size()); }

  // Returns true if the index is a valid item index.
  bool ContainsIndex(int item_index) const;

  // The returned pointer will not be valid across any changes to the grouper
  // (or underlying model, whcih will trigger updates in the grouper).
  TabStripGrouperItem* GetItemAt(int index);

  // Computes the index of the item that includes the given index in the model.
  // If the item is off the end, then items.size() will be returned. This
  // could be the case when inserting or removing items at the end. The values
  // of -1 is special-cased and passed through.
  int ItemIndexForModelIndex(int model_index) const;

  // If the item is a group, returns the model index of the first item in the
  // group. If the item index is off the end, model_->count() will be returned.
  // The values of -1 is special-cased and passed through.
  int ModelIndexForItemIndex(int item_index) const;

  void SetViewParams(const ViewParams& params);

  void AddObserver(TabStripGrouperObserver* observer);
  void RemoveObserver(TabStripGrouperObserver* observer);

 private:
  // Computes the new groups and sends notifications. Returns true if anything
  // changed.
  bool Regroup();

  std::vector<TabStripGrouperItem> ComputeGroups() const;

  // Converts a selection model from using model indices to item indices.
  ui::ListSelectionModel ItemSelectionForModelSelection(
      const ui::ListSelectionModel& source) const;

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
                        int model_index,
                        int reason) override;
  void TabSelectionChanged(TabStripModel* tab_strip_model,
                           const ui::ListSelectionModel& old_model) override;
  void TabMoved(content::WebContents* contents,
                int from_model_index,
                int to_model_index) override;
  void TabChangedAt(content::WebContents* contents,
                    int model_index,
                    TabChangeType change_type) override;
  void TabReplacedAt(TabStripModel* tab_strip_model,
                     content::WebContents* old_contents,
                     content::WebContents* new_contents,
                     int model_index) override;
  void TabPinnedStateChanged(TabStripModel* tab_strip_model,
                             content::WebContents* contents,
                             int model_index) override;
  void TabBlockedStateChanged(content::WebContents* contents,
                              int index) override;
  void TabStripEmpty() override;
  void WillCloseAllTabs() override;
  void CloseAllTabsCanceled() override;
  void TabNeedsAttentionAt(int model_index) override;

  TabStripModel* model_;
  ui::ListSelectionModel selection_model_;
  ViewParams view_params_;

  std::vector<TabStripGrouperItem> items_;

  base::ObserverList<TabStripGrouperObserver> observers_;

  DISALLOW_COPY_AND_ASSIGN(TabStripGrouper);
};

#endif  // CHROME_BROWSER_UI_TABS_TAB_STRIP_GROUPER_H_
