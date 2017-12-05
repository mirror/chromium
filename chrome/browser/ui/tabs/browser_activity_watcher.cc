// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/tabs/browser_activity_watcher.h"

#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_list.h"
#include "chrome/browser/ui/browser_window.h"
#include "chrome/browser/ui/tabs/tab_metrics_event.pb.h"
#include "services/metrics/public/cpp/ukm_builders.h"
#include "services/metrics/public/cpp/ukm_recorder.h"
#include "services/metrics/public/cpp/ukm_source_id.h"

using BrowserMetrics = BrowserActivityWatcher::BrowserMetrics;
using metrics::BrowserMetricsEvent;

struct BrowserActivityWatcher::BrowserMetrics {
  // ID for the browser, unique within the Chrome session.
  uint64_t browser_id;
  BrowserMetricsEvent::Type type;
  BrowserMetricsEvent::ShowState show_state;
};

namespace {

// Returns a populated BrowserMetrics for the browser.
BrowserMetrics CreateMetrics(const Browser* browser, uint64_t browser_id) {
  BrowserMetrics browser_metrics;
  browser_metrics.browser_id = browser_id;

  switch (browser->type()) {
    case Browser::TYPE_TABBED:
      browser_metrics.type = BrowserMetricsEvent::TYPE_TABBED;
      break;
    case Browser::TYPE_POPUP:
      browser_metrics.type = BrowserMetricsEvent::TYPE_POPUP;
      break;
  }

  if (browser->window()->IsFullscreen())
    browser_metrics.show_state = BrowserMetricsEvent::SHOW_STATE_FULLSCREEN;
  else if (browser->window()->IsMinimized())
    browser_metrics.show_state = BrowserMetricsEvent::SHOW_STATE_MINIMIZED;
  else if (browser->window()->IsMaximized())
    browser_metrics.show_state = BrowserMetricsEvent::SHOW_STATE_MAXIMIZED;
  else
    browser_metrics.show_state = BrowserMetricsEvent::SHOW_STATE_NORMAL;
  return browser_metrics;
}

// Returns a new ID for the browser which is unique for the duration of the
// browsing session.
uint64_t GetNextBrowserId() {
  static int id = 1;
  return id++;
}

// Logs a UKM entry with the corresponding metrics.
void LogUkmEntry(const BrowserMetrics& browser_metrics) {
  ukm::UkmRecorder* ukm_recorder = ukm::UkmRecorder::Get();
  if (!ukm_recorder)
    return;

  ukm::builders::TabManager_BrowserMetrics entry(ukm::AssignNewSourceId());
  entry.SetBrowserId(browser_metrics.browser_id)
      .SetShowState(browser_metrics.show_state)
      .SetType(browser_metrics.type)
      .Record(ukm_recorder);
}

// Comparisons for BrowserMetrics.
bool operator==(BrowserMetrics lhs, BrowserMetrics rhs) {
  return lhs.browser_id == rhs.browser_id && lhs.show_state == rhs.show_state &&
         lhs.type == rhs.type;
}

bool operator!=(BrowserMetrics lhs, BrowserMetrics rhs) {
  return !(lhs == rhs);
}

}  // namespace

// static
BrowserActivityWatcher* BrowserActivityWatcher::GetInstance() {
  CR_DEFINE_STATIC_LOCAL(BrowserActivityWatcher, instance, ());
  return &instance;
}

uint64_t BrowserActivityWatcher::CreateOrUpdateBrowserMetrics(
    const Browser* browser) {
  const auto& existing_metrics = browser_metrics_.find(browser);
  uint64_t browser_id = existing_metrics == browser_metrics_.end()
                            ? GetNextBrowserId()
                            : existing_metrics->second.browser_id;

  // We only need to create a new UKM entry if the metrics have changed or
  // this browser hasn't been logged yet.
  BrowserMetrics new_metrics = CreateMetrics(browser, browser_id);
  if (existing_metrics == browser_metrics_.end() ||
      existing_metrics->second != new_metrics) {
    LogUkmEntry(new_metrics);
    browser_metrics_[browser] = new_metrics;
  }

  return browser_id;
}

BrowserActivityWatcher::BrowserActivityWatcher()
    : browser_list_observer_(this) {
  browser_list_observer_.Add(BrowserList::GetInstance());
}

BrowserActivityWatcher::~BrowserActivityWatcher() = default;

void BrowserActivityWatcher::OnBrowserRemoved(Browser* browser) {
  browser_metrics_.erase(browser);
}

void BrowserActivityWatcher::OnBrowserSetLastActive(Browser* browser) {
  // We may need to update the metrics if a tab is already referencing this
  // browser's existing UKM entry.
  if (browser_metrics_.count(browser))
    CreateOrUpdateBrowserMetrics(browser);
}

void BrowserActivityWatcher::OnBrowserNoLongerActive(Browser* browser) {
  // We may need to update the metrics if a tab is already referencing this
  // browser's existing UKM entry.
  if (browser_metrics_.count(browser))
    CreateOrUpdateBrowserMetrics(browser);
}
