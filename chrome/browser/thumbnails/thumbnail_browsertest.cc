// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <memory>

#include "base/command_line.h"
#include "base/memory/scoped_refptr.h"
#include "base/run_loop.h"
#include "base/test/scoped_feature_list.h"
#include "build/build_config.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/thumbnails/thumbnail_service.h"
#include "chrome/browser/thumbnails/thumbnail_service_factory.h"
#include "chrome/browser/thumbnails/thumbnailing_context.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/common/chrome_features.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

using testing::_;
using testing::AtMost;
using testing::DoAll;
using testing::Field;
using testing::InSequence;
using testing::Return;
using testing::StrictMock;

namespace thumbnails {

ACTION_P(RunClosure, closure) {
  closure.Run();
}

class MockThumbnailService : public ThumbnailService {
 public:
  MOCK_METHOD2(SetPageThumbnail,
               bool(const ThumbnailingContext& context,
                    const gfx::Image& thumbnail));

  MOCK_METHOD3(GetPageThumbnail,
               bool(const GURL& url,
                    bool prefix_match,
                    scoped_refptr<base::RefCountedMemory>* bytes));

  MOCK_METHOD1(AddForcedURL, void(const GURL& url));

  MOCK_METHOD2(ShouldAcquirePageThumbnail,
               bool(const GURL& url, ui::PageTransition transition));

  // Implementation of RefcountedKeyedService.
  void ShutdownOnUIThread() override {}

 protected:
  ~MockThumbnailService() override = default;
};

class ThumbnailTest : public InProcessBrowserTest {
 public:
  ThumbnailTest() : test_server_(net::EmbeddedTestServer::TYPE_HTTPS) {}

  MockThumbnailService* thumbnail_service() {
    return static_cast<MockThumbnailService*>(
        ThumbnailServiceFactory::GetForProfile(browser()->profile()).get());
  }

  GURL GetURL(const std::string& path) const {
    return test_server_.GetURL(path);
  }

 private:
  void SetUpInProcessBrowserTestFixture() override {
    feature_list_.InitAndEnableFeature(
        features::kCaptureThumbnailOnNavigatingAway);

    test_server_.ServeFilesFromSourceDirectory("chrome/test/data");
    ASSERT_TRUE(test_server_.Start());

    will_create_browser_context_services_subscription_ =
        BrowserContextDependencyManager::GetInstance()
            ->RegisterWillCreateBrowserContextServicesCallbackForTesting(
                base::Bind(&ThumbnailTest::OnWillCreateBrowserContextServices,
                           base::Unretained(this)));
  }

  static scoped_refptr<RefcountedKeyedService> CreateThumbnailService(
      content::BrowserContext* context) {
    return scoped_refptr<RefcountedKeyedService>(
        new StrictMock<MockThumbnailService>());
  }

  void OnWillCreateBrowserContextServices(content::BrowserContext* context) {
    ThumbnailServiceFactory::GetInstance()->SetTestingFactory(
        context, &ThumbnailTest::CreateThumbnailService);
  }

  base::test::ScopedFeatureList feature_list_;

  net::EmbeddedTestServer test_server_;

  std::unique_ptr<
      base::CallbackList<void(content::BrowserContext*)>::Subscription>
      will_create_browser_context_services_subscription_;
};

IN_PROC_BROWSER_TEST_F(ThumbnailTest, ShouldCaptureOnNavigatingAway) {
#if BUILDFLAG(ENABLE_PACKAGE_MASH_SERVICES)
  if (base::CommandLine::ForCurrentProcess()->HasSwitch(switches::kMus)) {
    LOG(ERROR) << "This test fails in MUS mode, aborting.";
    return;
  }
#endif

  InSequence sequence;

  // The test framework opens an about:blank tab by default.
  content::WebContents* active_tab =
      browser()->tab_strip_model()->GetActiveWebContents();
  ASSERT_EQ(GURL("about:blank"), active_tab->GetLastCommittedURL());

  // Navigate to some page. Before navigating away, we should check whether to
  // take a thumbnail of the old page (about:blank);
  EXPECT_CALL(*thumbnail_service(),
              ShouldAcquirePageThumbnail(GURL("about:blank"), _))
      .WillOnce(Return(false));
  ui_test_utils::NavigateToURL(browser(), GetURL("/simple.html"));

  // Before navigating away, we should take a thumbnail of the page.
  base::RunLoop run_loop;
  EXPECT_CALL(*thumbnail_service(),
              ShouldAcquirePageThumbnail(GetURL("/simple.html"), _))
      .WillOnce(Return(true));
  EXPECT_CALL(*thumbnail_service(),
              SetPageThumbnail(
                  Field(&ThumbnailingContext::url, GetURL("/simple.html")), _))
      .WillOnce(DoAll(RunClosure(run_loop.QuitClosure()), Return(true)));
  // Navigate to about:blank again.
  ui_test_utils::NavigateToURL(browser(), GURL("about:blank"));
  // Wait for the thumbnailing process to finish.
  run_loop.Run();

  // When the tab gets destroyed during shutdown, we may get another check
  // whether to take a thumbnail.
  EXPECT_CALL(*thumbnail_service(),
              ShouldAcquirePageThumbnail(GURL("about:blank"), _))
      .Times(AtMost(1))
      .WillOnce(Return(false));
}

IN_PROC_BROWSER_TEST_F(ThumbnailTest, ShouldCaptureOnSwitchingTab) {
#if BUILDFLAG(ENABLE_PACKAGE_MASH_SERVICES)
  if (base::CommandLine::ForCurrentProcess()->HasSwitch(switches::kMus)) {
    LOG(ERROR) << "This test fails in MUS mode, aborting.";
    return;
  }
#endif

  InSequence sequence;

  // The test framework opens an about:blank tab by default.
  content::WebContents* active_tab =
      browser()->tab_strip_model()->GetActiveWebContents();
  ASSERT_EQ(GURL("about:blank"), active_tab->GetLastCommittedURL());

  // Open a new tab. When the first tab gets hidden, we should check whether to
  // take a thumbnail.
  EXPECT_CALL(*thumbnail_service(),
              ShouldAcquirePageThumbnail(GURL("about:blank"), _))
      .WillOnce(Return(false));
  ui_test_utils::NavigateToURLWithDisposition(
      browser(), GetURL("/simple.html"),
      WindowOpenDisposition::NEW_FOREGROUND_TAB,
      ui_test_utils::BROWSER_TEST_WAIT_FOR_TAB |
          ui_test_utils::BROWSER_TEST_WAIT_FOR_NAVIGATION);

  active_tab = browser()->tab_strip_model()->GetActiveWebContents();

  // Before the second tab gets hidden, we should take a thumbnail of the page.
  base::RunLoop run_loop;
  EXPECT_CALL(*thumbnail_service(),
              ShouldAcquirePageThumbnail(GetURL("/simple.html"), _))
      .WillOnce(Return(true));
  EXPECT_CALL(*thumbnail_service(),
              SetPageThumbnail(
                  Field(&ThumbnailingContext::url, GetURL("/simple.html")), _))
      .WillOnce(DoAll(RunClosure(run_loop.QuitClosure()), Return(true)));
  // Switch back to the about:blank tab.
  ASSERT_EQ(1, browser()->tab_strip_model()->active_index());
  ASSERT_EQ(GetURL("/simple.html"), active_tab->GetLastCommittedURL());
  browser()->tab_strip_model()->ActivateTabAt(0, /*user_gesture=*/true);
  active_tab = browser()->tab_strip_model()->GetActiveWebContents();
  ASSERT_EQ(GURL("about:blank"), active_tab->GetLastCommittedURL());
  // Wait for the thumbnailing process to finish.
  run_loop.Run();

  // When the active tab gets destroyed during shutdown, we may get another
  // check whether to take a thumbnail.
  EXPECT_CALL(*thumbnail_service(),
              ShouldAcquirePageThumbnail(GURL("about:blank"), _))
      .Times(AtMost(1))
      .WillOnce(Return(false));
}

}  // namespace thumbnails
