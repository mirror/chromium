// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <cmath>  // For std::modf.
#include <map>
#include <string>

#include "base/command_line.h"
#include "base/run_loop.h"
#include "base/strings/string_number_conversions.h"
#include "base/test/histogram_tester.h"
#include "build/build_config.h"
#include "content/browser/net/network_quality_observer_impl.h"
#include "content/public/common/content_switches.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/content_browser_test.h"
#include "content/public/test/content_browser_test_utils.h"
#include "content/shell/browser/shell.h"
#include "net/base/network_change_notifier.h"
#include "net/base/network_change_notifier_factory.h"
#include "net/log/test_net_log.h"
#include "net/nqe/effective_connection_type.h"
#include "net/nqe/network_quality_estimator_test_util.h"

#include "net/base/net_errors.h"
#include "net/dns/mock_host_resolver.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "net/test/embedded_test_server/http_request.h"
#include "net/test/embedded_test_server/http_response.h"
#include "net/test/url_request/url_request_failed_job.h"
#include "net/test/url_request/url_request_mock_http_job.h"
#include "net/url_request/url_request.h"
#include "url/gurl.h"

namespace content {

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

  void SetUpInProcessBrowserTestFixture() override {
    ASSERT_TRUE(embedded_test_server()->Start());
  }

  void SetUpOnMainThread() override { base::RunLoop().RunUntilIdle(); }

  net::EmbeddedTestServer https_server_;
};

// Make sure the type is correct when the page is first opened.
IN_PROC_BROWSER_TEST_F(ClientHintsBrowserTest, VerifyNetworkStateInitialized) {
  ASSERT_TRUE(https_server_.Start());
  base::HistogramTester histogram_tester;
  EXPECT_TRUE(NavigateToURL(
      shell(), https_server_.GetURL("/client_hints_lifetime.html")));

  FetchHistogramsFromChildProcesses();
  histogram_tester.ExpectBucketCount("Blink.UseCounter.Features", 2065 /*
      static_cast<int>(WebFeature::kPersistentClientHintHeader)*/, 1);
}

}  // namespace content
