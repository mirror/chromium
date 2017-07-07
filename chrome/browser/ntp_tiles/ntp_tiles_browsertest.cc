// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/history/top_sites_factory.h"
#include "chrome/browser/ntp_tiles/chrome_most_visited_sites_factory.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/history/core/browser/top_sites.h"
#include "components/history/core/browser/top_sites_observer.h"
#include "components/ntp_tiles/most_visited_sites.h"
#include "components/ntp_tiles/ntp_tile.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace ntp_tiles {

namespace {

using history::TopSites;

// Waits for most visited URLs to be made available, and verifies that it
// includes a tile for the specified URL.
class MostVisitedSitesWaiter : public MostVisitedSites::Observer {
 public:
  explicit MostVisitedSitesWaiter(const GURL& required_url)
      : required_url_(required_url) {}
  void Wait() {
    base::RunLoop run_loop;
    quit_closure_ = run_loop.QuitClosure();
    run_loop.Run();
  }

  void OnMostVisitedURLsAvailable(const NTPTilesVector& tiles) override {
    const GURL& url = required_url_;
    auto it =
        std::find_if(tiles.begin(), tiles.end(),
                     [&url](const NTPTile& tile) { return tile.url == url; });
    if (!quit_closure_.is_null() && (it != tiles.end())) {
      quit_closure_.Run();
    }
  }
  void OnIconMadeAvailable(const GURL& site_url) override {}

 private:
  base::Closure quit_closure_;
  const GURL& required_url_;
};
}  // namespace

class NTPTilesTest : public InProcessBrowserTest {
 public:
  NTPTilesTest() {}

 protected:
  void SetUpOnMainThread() override {
    top_sites_ = TopSitesFactory::GetForProfile(browser()->profile());
    most_visited_sites_ =
        ChromeMostVisitedSitesFactory::NewForProfile(browser()->profile());
  }

  void TearDownOnMainThread() override {
    // Reset most_visited_sites_, otherwise there is a CHECK in callback_list.h
    // because callbacks_.size() is not 0.
    most_visited_sites_.reset();
  }

  std::unique_ptr<ntp_tiles::MostVisitedSites> most_visited_sites_;
  scoped_refptr<TopSites> top_sites_;

 private:
  DISALLOW_COPY_AND_ASSIGN(NTPTilesTest);
};

// Tests that after navigating to URL, ntp tiles will include the URL.
IN_PROC_BROWSER_TEST_F(NTPTilesTest, LoadURL) {
  ASSERT_TRUE(embedded_test_server()->Start());
  const GURL url = embedded_test_server()->GetURL("/simple.html");

  ui_test_utils::NavigateToURLWithDisposition(
      browser(), url, WindowOpenDisposition::CURRENT_TAB,
      ui_test_utils::BROWSER_TEST_WAIT_FOR_NAVIGATION);

  // Explicitly sync history, so the test doesn't have to wait for it to
  // happen automatically.
  top_sites_->SyncWithHistory();
  most_visited_sites_->Refresh();

  MostVisitedSitesWaiter waiter(url);
  most_visited_sites_->SetMostVisitedURLsObserver(&waiter,
                                                  /*num_sites=*/8);
  waiter.Wait();
}

}  // namespace ntp_tiles
