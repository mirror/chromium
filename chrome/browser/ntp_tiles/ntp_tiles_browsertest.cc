// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
#include "chrome/browser/ntp_tiles/chrome_most_visited_sites_factory.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/ntp_tiles/most_visited_sites.h"
#include "components/ntp_tiles/ntp_tile.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace ntp_tiles {

namespace {

// Waits for most visited URLs to be made available.
class MostVisitedSitesWaiter : public MostVisitedSites::Observer {
 public:
  MostVisitedSitesWaiter() : tiles_(NTPTilesVector()) {}

  // Waits until most visited URLs are available, and then returns all the
  // tiles that have been seen since the object was created.
  NTPTilesVector WaitForTiles() {
    base::RunLoop run_loop;
    quit_closure_ = run_loop.QuitClosure();
    run_loop.Run();
    return tiles_;
  }

  void OnMostVisitedURLsAvailable(const NTPTilesVector& tiles) override {
    tiles_.insert(std::end(tiles_), std::begin(tiles), std::end(tiles));
    if (!quit_closure_.is_null()) {
      quit_closure_.Run();
    }
  }

  void OnIconMadeAvailable(const GURL& site_url) override {}

 private:
  base::Closure quit_closure_;
  NTPTilesVector tiles_;
};

}  // namespace

class NTPTilesTest : public InProcessBrowserTest {
 public:
  NTPTilesTest() {}

 protected:
  void SetUpOnMainThread() override {
    most_visited_sites_ =
        ChromeMostVisitedSitesFactory::NewForProfile(browser()->profile());
  }

  void TearDownOnMainThread() override {
    // Reset most_visited_sites_, otherwise there is a CHECK in callback_list.h
    // because callbacks_.size() is not 0.
    most_visited_sites_.reset();
  }

  std::unique_ptr<ntp_tiles::MostVisitedSites> most_visited_sites_;

 private:
  DISALLOW_COPY_AND_ASSIGN(NTPTilesTest);
};

// Tests that after navigating to a URL, ntp tiles will include the URL.
IN_PROC_BROWSER_TEST_F(NTPTilesTest, LoadURL) {
  ASSERT_TRUE(embedded_test_server()->Start());
  const GURL url = embedded_test_server()->GetURL("/simple.html");

  ui_test_utils::NavigateToURLWithDisposition(
      browser(), url, WindowOpenDisposition::CURRENT_TAB,
      ui_test_utils::BROWSER_TEST_WAIT_FOR_NAVIGATION);

  MostVisitedSitesWaiter waiter;
  most_visited_sites_->SetMostVisitedURLsObserver(&waiter, /*num_sites=*/8);

  NTPTilesVector tiles = waiter.WaitForTiles();
  auto it =
      std::find_if(tiles.begin(), tiles.end(),
                   [&url](const NTPTile& tile) { return tile.url == url; });
  EXPECT_TRUE(it != tiles.end());
}

}  // namespace ntp_tiles
