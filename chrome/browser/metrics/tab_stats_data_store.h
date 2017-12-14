// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_METRICS_TAB_STATS_DATA_STORE_H_
#define CHROME_BROWSER_METRICS_TAB_STATS_DATA_STORE_H_

#include <map>
#include <memory>
#include <vector>

#include "base/containers/flat_map.h"
#include "base/containers/flat_set.h"
#include "base/sequence_checker.h"

class PrefService;

namespace content {
class WebContents;
}  // namespace content

namespace metrics {

// The data store that keeps track of all the information that the
// TabStatsTracker wants to track.
class TabStatsDataStore {
 public:
  // Houses all of the statistics gathered by the TabStatsTracker.
  struct TabsStats {
    // Constructor, initializes everything to zero.
    TabsStats();

    // The total number of tabs opened across all the windows.
    size_t total_tab_count;

    // The maximum total number of tabs that has been opened across all the
    // windows at the same time.
    size_t total_tab_count_max;

    // The maximum total number of tabs that has been opened at the same time in
    // a single window.
    size_t max_tab_per_window;

    // The total number of windows opened.
    size_t window_count;

    // The maximum total number of windows opened at the same time.
    size_t window_count_max;
  };

  // Structure describing the state of a tab during an interval of time.
  struct TabStateDuringInterval {
    // Indicates if the tab exists at the beginning of the interval.
    bool existed_before_interval;
    // Indicates if the tab still exists.
    bool exists_currently;
    // Whether or not the tab has been visible or audible at any moment during
    // the interval.
    bool visible_or_audible_during_interval;
    // Indicates if the tab has been interacted with or became active during the
    // interval.
    bool interacted_during_interval;
  };

  using WebContentsID = size_t;
  // Represents the state of a set of tabs during an interval of time.
  using TabsStateDuringIntervalMap =
      std::map<WebContentsID, TabStateDuringInterval>;

  explicit TabStatsDataStore(PrefService* pref_service);
  virtual ~TabStatsDataStore();

  // Functions used to update the window/tab count.
  void OnWindowAdded();
  void OnWindowRemoved();
  // Virtual for unittesting.
  virtual void OnTabAdded(content::WebContents* web_contents);
  virtual void OnTabRemoved(content::WebContents* web_contents);
  void OnTabReplaced(content::WebContents* old_contents,
                     content::WebContents* new_contents);

  // Update the maximum number of tabs in a single window if |value| exceeds
  // this.
  // TODO(sebmarchand): Store a list of windows in this class and track the
  // number of tabs per window.
  void UpdateMaxTabsPerWindowIfNeeded(size_t value);

  // Reset all the maximum values to the current state, to be used once the
  // metrics have been reported.
  void ResetMaximumsToCurrentState();

  // Records that a tab became audible.
  void OnTabAudible(content::WebContents* web_contents);

  // Records that there's been a direct user interaction with a tab, see the
  // comment for |DidGetUserInteraction| in
  // content/public/browser/web_contents_observer.h for a list of the possible
  // type of interactions.
  void OnTabInteraction(content::WebContents* web_contents);

  // Records that a tab became visible.
  void OnTabVisible(content::WebContents* web_contents);

  // Creates a new interval map for the given interval.
  TabsStateDuringIntervalMap* AddInterval();

  // Reset |interval_map| with the list of current tabs.
  void ResetIntervalData(TabsStateDuringIntervalMap* interval_map);

  const TabsStats& tab_stats() const { return tab_stats_; }

  WebContentsID get_web_contents_id_for_testing(
      content::WebContents* web_contents) {
    return GetWebContentsID(web_contents);
  }

 protected:
  // Update the maximums metrics if needed.
  void UpdateTotalTabCountMaxIfNeeded();
  void UpdateWindowCountMaxIfNeeded();

  // Adds a tab to an interval map.
  void AddTabToInterval(TabsStateDuringIntervalMap* interval_map,
                        content::WebContents* web_contents,
                        bool existed_before_interval);

  // Returns the WebContentsID associated with a tab.
  WebContentsID GetWebContentsID(content::WebContents* web_contents);

 private:
  // The tabs stats.
  TabsStats tab_stats_;

  // A raw pointer to the PrefService used to read and write the statistics.
  PrefService* pref_service_;

  // The interval maps, one per period of time that we want to observe.
  std::vector<std::unique_ptr<TabsStateDuringIntervalMap>> interval_maps_;

  // The existing tabs.
  base::flat_map<content::WebContents*, WebContentsID> existing_tabs_;

  SEQUENCE_CHECKER(sequence_checker_);

  DISALLOW_COPY_AND_ASSIGN(TabStatsDataStore);
};

}  // namespace metrics

#endif  // CHROME_BROWSER_METRICS_TAB_STATS_DATA_STORE_H_
