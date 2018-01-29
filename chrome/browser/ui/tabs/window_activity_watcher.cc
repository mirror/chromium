// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/tabs/window_activity_watcher.h"

#include <algorithm>

#include "base/bits.h"
#include "base/optional.h"
#include "base/scoped_observer.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_list.h"
#include "chrome/browser/ui/browser_window.h"
#include "chrome/browser/ui/browser_window_observer.h"
#include "chrome/browser/ui/tabs/tab_metrics_event.pb.h"
#include "services/metrics/public/cpp/metrics_utils.h"
#include "services/metrics/public/cpp/ukm_builders.h"
#include "services/metrics/public/cpp/ukm_recorder.h"
#include "services/metrics/public/cpp/ukm_source_id.h"

using metrics::WindowMetricsEvent;

namespace {

// TabCount is bucketized using a spacing factor that ensures values up to 3 can
// be represented exactly. At 4+ tabs, we start bucketizing.
constexpr double kTabCountSpacingFactor = 1.5;
constexpr int kTabCountBucketStart = 4;

struct WindowMetrics {
  bool operator==(const WindowMetrics& other) {
    return window_id == other.window_id && type == other.type &&
           show_state == other.show_state && is_active == other.is_active &&
           tab_count == other.tab_count &&
           fullscreen_count == other.fullscreen_count &&
           maximize_count == other.maximize_count &&
           minimize_count == other.minimize_count;
  }

  bool operator!=(const WindowMetrics& other) { return !operator==(other); }

  // True if the struct has been sanitized and is suitable for UKM reporting.
  bool is_sanitized = false;

  // ID for the window, unique within the Chrome session.
  SessionID::id_type window_id;
  WindowMetricsEvent::Type type;
  WindowMetricsEvent::ShowState show_state;

  // True if this is the active (frontmost) window.
  bool is_active;

  // Number of tabs in the tab strip.
  int tab_count;

  // Number of times the window has entered various states. Updated when the
  // window state changes.
  int fullscreen_count = 0;
  int maximize_count = 0;
  int minimize_count = 0;
};

// Returns the WindowMetricsEvent show state for the window.
WindowMetricsEvent::ShowState GetWindowMetricsShowState(
    const BrowserWindow& window) {
  if (window.IsFullscreen())
    return WindowMetricsEvent::SHOW_STATE_FULLSCREEN;
  if (window.IsMaximized())
    return WindowMetricsEvent::SHOW_STATE_MAXIMIZED;
  if (window.IsMinimized())
    return WindowMetricsEvent::SHOW_STATE_MINIMIZED;
  return WindowMetricsEvent::SHOW_STATE_NORMAL;
}

// Sets metric values that are dependent on the current window state.
void UpdateMetrics(const Browser* browser, WindowMetrics* window_metrics) {
  DCHECK(browser->window());
  window_metrics->show_state = GetWindowMetricsShowState(*browser->window());
  window_metrics->is_active = browser->window()->IsActive();
  window_metrics->tab_count = browser->tab_strip_model()->count();
}

// Returns a populated WindowMetrics for the browser.
WindowMetrics CreateMetrics(const Browser* browser) {
  WindowMetrics window_metrics;
  window_metrics.window_id = browser->session_id().id();

  switch (browser->type()) {
    case Browser::TYPE_TABBED:
      window_metrics.type = WindowMetricsEvent::TYPE_TABBED;
      break;
    case Browser::TYPE_POPUP:
      window_metrics.type = WindowMetricsEvent::TYPE_POPUP;
      break;
  }

  UpdateMetrics(browser, &window_metrics);
  return window_metrics;
}

// Helper function to apply bucketization to a value.
// value: Value to be converted to the start value of its bucket.
// spacing_factor: Higher values generate larger buckets. Defaults to 2 for
// power-of-2 buckets.
void Bucketize(int* value, double spacing_factor = 2) {
  DCHECK(*value >= 0);

  if (spacing_factor == 2) {
    if (*value > 2)
      *value = 1 << base::bits::Log2Floor(*value);  // Round down to power of 2.
  } else {
    DCHECK_EQ(kTabCountSpacingFactor, spacing_factor)
        << "Unsupported spacing_factor";
    if (*value > kTabCountBucketStart)
      *value = ukm::GetExponentialBucketMin(*value, spacing_factor);
  }
}

// Returns a copy of |raw_metrics| with fields bucketized into appropriate
// ranges for privacy considerations.
WindowMetrics SanitizeWindowMetricsForUkm(const WindowMetrics& raw_metrics) {
  WindowMetrics metrics = raw_metrics;
  Bucketize(&metrics.fullscreen_count);
  Bucketize(&metrics.minimize_count);
  Bucketize(&metrics.maximize_count);
  Bucketize(&metrics.tab_count, kTabCountSpacingFactor);
  metrics.is_sanitized = true;
  return metrics;
}

// Logs a UKM entry with the metrics from |window_metrics|.
void LogWindowMetricsUkmEntry(const WindowMetrics& window_metrics) {
  CHECK(window_metrics.is_sanitized)
      << "Attempted to log WindowMetrics UKM without sanitizing first";

  ukm::UkmRecorder* ukm_recorder = ukm::UkmRecorder::Get();
  if (!ukm_recorder)
    return;

  ukm::builders::TabManager_WindowMetrics entry(ukm::AssignNewSourceId());
  entry.SetWindowId(window_metrics.window_id)
      .SetIsActive(window_metrics.is_active)
      .SetShowState(window_metrics.show_state)
      .SetType(window_metrics.type);

  entry.SetTabCount(window_metrics.tab_count)
      .SetMaximizeCount(window_metrics.maximize_count)
      .SetMinimizeCount(window_metrics.minimize_count)
      .SetFullscreenCount(window_metrics.fullscreen_count);

  entry.Record(ukm_recorder);
}

}  // namespace

// Observes a browser window's tab strip and logs a WindowMetrics UKM event for
// the window upon changes to metrics like TabCount.
class WindowActivityWatcher::BrowserWatcher : public TabStripModelObserver,
                                              public BrowserWindowObserver {
 public:
  explicit BrowserWatcher(Browser* browser)
      : browser_(browser), tab_strip_model_observer_(this) {
    DCHECK(!browser->profile()->IsOffTheRecord());
    current_metrics_ = CreateMetrics(browser_);
    tab_strip_model_observer_.Add(browser->tab_strip_model());
    browser->window()->AddObserver(this);
    MaybeLogWindowMetricsUkmEntry();
  }

  ~BrowserWatcher() override {
    // BrowserWatcher is normally destroyed when its browser is being removed.
    // In this case, the BrowserWindow is already destroyed.
    if (std::find(BrowserList::GetInstance()->begin(),
                  BrowserList::GetInstance()->end(),
                  browser_) != BrowserList::GetInstance()->end()) {
      browser_->window()->RemoveObserver(this);
    }
  }

  // Logs a new WindowMetrics entry to the UKM recorder if the entry would be
  // different than the last one we logged.
  void MaybeLogWindowMetricsUkmEntry() {
    // Do nothing if the window has no tabs (which can happen when a window is
    // opened, before a tab is added) or is being closed.
    if (browser_->tab_strip_model()->empty() ||
        browser_->tab_strip_model()->closing_all()) {
      return;
    }

    // Generate values for a UKM entry.
    UpdateMetrics(browser_, &current_metrics_);
    WindowMetrics new_ukm_metrics =
        SanitizeWindowMetricsForUkm(current_metrics_);

    // We should only log the entry if the sanitized UKM is different.
    if (!last_reported_metrics_ ||
        new_ukm_metrics != last_reported_metrics_.value()) {
      LogWindowMetricsUkmEntry(new_ukm_metrics);
      last_reported_metrics_ = new_ukm_metrics;
    }
  }

 private:
  // BrowserWindowObserver:
  void OnShowStateChanged() override {
    WindowMetricsEvent::ShowState new_show_state =
        GetWindowMetricsShowState(*browser_->window());
    // Do nothing if the show state hasn't changed.
    if (last_reported_metrics_ &&
        new_show_state == last_reported_metrics_->show_state) {
      return;
    }

    // Increment state counter if necessary and log new metrics.
    if (new_show_state == WindowMetricsEvent::SHOW_STATE_FULLSCREEN)
      current_metrics_.fullscreen_count++;
    else if (new_show_state == WindowMetricsEvent::SHOW_STATE_MAXIMIZED)
      current_metrics_.maximize_count++;
    else if (new_show_state == WindowMetricsEvent::SHOW_STATE_MINIMIZED)
      current_metrics_.minimize_count++;
    MaybeLogWindowMetricsUkmEntry();
  }

  // TabStripModelObserver:
  void TabInsertedAt(TabStripModel* tab_strip_model,
                     content::WebContents* contents,
                     int index,
                     bool foreground) override {
    MaybeLogWindowMetricsUkmEntry();
  }
  void TabDetachedAt(content::WebContents* contents, int index) override {
    MaybeLogWindowMetricsUkmEntry();
  }

  // The browser whose tab strip we track. WindowActivityWatcher should ensure
  // this outlives us.
  Browser* browser_;

  // The actual state of the window, using raw numbers (no bucketing).
  WindowMetrics current_metrics_;

  // The most recent WindowMetrics UKM entry logged, with some values bucketed
  // into ranges for privacy reasons.
  base::Optional<WindowMetrics> last_reported_metrics_;

  // Used to update tab count metrics for browser windows.
  ScopedObserver<TabStripModel, TabStripModelObserver>
      tab_strip_model_observer_;

  DISALLOW_COPY_AND_ASSIGN(BrowserWatcher);
};

// static
WindowActivityWatcher* WindowActivityWatcher::GetInstance() {
  CR_DEFINE_STATIC_LOCAL(WindowActivityWatcher, instance, ());
  return &instance;
}

WindowActivityWatcher::WindowActivityWatcher() {
  BrowserList::AddObserver(this);
  for (Browser* browser : *BrowserList::GetInstance())
    OnBrowserAdded(browser);
}

WindowActivityWatcher::~WindowActivityWatcher() {
  BrowserList::RemoveObserver(this);
}

bool WindowActivityWatcher::ShouldTrackBrowser(Browser* browser) {
  // Don't track incognito browsers. This is also enforced by UKM.
  return !browser->profile()->IsOffTheRecord();
}

void WindowActivityWatcher::OnBrowserAdded(Browser* browser) {
  if (ShouldTrackBrowser(browser))
    browser_watchers_[browser] = std::make_unique<BrowserWatcher>(browser);
}

void WindowActivityWatcher::OnBrowserRemoved(Browser* browser) {
  browser_watchers_.erase(browser);
}

void WindowActivityWatcher::OnBrowserSetLastActive(Browser* browser) {
  if (ShouldTrackBrowser(browser))
    browser_watchers_[browser]->MaybeLogWindowMetricsUkmEntry();
}

void WindowActivityWatcher::OnBrowserNoLongerActive(Browser* browser) {
  if (ShouldTrackBrowser(browser))
    browser_watchers_[browser]->MaybeLogWindowMetricsUkmEntry();
}
