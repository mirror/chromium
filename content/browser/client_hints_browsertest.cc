// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/command_line.h"
#include "base/run_loop.h"
#include "base/test/histogram_tester.h"
#include "content/public/common/content_switches.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/content_browser_test.h"
#include "content/public/test/content_browser_test_utils.h"
#include "content/shell/browser/shell.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "url/gurl.h"

namespace content {

namespace {

class ClientHintsBrowserTest : public content::ContentBrowserTest {
 public:
  ClientHintsBrowserTest()
      : https_server_(net::EmbeddedTestServer::TYPE_HTTPS) {
    https_server_.ServeFilesFromSourceDirectory("content/test/data");
  }

 protected:
  void SetUpCommandLine(base::CommandLine* command_line) override {
    command_line->AppendSwitch(
        switches::kEnableExperimentalWebPlatformFeatures);
  }

  void SetUpOnMainThread() override { base::RunLoop().RunUntilIdle(); }

  net::EmbeddedTestServer https_server_;
};

IN_PROC_BROWSER_TEST_F(ClientHintsBrowserTest, Basic) {
  ASSERT_TRUE(https_server_.Start());
  base::HistogramTester histogram_tester;
  EXPECT_TRUE(NavigateToURL(
      shell(), https_server_.GetURL("/client_hints_lifetime.html")));

  FetchHistogramsFromChildProcesses();
  histogram_tester.ExpectBucketCount("Blink.UseCounter.Features", 2065 /*
      static_cast<int>(WebFeature::kPersistentClientHintHeader)*/, 1);
}

}  // namespace

}  // namespace content
