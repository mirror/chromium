// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/command_line.h"
#include "components/ukm/content/source_url_recorder_web_contents_observer.h"
#include "components/ukm/test_ukm_recorder.h"
#include "components/ukm/ukm_source.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/content_switches.h"
#include "content/public/test/content_browser_test.h"
#include "content/public/test/content_browser_test_utils.h"
#include "content/public/test/download_test_observer.h"
#include "content/public/test/navigation_handle_observer.h"
#include "content/shell/browser/shell.h"
#include "content/shell/browser/shell_browser_context.h"

class SourceUrlRecorderWebContentsObserverBrowserTest
    : public content::ContentBrowserTest {
 protected:
  void SetUpOnMainThread() override {
    content::ContentBrowserTest::SetUpOnMainThread();

    ASSERT_TRUE(embedded_test_server()->Start());

    // UKM DCHECKs if the active UkmRecorder is changed from one instance
    // to another, rather than being changed from a nullptr; browser_tests
    // need to circumvent that to be able to intercept UKM calls with its
    // own TestUkmRecorder instance rather than the default UkmRecorder.
    ukm::UkmRecorder::Set(nullptr);
    test_ukm_recorder_ = base::MakeUnique<ukm::TestAutoSetUkmRecorder>();

    ukm::InitializeSourceUrlRecorderForWebContents(shell()->web_contents());
  }

  void SetUpCommandLine(base::CommandLine* command_line) override {
    content::ContentBrowserTest::SetUpCommandLine(command_line);
    command_line->AppendSwitchASCII(switches::kEnableFeatures,
                                    ukm::kUkmFeature.name);
  }

  const ukm::UkmSource* GetSourceForNavigationId(int64_t navigation_id) {
    CHECK_GT(navigation_id, 0);
    const ukm::SourceId source_id =
        ukm::ConvertToSourceId(navigation_id, ukm::SourceIdType::NAVIGATION_ID);
    return test_ukm_recorder_->GetSourceForSourceId(source_id);
  }

  std::unique_ptr<ukm::TestAutoSetUkmRecorder> test_ukm_recorder_;
};

IN_PROC_BROWSER_TEST_F(SourceUrlRecorderWebContentsObserverBrowserTest, Basic) {
  GURL url = embedded_test_server()->GetURL("/title1.html");
  content::NavigationHandleObserver observer(shell()->web_contents(), url);
  content::NavigateToURL(shell(), url);
  EXPECT_TRUE(observer.has_committed());
  const ukm::UkmSource* source =
      GetSourceForNavigationId(observer.navigation_id());
  EXPECT_NE(nullptr, source);
  EXPECT_EQ(url, source->url());
  EXPECT_TRUE(source->initial_url().is_empty());
}

IN_PROC_BROWSER_TEST_F(SourceUrlRecorderWebContentsObserverBrowserTest,
                       IgnoreUnsupportedScheme) {
  GURL url("about:blank");
  content::NavigationHandleObserver observer(shell()->web_contents(), url);
  content::NavigateToURL(shell(), url);
  EXPECT_TRUE(observer.has_committed());
  EXPECT_EQ(nullptr, GetSourceForNavigationId(observer.navigation_id()));
}

IN_PROC_BROWSER_TEST_F(SourceUrlRecorderWebContentsObserverBrowserTest,
                       IgnoreUrlInSubframe) {
  GURL main_url = embedded_test_server()->GetURL("/page_with_iframe.html");
  GURL subframe_url = embedded_test_server()->GetURL("/title1.html");

  content::NavigationHandleObserver main_observer(shell()->web_contents(),
                                                  main_url);
  content::NavigationHandleObserver subframe_observer(shell()->web_contents(),
                                                      subframe_url);
  content::NavigateToURL(shell(), main_url);
  EXPECT_TRUE(main_observer.has_committed());
  EXPECT_TRUE(main_observer.is_main_frame());
  EXPECT_FALSE(subframe_observer.is_main_frame());

  const ukm::UkmSource* source =
      GetSourceForNavigationId(main_observer.navigation_id());
  EXPECT_NE(nullptr, source);
  EXPECT_EQ(main_url, source->url());
  EXPECT_EQ(nullptr,
            GetSourceForNavigationId(subframe_observer.navigation_id()));
}

IN_PROC_BROWSER_TEST_F(SourceUrlRecorderWebContentsObserverBrowserTest,
                       IgnoreDownload) {
  content::DownloadTestObserverTerminal downloads_observer(
      content::BrowserContext::GetDownloadManager(
          shell()->web_contents()->GetBrowserContext()),
      1,  // == wait_count (only waiting for "download-test3.gif").
      content::DownloadTestObserver::ON_DANGEROUS_DOWNLOAD_FAIL);

  GURL url(embedded_test_server()->GetURL("/download-test1.lib"));
  content::NavigationHandleObserver observer(shell()->web_contents(), url);
  content::NavigateToURL(shell(), url);
  downloads_observer.WaitForFinished();
  EXPECT_FALSE(observer.has_committed());
  EXPECT_EQ(nullptr, GetSourceForNavigationId(observer.navigation_id()));
}
