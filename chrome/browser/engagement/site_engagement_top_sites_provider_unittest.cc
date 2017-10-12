// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/engagement/site_engagement_top_sites_provider.h"

#include <vector>

#include "base/bind.h"
#include "base/callback.h"
#include "base/files/scoped_temp_dir.h"
#include "base/run_loop.h"
#include "base/strings/utf_string_conversions.h"
#include "base/task/cancelable_task_tracker.h"
#include "chrome/browser/engagement/site_engagement_score.h"
#include "chrome/browser/engagement/site_engagement_service.h"
#include "chrome/test/base/chrome_render_view_host_test_harness.h"
#include "chrome/test/base/testing_profile.h"
#include "components/history/core/browser/history_client.h"
#include "components/history/core/browser/history_database_params.h"
#include "components/history/core/browser/visit_delegate.h"
#include "components/history/core/test/test_history_database.h"
#include "ui/base/page_transition_types.h"
#include "url/gurl.h"

namespace {

// A class that implements a QueryMostVisitedURLsCallback, and verifies the
// results of the callback call against a provided set of inputs.
class CallbackTester {
 public:
  CallbackTester(const base::Closure& quit_closure,
                 const std::vector<GURL>& expected_urls,
                 const std::vector<base::string16>& expected_titles,
                 const std::vector<history::RedirectList>& expected_redirects)
      : quit_closure_(quit_closure),
        expected_urls_(expected_urls),
        expected_titles_(expected_titles),
        expected_redirects_(expected_redirects) {}

  base::CancelableTaskTracker* cancelable_task_tracker() {
    return &cancelable_task_tracker_;
  }

  void OnProvideTopSitesComplete(const history::MostVisitedURLList* urls) {
    EXPECT_EQ(expected_urls_.size(), urls->size());

    if (!expected_urls_.empty()) {
      for (size_t i = 0; i < expected_urls_.size(); ++i) {
        EXPECT_EQ(expected_urls_[i], urls->at(i).url);

        if (!expected_titles_.empty())
          EXPECT_EQ(expected_titles_[i], urls->at(i).title);

        if (!expected_redirects_.empty())
          EXPECT_EQ(expected_redirects_[i], urls->at(i).redirects);
      }
    }

    quit_closure_.Run();
  }

 private:
  base::Closure quit_closure_;
  std::vector<GURL> expected_urls_;
  std::vector<base::string16> expected_titles_;
  std::vector<history::RedirectList> expected_redirects_;
  base::CancelableTaskTracker cancelable_task_tracker_;
};

}  // namespace

class SiteEngagementTopSitesProviderTest
    : public ChromeRenderViewHostTestHarness {
 public:
  void SetUp() override {
    ChromeRenderViewHostTestHarness::SetUp();
    CreateHistoryService();
    SiteEngagementScore::SetParamValuesForTesting();
  }

  void TearDown() override {
    ChromeRenderViewHostTestHarness::TearDown();
    history_service_->Shutdown();
    history_service_.reset();
  }

 protected:
  void CreateHistoryService() {
    ASSERT_TRUE(scoped_temp_dir_.CreateUniqueTempDir());
    history_service_.reset(new history::HistoryService(
        nullptr, std::unique_ptr<history::VisitDelegate>()));
    ASSERT_TRUE(history_service_->Init(
        history::TestHistoryDatabaseParamsForPath(scoped_temp_dir_.GetPath())));
  }

  void AddPageToHistory(const GURL& url,
                        const base::string16& title,
                        const history::RedirectList& redirects,
                        base::Time time) {
    history_service_->AddPage(
        url, time, reinterpret_cast<history::ContextID>(1), 0, GURL(),
        redirects, ui::PAGE_TRANSITION_TYPED, history::SOURCE_BROWSED, false);
    history_service_->SetPageTitle(url, title);
  }

  history::HistoryService* history_service() { return history_service_.get(); }

  SiteEngagementService* GetEngagementService() {
    return SiteEngagementService::Get(profile());
  }

  std::unique_ptr<SiteEngagementTopSitesProvider> GetProvider() {
    return std::make_unique<SiteEngagementTopSitesProvider>(
        GetEngagementService(), history_service());
  }

  base::ScopedTempDir scoped_temp_dir_;
  std::unique_ptr<history::HistoryService> history_service_;
};

TEST_F(SiteEngagementTopSitesProviderTest, EmptyList) {
  // Ensure that the callback is run if the site engagement service provides no
  // URLs.
  std::unique_ptr<SiteEngagementTopSitesProvider> provider = GetProvider();

  base::RunLoop run_loop;
  CallbackTester tester(run_loop.QuitClosure(), {}, {}, {});

  provider->ProvideTopSites(
      10,
      base::Bind(&CallbackTester::OnProvideTopSitesComplete,
                 base::Unretained(&tester)),
      tester.cancelable_task_tracker());

  run_loop.Run();
}

TEST_F(SiteEngagementTopSitesProviderTest, NoDataInHistory) {
  // Ensure that the callback is run if the history service is empty.
  GURL url("https://www.google.com");

  SiteEngagementService* service = GetEngagementService();
  service->ResetBaseScoreForURL(url, 3);
  std::unique_ptr<SiteEngagementTopSitesProvider> provider = GetProvider();

  // Empty title and the URL itself as the sole redirect are expected.
  base::RunLoop run_loop;
  CallbackTester tester(run_loop.QuitClosure(), {url}, {base::string16()},
                        {history::RedirectList{url}});

  provider->ProvideTopSites(
      1,
      base::Bind(&CallbackTester::OnProvideTopSitesComplete,
                 base::Unretained(&tester)),
      tester.cancelable_task_tracker());

  run_loop.Run();
}

TEST_F(SiteEngagementTopSitesProviderTest, TitleAndRedirectsInHistory) {
  // Test a basic case with both titles and redirect lists.
  GURL url1("https://www.google.com.au");
  GURL url2("https://www.google.com");
  GURL url3("https://www.google.blog");

  SiteEngagementService* service = GetEngagementService();
  service->ResetBaseScoreForURL(url1, 4);
  AddPageToHistory(url1, base::UTF8ToUTF16("Title"), history::RedirectList(),
                   base::Time::Now());

  // Set up redirects from url1 and url2 to url3, making the expected redirect
  // list url2, url3, url1 and the expected title that of url3.
  AddPageToHistory(url3, base::UTF8ToUTF16("Another title"),
                   history::RedirectList{url1, url2, url3}, base::Time::Now());

  std::unique_ptr<SiteEngagementTopSitesProvider> provider = GetProvider();

  base::RunLoop run_loop;
  CallbackTester tester(run_loop.QuitClosure(), {url1},
                        {base::UTF8ToUTF16("Another title")},
                        {history::RedirectList{url2, url3, url1}});

  provider->ProvideTopSites(
      10,
      base::Bind(&CallbackTester::OnProvideTopSitesComplete,
                 base::Unretained(&tester)),
      tester.cancelable_task_tracker());

  run_loop.Run();
}

TEST_F(SiteEngagementTopSitesProviderTest, TitleOnlyInHistory) {
  // Test when the history service returns empty redirect lists.
  GURL url("https://www.google.com");

  SiteEngagementService* service = GetEngagementService();
  service->ResetBaseScoreForURL(url, 1);
  AddPageToHistory(url, base::UTF8ToUTF16("Title"), history::RedirectList(),
                   base::Time::Now());

  std::unique_ptr<SiteEngagementTopSitesProvider> provider = GetProvider();

  base::RunLoop run_loop;
  CallbackTester tester(run_loop.QuitClosure(), {url},
                        {base::UTF8ToUTF16("Title")},
                        {history::RedirectList{url}});

  provider->ProvideTopSites(
      1,
      base::Bind(&CallbackTester::OnProvideTopSitesComplete,
                 base::Unretained(&tester)),
      tester.cancelable_task_tracker());

  run_loop.Run();
}

TEST_F(SiteEngagementTopSitesProviderTest, RedirectsOnlyInHistory) {
  // Test when the history service returns empty titles.
  GURL url1("https://www.google.com.au");
  GURL url2("https://www.google.com");
  GURL url3("https://www.google.blog");

  SiteEngagementService* service = GetEngagementService();
  service->ResetBaseScoreForURL(url1, 0.5);
  AddPageToHistory(url1, base::string16(), history::RedirectList{},
                   base::Time::Now());
  AddPageToHistory(url2, base::string16(),
                   history::RedirectList{url1, url3, url2}, base::Time::Now());

  std::unique_ptr<SiteEngagementTopSitesProvider> provider = GetProvider();

  base::RunLoop run_loop;
  CallbackTester tester(run_loop.QuitClosure(), {url1}, {base::string16()},
                        {history::RedirectList{url3, url2, url1}});

  provider->ProvideTopSites(
      8,
      base::Bind(&CallbackTester::OnProvideTopSitesComplete,
                 base::Unretained(&tester)),
      tester.cancelable_task_tracker());

  run_loop.Run();
}

TEST_F(SiteEngagementTopSitesProviderTest, MaxRequestedIsRespected) {
  // Test that the maximum requested number of sites is respected.
  GURL url1("https://www.google.com.au");
  GURL url2("https://www.google.com");
  GURL url3("https://www.google.blog");
  GURL url4("https://www.google.org");

  SiteEngagementService* service = GetEngagementService();
  service->ResetBaseScoreForURL(url1, 2);
  service->ResetBaseScoreForURL(url2, 1);
  service->ResetBaseScoreForURL(url3, 4);
  service->ResetBaseScoreForURL(url4, 0.5);
  AddPageToHistory(url1, base::UTF8ToUTF16("Title"), history::RedirectList(),
                   base::Time::Now());

  AddPageToHistory(url2, base::UTF8ToUTF16("Other title"),
                   history::RedirectList(), base::Time::Now());

  AddPageToHistory(url3, base::UTF8ToUTF16("Another title"),
                   history::RedirectList(), base::Time::Now());

  auto provider = std::make_unique<SiteEngagementTopSitesProvider>(
      service, history_service());

  base::RunLoop run_loop;
  CallbackTester tester(
      run_loop.QuitClosure(), {url3, url1},
      {base::UTF8ToUTF16("Another title"), base::UTF8ToUTF16("Title")},
      {history::RedirectList{url3}, history::RedirectList{url1}});

  provider->ProvideTopSites(
      2,
      base::Bind(&CallbackTester::OnProvideTopSitesComplete,
                 base::Unretained(&tester)),
      tester.cancelable_task_tracker());

  run_loop.Run();
}

TEST_F(SiteEngagementTopSitesProviderTest, OverlappingRedirects) {
  // Tests a case with a chain of redirects through url1 -> url2 -> url3. All
  // titles should match url3.
  GURL url1("https://www.google.com.au");
  GURL url2("https://www.google.com");
  GURL url3("https://www.google.blog");

  SiteEngagementService* service = GetEngagementService();
  service->ResetBaseScoreForURL(url1, 4);
  service->ResetBaseScoreForURL(url2, 1);
  service->ResetBaseScoreForURL(url3, 2);

  AddPageToHistory(url1, base::UTF8ToUTF16("Title"), history::RedirectList(),
                   base::Time::Now());

  AddPageToHistory(url2, base::UTF8ToUTF16("Other title"),
                   history::RedirectList(), base::Time::Now());

  AddPageToHistory(url3, base::UTF8ToUTF16("Another title"),
                   history::RedirectList{url1, url2, url3}, base::Time::Now());

  auto provider = std::make_unique<SiteEngagementTopSitesProvider>(
      service, history_service());

  base::RunLoop run_loop;
  CallbackTester tester(
      run_loop.QuitClosure(), {url1, url3, url2},
      {base::UTF8ToUTF16("Another title"), base::UTF8ToUTF16("Another title"),
       base::UTF8ToUTF16("Another title")},
      {history::RedirectList{url2, url3, url1}, history::RedirectList{url3},
       history::RedirectList{url3, url2}});

  provider->ProvideTopSites(
      3,
      base::Bind(&CallbackTester::OnProvideTopSitesComplete,
                 base::Unretained(&tester)),
      tester.cancelable_task_tracker());

  run_loop.Run();
}
