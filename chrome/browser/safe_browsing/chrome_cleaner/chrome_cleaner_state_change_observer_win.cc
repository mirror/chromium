// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/safe_browsing/chrome_cleaner/chrome_cleaner_state_change_observer_win.h"

#include "base/threading/thread_checker.h"
#include "content/public/browser/browser_thread.h"

using content::BrowserThread;

namespace safe_browsing {

ChromeCleanerStateChangeObserver::ChromeCleanerStateChangeObserver(
    const base::RepeatingClosure& on_state_change)
    : on_state_change_(on_state_change) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  ChromeCleanerController::GetInstance()->AddObserver(this);
}

ChromeCleanerStateChangeObserver::~ChromeCleanerStateChangeObserver() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  ChromeCleanerController::GetInstance()->RemoveObserver(this);
}

void ChromeCleanerStateChangeObserver::OnCleanupStateChange() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  on_state_change_.Run();
}

void ChromeCleanerStateChangeObserver::OnIdle(
    ChromeCleanerController::IdleReason idle_reason) {
  OnCleanupStateChange();
}

void ChromeCleanerStateChangeObserver::OnScanning() {
  OnCleanupStateChange();
}

void ChromeCleanerStateChangeObserver::OnInfected(
    const std::set<base::FilePath>& files) {
  OnCleanupStateChange();
}

void ChromeCleanerStateChangeObserver::OnCleaning(
    const std::set<base::FilePath>& files) {
  OnCleanupStateChange();
}

void ChromeCleanerStateChangeObserver::OnRebootRequired() {
  OnCleanupStateChange();
}

void ChromeCleanerStateChangeObserver::OnLogsEnabledChanged(bool logs_enabled) {
  OnCleanupStateChange();
}

}  // namespace safe_browsing
