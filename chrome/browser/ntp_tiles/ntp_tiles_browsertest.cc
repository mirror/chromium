// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/history/top_sites_factory.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/history/core/browser/top_sites.h"
#include "components/history/core/browser/top_sites_observer.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace ntp_tiles {

namespace {

using history::TopSites;

// Used to wait for top sites to be changed.
class TopSitesChangedWaiter : public history::TopSitesObserver {
 public:
  explicit TopSitesChangedWaiter(scoped_refptr<TopSites> top_sites)
      : top_sites_(top_sites) {
    if (top_sites_) {
      top_sites_->AddObserver(this);
    }
  }

  ~TopSitesChangedWaiter() override {
    if (top_sites_) {
      top_sites_->RemoveObserver(this);
    }
  }

  // Waits until a change to top sites is observed, when TopSitesChanged() is
  // called.
  void WaitForTopSitesChanged() {
    if (top_sites_) {
      run_loop_.Run();
    }
  }

 private:
  void TopSitesLoaded(TopSites* top_sites) override {}

  void TopSitesChanged(TopSites* top_sites,
                       ChangeReason change_reason) override {
    run_loop_.Quit();
  }

  scoped_refptr<TopSites> top_sites_;
  base::RunLoop run_loop_;

  DISALLOW_COPY_AND_ASSIGN(TopSitesChangedWaiter);
};

}  // namespace

class NTPTilesTest : public InProcessBrowserTest {
 public:
  NTPTilesTest() {}
  void SetUpOnMainThread() override {
    top_sites_ = TopSitesFactory::GetForProfile(browser()->profile());
  }

  ~NTPTilesTest() override {}

  TopSites* top_sites() { return top_sites_.get(); }

 private:
  scoped_refptr<TopSites> top_sites_;
  DISALLOW_COPY_AND_ASSIGN(NTPTilesTest);
};

// Test that loading a page creates an entry in top sites.
IN_PROC_BROWSER_TEST_F(NTPTilesTest, LoadURL) {
  ASSERT_TRUE(embedded_test_server()->Start());
  GURL url = embedded_test_server()->GetURL("/simple.html");

  TopSitesChangedWaiter waiter(top_sites());
  EXPECT_FALSE(top_sites()->IsKnownURL(url));

  ui_test_utils::NavigateToURL(browser(), url);

  // Explicitly sync history, so the test doesn't have to wait for it to
  // happen automatically.
  top_sites()->SyncWithHistory();

  waiter.WaitForTopSitesChanged();
  EXPECT_TRUE(top_sites()->IsKnownURL(url));
}

}  // namespace ntp_tiles
