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
// Defined for googletest. Must be defined in the same namespace.
void PrintTo(const NTPTile& tile, std::ostream* os) {
  *os << "{\"" << tile.title << "\", \"" << tile.url << "\", "
      << static_cast<int>(tile.source) << "}";
}

namespace {

using history::TopSites;
using testing::Contains;
using testing::StrictMock;

std::string PrintTile(const std::string& title,
                      const std::string& url,
                      TileSource source) {
  return std::string("has title \"") + title + std::string("\" and url \"") +
         url + std::string("\" and source ") +
         testing::PrintToString(static_cast<int>(source));
}

MATCHER_P3(MatchesTile, title, url, source, PrintTile(title, url, source)) {
  return arg.title == base::ASCIIToUTF16(title) && arg.url == GURL(url) &&
         arg.source == source;
}

class MockMostVisitedSitesObserver : public MostVisitedSites::Observer {
 public:
  MOCK_METHOD1(OnMostVisitedURLsAvailable, void(const NTPTilesVector& tiles));
  MOCK_METHOD1(OnIconMadeAvailable, void(const GURL& site_url));
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

  void TearDownOnMainThread() override { most_visited_sites_.reset(); }

  StrictMock<MockMostVisitedSitesObserver> mock_observer_;
  std::unique_ptr<ntp_tiles::MostVisitedSites> most_visited_sites_;
  scoped_refptr<TopSites> top_sites_;

 private:
  DISALLOW_COPY_AND_ASSIGN(NTPTilesTest);
};

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

  // HACK: Sleep so things can sync. Better to do something else:
  // 1. Is there a way to have the mock wait for the right call?
  // 2. Instead of a mock, create a custom observer that will wait for the
  //    correct results, or timeout.
  // 3. Is there some better callback API?
  base::RunLoop run_loop;
  base::ThreadTaskRunnerHandle::Get()->PostDelayedTask(
      FROM_HERE, run_loop.QuitClosure(),
      base::TimeDelta::FromMilliseconds(3000));
  base::MessageLoop::ScopedNestableTaskAllower allow_nested(
      base::MessageLoop::current());
  run_loop.Run();

  EXPECT_CALL(mock_observer_,
              OnMostVisitedURLsAvailable(Contains(MatchesTile(
                  "OK", url.spec().c_str(), TileSource::TOP_SITES))));

  most_visited_sites_->SetMostVisitedURLsObserver(&mock_observer_,
                                                  /*num_sites=*/1);

  base::RunLoop().RunUntilIdle();
}

}  // namespace ntp_tiles
