// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/metrics/out_of_memory_reporter.h"

#include <set>
#include <utility>

#include "base/callback.h"
#include "base/macros.h"
#include "base/optional.h"
#include "base/run_loop.h"
#include "build/build_config.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/common/url_constants.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

class OutOfMemoryReporterBrowserTest : public InProcessBrowserTest,
                                       public OutOfMemoryReporter::Observer {
 public:
  OutOfMemoryReporterBrowserTest() {}
  ~OutOfMemoryReporterBrowserTest() override {}

  // OutOfMemoryReporter::Observer:
  void OnForegroundOOMDetected(const GURL& url,
                               ukm::SourceId source_id) override {
    if (url == url_to_wait_for_oom_) {
      std::move(wait_closure_).Run();
      url_to_wait_for_oom_.reset();
    }
  }

  void WaitForOOMOnUrl(const GURL& url) {
    url_to_wait_for_oom_ = url;
    base::RunLoop run_loop;
    wait_closure_ = run_loop.QuitClosure();
    run_loop.Run();
  }

 private:
  base::Optional<GURL> url_to_wait_for_oom_;
  base::OnceClosure wait_closure_;
  DISALLOW_COPY_AND_ASSIGN(OutOfMemoryReporterBrowserTest);
};

// No current reliable way to determine OOM on Linux.
#if defined(OS_LINUX)
#define MAYBE_MemoryExhuast DISABLED_MemoryExhaust
#else
#define MAYBE_MemoryExhuast MemoryExhaust
#endif
IN_PROC_BROWSER_TEST_F(OutOfMemoryReporterBrowserTest, MAYBE_MemoryExhaust) {
  content::WebContents* web_contents =
      browser()->tab_strip_model()->GetActiveWebContents();
  auto* reporter = OutOfMemoryReporter::FromWebContents(web_contents);
  ASSERT_TRUE(reporter);
  reporter->AddObserver(this);
  const GURL exhaust_url(content::kChromeUIMemoryExhaustURL);

  ui_test_utils::NavigateToURL(browser(), exhaust_url);

  // Will timeout if the signal is not sent.
  WaitForOOMOnUrl(exhaust_url);
}
