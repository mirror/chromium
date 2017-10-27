// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/metrics/out_of_memory_reporter.h"

#include <memory>

#include "base/macros.h"
#include "base/optional.h"
#include "base/process/kill.h"
#include "chrome/test/base/chrome_render_view_host_test_harness.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/mock_render_process_host.h"
#include "content/public/test/navigation_simulator.h"
#include "content/public/test/test_renderer_host.h"
#include "net/base/net_errors.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

class OutOfMemoryReporterTest : public ChromeRenderViewHostTestHarness,
                                public OutOfMemoryReporter::Observer {
 public:
  OutOfMemoryReporterTest() {}
  ~OutOfMemoryReporterTest() override {}

  // ChromeRenderViewHostTestHarness:
  void SetUp() override {
    ChromeRenderViewHostTestHarness::SetUp();
    OutOfMemoryReporter::CreateForWebContents(web_contents());
    OutOfMemoryReporter::FromWebContents(web_contents())->AddObserver(this);
  }

  // OutOfMemoryReporter::Observer:
  void OnForegroundOOMDetected(const GURL& url,
                               ukm::SourceId source_id) override {
    last_oom_url_ = url;
  }

 protected:
  base::Optional<GURL> last_oom_url_;

 private:
  DISALLOW_COPY_AND_ASSIGN(OutOfMemoryReporterTest);
};

TEST_F(OutOfMemoryReporterTest, SimpleOOM) {
  const GURL url("https://example.test/");
  NavigateAndCommit(url);

  process()->SimulateRenderProcessExit(base::TERMINATION_STATUS_OOM, 0);

  // Should get the notification synchronously.
  EXPECT_EQ(url, last_oom_url_.value());
}

TEST_F(OutOfMemoryReporterTest, NormalCrash_NoOOM) {
  const GURL url("https://example.test/");
  NavigateAndCommit(url);

  process()->SimulateRenderProcessExit(
      base::TERMINATION_STATUS_ABNORMAL_TERMINATION, 0);

  // Would get the notification synchronously.
  EXPECT_FALSE(last_oom_url_.has_value());
}

TEST_F(OutOfMemoryReporterTest, OOMOnPreviousPage) {
  const GURL url1("https://example.test1/");
  const GURL url2("https://example.test2/");
  const GURL url3("https://example.test2/");
  NavigateAndCommit(url1);
  NavigateAndCommit(url2);

  // Should not commit.
  content::NavigationSimulator::NavigateAndFailFromBrowser(web_contents(), url3,
                                                           net::ERR_ABORTED);
  process()->SimulateRenderProcessExit(base::TERMINATION_STATUS_OOM, 0);

  // Would get the notification synchronously.
  EXPECT_EQ(url2, last_oom_url_.value());
  last_oom_url_.reset();

  NavigateAndCommit(url1);

  // Should navigate to an error page.
  content::NavigationSimulator::NavigateAndFailFromBrowser(
      web_contents(), url3, net::ERR_CONNECTION_RESET);
  // Don't report OOMs on error pages.
  process()->SimulateRenderProcessExit(base::TERMINATION_STATUS_OOM, 0);
  EXPECT_FALSE(last_oom_url_.has_value());
}
