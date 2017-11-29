// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_METRICS_TAB_STATS_DATA_STORE_H_
#define CHROME_BROWSER_METRICS_TAB_STATS_DATA_STORE_H_

#include <map>
#include <memory>
#include <set>
#include <vector>

#include "base/sequence_checker.h"
#include "content/public/browser/web_contents_observer.h"

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
    // Indicates if the tab is still present at the end of the interval.
    bool exists_after_interval;
    // Whether or not the tab has been visible at any moment during the
    // interval.
    bool visible_during_interval;
    // Indicates if the tab has been interacted with or became active during the
    // interval.
    bool interacted_during_interval;
  };

  // Represents the state of a set of tabs during an interval of time.
  typedef std::map<content::WebContents*, TabStateDuringInterval>
      TabsStateDuringIntervalMap;

  explicit TabStatsDataStore(PrefService* pref_service);
  virtual ~TabStatsDataStore();

  // Functions used to update the window/tab count. Virtual for unittesting.
  void OnWindowAdded();
  void OnWindowRemoved();
  virtual void OnTabAdded(content::WebContents* web_contents);
  virtual void OnTabRemoved(content::WebContents* web_contents);

  // Update the maximum number of tabs in a single window if |value| exceeds
  // this.
  // TODO(sebmarchand): Store a list of windows in this class and track the
  // number of tabs per window.
  void UpdateMaxTabsPerWindowIfNeeded(size_t value);

  // Reset all the maximum values to the current state, to be used once the
  // metrics have been reported.
  void ResetMaximumsToCurrentState();

  // Adds a WebContents to the list of active tabs and update the interval maps
  // to indicate that it has been interacted with.
  void SetTabActive(content::WebContents* web_contents);

  // Removes a WebContents from the list of active tabs.
  void SetTabUnactive(content::WebContents* web_contents);

  // Records that a tab has been interacted with.
  void OnTabInteraction(content::WebContents* web_contents);

  // Records that a tab became visible.
  void OnTabVisible(content::WebContents* web_contents);

  // Creates a new interval map for the given interval.
  void AddInterval(size_t interval_time_in_sec);

  // Returns the interval map for the duration specified.
  TabsStateDuringIntervalMap* GetIntervalMap(size_t interval_time_in_sec);

  // Reset |interval_map| with the list of current tabs.
  void ResetIntervalData(TabsStateDuringIntervalMap* interval_map);

  const TabsStats& tab_stats() const { return tab_stats_; }

 protected:
  // DO NOT SUBMIT
  class WebContentsUsageObserver : public content::WebContentsObserver {
   public:
    WebContentsUsageObserver(content::WebContents* web_contents,
                             TabStatsDataStore* data_store)
        : content::WebContentsObserver(web_contents), data_store_(data_store) {}

    // content::WebContentsObserver:
    void DidGetUserInteraction(const blink::WebInputEvent::Type type) override;
    void WasShown() override;

    // TODO(sebmarchand): Override the MediaStartedPlaying/MediaStoppedPlaying
    // functions?
   private:
    TabStatsDataStore* data_store_;
  };

  // Update the maximums metrics if needed.
  void UpdateTotalTabCountMaxIfNeeded();
  void UpdateWindowCountMaxIfNeeded();

  // For unittesting.
  TabsStats& tab_stats_mutable() { return tab_stats_; }

 private:
  // The tabs stats.
  TabsStats tab_stats_;

  // A raw pointer to the PrefService used to read and write the statistics.
  PrefService* pref_service_;

  // The existing tabs.
  std::set<content::WebContents*> existing_tabs_;

  // The current active tabs.
  std::set<content::WebContents*> active_tabs_;

  // The interval maps, one per period of time that we want to observe.
  std::map<size_t, std::unique_ptr<TabsStateDuringIntervalMap>> interval_maps_;

  // The observers that tracks how the tabs are used.
  std::map<content::WebContents*, std::unique_ptr<WebContentsUsageObserver>>
      web_contents_usage_observers_;

  SEQUENCE_CHECKER(sequence_checker_);

  DISALLOW_COPY_AND_ASSIGN(TabStatsDataStore);
};

}  // namespace metrics

#endif  // CHROME_BROWSER_METRICS_TAB_STATS_DATA_STORE_H_
