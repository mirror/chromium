// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_PUBLIC_TEST_TEST_NAVIGATION_FINISHED_OBSERVER_H_
#define CONTENT_PUBLIC_TEST_TEST_NAVIGATION_FINISHED_OBSERVER_H_

#include <memory>
#include <set>

#include "base/callback.h"
#include "base/callback_forward.h"
#include "base/macros.h"
#include "url/gurl.h"

namespace content {

class NavigationHandle;
class WebContents;

// For browser_tests, which run on the UI thread, run a nested RunLoop and quit
// when the navigation finishes for a URL in any WebContents.
class TestNavigationFinishedObserver {
 public:
  // Create and register a new TestNavigationFinishedObserver for |target_url|.
  explicit TestNavigationFinishedObserver(const GURL& target_url);

  ~TestNavigationFinishedObserver();

  // Runs a nested RunLoop and blocks until a navigation to |target_url_| has
  // finished.
  void Wait();

  bool navigation_succeeded() const { return navigation_succeeded_; }

 private:
  class TestWebContentsObserver;

  void OnDidFinishNavigation(NavigationHandle* navigation_handle);

  void OnWebContentsCreated(WebContents* web_contents);

  void OnWebContentsDestroyed(TestWebContentsObserver* observer);

  void RegisterObserver(WebContents* web_contents);

  // URL this observer will wait for.
  GURL target_url_;

  // True if a navigation to |target_url_| has finished.
  bool navigation_finished_;

  // True if a navigation to |target_url_| has finished and it was successful.
  bool navigation_succeeded_;

  // Closure to quit the nested run loop. Runs once a navigation to
  // |target_url_| has finished.
  base::OnceClosure quit_closure_;

  // Callback invoked when WebContents are created.
  base::RepeatingCallback<void(WebContents*)> web_contents_created_callback_;

  // Active TestWebContentsObserver.
  std::set<std::unique_ptr<TestWebContentsObserver>> web_contents_observers_;

  DISALLOW_COPY_AND_ASSIGN(TestNavigationFinishedObserver);
};

}  // namespace content

#endif  // CONTENT_PUBLIC_TEST_TEST_NAVIGATION_FINISHED_OBSERVER_H_
