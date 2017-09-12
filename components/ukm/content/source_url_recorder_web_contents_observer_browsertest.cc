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
#include "content/shell/browser/shell.h"
#include "content/shell/browser/shell_browser_context.h"

class SourceUrlRecorderWebContentsObserverBrowserTest
    : public content::ContentBrowserTest {
 protected:
  void PreRunTestOnMainThread() override {
    content::ContentBrowserTest::PreRunTestOnMainThread();

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

  std::unique_ptr<ukm::TestAutoSetUkmRecorder> test_ukm_recorder_;
};

IN_PROC_BROWSER_TEST_F(SourceUrlRecorderWebContentsObserverBrowserTest, Basic) {
  GURL url = embedded_test_server()->GetURL("/title1.html");
  content::NavigateToURL(shell(), url);

  EXPECT_EQ(1ul, test_ukm_recorder_->sources_count());
  const ukm::UkmSource* source = test_ukm_recorder_->GetSourceForUrl(url);
  EXPECT_NE(nullptr, source);
  EXPECT_EQ(url, source->url());
}

IN_PROC_BROWSER_TEST_F(SourceUrlRecorderWebContentsObserverBrowserTest,
                       IgnoreUnsupportedScheme) {
  content::NavigateToURL(shell(), GURL("about:blank"));
  EXPECT_EQ(0ul, test_ukm_recorder_->sources_count());
}

IN_PROC_BROWSER_TEST_F(SourceUrlRecorderWebContentsObserverBrowserTest,
                       IgnoreUrlInSubframe) {
  GURL url = embedded_test_server()->GetURL("/iframe.html");
  content::NavigateToURL(shell(), url);

  EXPECT_EQ(1ul, test_ukm_recorder_->sources_count());
  const ukm::UkmSource* source = test_ukm_recorder_->GetSourceForUrl(url);
  EXPECT_NE(nullptr, source);
  EXPECT_EQ(url, source->url());
}

IN_PROC_BROWSER_TEST_F(SourceUrlRecorderWebContentsObserverBrowserTest,
                       IgnoreDownload) {
  content::DownloadTestObserverTerminal downloads_observer(
      content::BrowserContext::GetDownloadManager(
          shell()->web_contents()->GetBrowserContext()),
      1,  // == wait_count (only waiting for "download-test3.gif").
      content::DownloadTestObserver::ON_DANGEROUS_DOWNLOAD_FAIL);

  content::NavigateToURL(shell(),
                         embedded_test_server()->GetURL("/download-test1.lib"));
  downloads_observer.WaitForFinished();

  EXPECT_EQ(0ul, test_ukm_recorder_->sources_count());
}
