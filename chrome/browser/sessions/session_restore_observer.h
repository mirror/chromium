// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_SESSIONS_SESSION_RESTORE_OBSERVER_H_
#define CHROME_BROWSER_SESSIONS_SESSION_RESTORE_OBSERVER_H_

#include <stddef.h>

namespace content {
class WebContents;
}

// Observer of events during session restore. This observer does not cover
// SessionRestoreImpl::RestoreForeignTab() which restores a single foreign tab.
class SessionRestoreObserver {
 public:
  // OnSessionRestoreStartedLoadingTabs() is called from session restore
  // prior to creating the first tab from session restore. Session restore may
  // do processing before this, and if no tabs are created (there was no
  // previous session, or perhaps the data was corrupt) this is not called.
  // OnSessionRestoreStartedLoadingTabs() is *not* called if another session
  // restore is triggered while waiting for a load to complete.
  virtual void OnSessionRestoreStartedLoadingTabs() {}

  // OnSessionRestoreFinishedLoadingTabs() is called once all the tabs created
  // by session restore have completed loading (or loading is canceled because
  // of memory pressure). This is called on the last session restore when
  // multiple concurrent session restores (on all profiles) occur.
  virtual void OnSessionRestoreFinishedLoadingTabs() {}

  // OnWillRestoreTab() is called right after a tab is created by session
  // restore.
  virtual void OnWillRestoreTab(content::WebContents* web_contents) {}

  // OnWillRestoreNumOfTabs() notifies observers the number of tabs that will be
  // restored. It is called after all restored tabs are created, but before
  // actually loading most of them. The exact sequence is:
  // 1. OnSessionRestoreStartedLoadingTabs()
  // 2. Create the 1st restored tab, which is in the foreground. Showing it will
  //    kick off its navigation and loading.
  // 3. Create all other restored tabs, which are backgrounded.
  // 4. OnWillRestoreNumOfTabs(num_of_tabs)
  // 5. Schedule loading the rest of restored tabs.
  virtual void OnWillRestoreNumOfTabs(size_t num_of_tabs) {}
};

#endif  // CHROME_BROWSER_SESSIONS_SESSION_RESTORE_OBSERVER_H_
