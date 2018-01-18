// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/tabs/window_activity_watcher.h"

#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/browser_list.h"
#include "chrome/browser/ui/browser_window.h"
#include "chrome/browser/ui/tabs/tab_metrics_event.pb.h"
#include "services/metrics/public/cpp/ukm_builders.h"
#include "services/metrics/public/cpp/ukm_recorder.h"
#include "services/metrics/public/cpp/ukm_source_id.h"

using WindowMetrics = WindowActivityWatcher::WindowMetrics;
using metrics::WindowMetricsEvent;

struct WindowActivityWatcher::WindowMetrics {
  bool operator==(const WindowMetrics& other) {
    return window_id == other.window_id && type == other.type &&
           show_state == other.show_state && is_active == other.is_active &&
           tab_count == other.tab_count;
  }

  bool operator!=(const WindowMetrics& other) { return !operator==(other); }

  // ID for the window, unique within the Chrome session.
  SessionID::id_type window_id;
  WindowMetricsEvent::Type type;
  // TODO(michaelpg): Observe the show state and log when it changes.
  WindowMetricsEvent::ShowState show_state;

  // True if this is the active (frontmost) window.
  bool is_active;

  // Number of tabs in the tab strip.
  int tab_count;
};

namespace {

// Sets metric values that are dependent on the current window state.
void UpdateMetrics(const Browser* browser, WindowMetrics* window_metrics) {
  if (browser->window()->IsFullscreen())
    window_metrics->show_state = WindowMetricsEvent::SHOW_STATE_FULLSCREEN;
  else if (browser->window()->IsMinimized())
    window_metrics->show_state = WindowMetricsEvent::SHOW_STATE_MINIMIZED;
  else if (browser->window()->IsMaximized())
    window_metrics->show_state = WindowMetricsEvent::SHOW_STATE_MAXIMIZED;
  else
    window_metrics->show_state = WindowMetricsEvent::SHOW_STATE_NORMAL;

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

// Logs a UKM entry with the corresponding metrics.
void LogUkmEntry(const WindowMetrics& window_metrics) {
  ukm::UkmRecorder* ukm_recorder = ukm::UkmRecorder::Get();
  if (!ukm_recorder)
    return;

  ukm::builders::TabManager_WindowMetrics entry(ukm::AssignNewSourceId());
  entry.SetWindowId(window_metrics.window_id)
      .SetIsActive(window_metrics.is_active)
      .SetShowState(window_metrics.show_state)
      .SetTabCount(window_metrics.tab_count)
      .SetType(window_metrics.type)
      .Record(ukm_recorder);
}

}  // namespace

// static
WindowActivityWatcher* WindowActivityWatcher::GetInstance() {
  CR_DEFINE_STATIC_LOCAL(WindowActivityWatcher, instance, ());
  return &instance;
}

void WindowActivityWatcher::CreateOrUpdateWindowMetrics(
    const Browser* browser) {
  DCHECK(!browser->profile()->IsOffTheRecord());
  // Ignore browser windows with no tabs (which can happen when a window is
  // opened, before a tab is added).
  if (browser->tab_strip_model()->empty())
    return;
  SessionID::id_type window_id = browser->session_id().id();
  const auto& existing_metrics_it = window_metrics_.find(window_id);
  if (existing_metrics_it == window_metrics_.end()) {
    window_metrics_[window_id] = CreateMetrics(browser);
    LogUkmEntry(window_metrics_[window_id]);
    return;
  }

  // Copy old state to compare with.
  WindowMetrics old_metrics = existing_metrics_it->second;
  UpdateMetrics(browser, &existing_metrics_it->second);
  // We only need to create a new UKM entry if the metrics have changed.
  if (old_metrics != existing_metrics_it->second)
    LogUkmEntry(existing_metrics_it->second);
}

WindowActivityWatcher::WindowActivityWatcher()
    : browser_tab_strip_tracker_(this, this, this) {
  browser_tab_strip_tracker_.Init();
}

WindowActivityWatcher::~WindowActivityWatcher() = default;

void WindowActivityWatcher::OnBrowserAdded(Browser* browser) {
  CreateOrUpdateWindowMetrics(browser);
}

void WindowActivityWatcher::OnBrowserRemoved(Browser* browser) {
  window_metrics_.erase(browser->session_id().id());
}

void WindowActivityWatcher::OnBrowserSetLastActive(Browser* browser) {
  CreateOrUpdateWindowMetrics(browser);
}

void WindowActivityWatcher::OnBrowserNoLongerActive(Browser* browser) {
  CreateOrUpdateWindowMetrics(browser);
}

void WindowActivityWatcher::TabInsertedAt(TabStripModel* tab_strip_model,
                                          content::WebContents* contents,
                                          int index,
                                          bool foreground) {
  Browser* browser = chrome::FindBrowserWithWebContents(contents);
  DCHECK(browser);
  CreateOrUpdateWindowMetrics(browser);
}

void WindowActivityWatcher::TabDetachedAt(content::WebContents* contents,
                                          int index) {
  Browser* browser = chrome::FindBrowserWithWebContents(contents);
  // The browser may have been closed.
  if (browser)
    CreateOrUpdateWindowMetrics(browser);
}

bool WindowActivityWatcher::ShouldTrackBrowser(Browser* browser) {
  // Don't track incognito browsers. This is also enforced by UKM.
  return !browser->profile()->IsOffTheRecord();
}
