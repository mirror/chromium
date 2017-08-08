// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/command_line.h"
#include "base/path_service.h"
#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversions.h"
#include "base/test/histogram_tester.h"
#include "base/test/scoped_feature_list.h"
#include "build/build_config.h"
#include "chrome/browser/browsing_data/browsing_data_cookie_helper.h"
#include "chrome/browser/chrome_notification_types.h"
#include "chrome/browser/content_settings/cookie_settings_factory.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/browser/content_settings/tab_specific_content_settings.h"
#include "chrome/browser/metrics/subprocess_metrics_provider.h"
#include "chrome/browser/net/url_request_mock_util.h"
#include "chrome/browser/plugins/chrome_plugin_service_filter.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_commands.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/common/chrome_features.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/test_launcher_utils.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/content_settings/core/browser/cookie_settings.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/notification_observer.h"
#include "content/public/browser/notification_service.h"
#include "content/public/browser/plugin_service.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/content_constants.h"
#include "content/public/common/content_switches.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/ppapi_test_utils.h"
#include "content/public/test/test_utils.h"
#include "media/cdm/cdm_paths.h"
#include "media/media_features.h"
#include "net/dns/mock_host_resolver.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "net/test/url_request/url_request_mock_http_job.h"
#include "ppapi/features/features.h"
#include "ppapi/shared_impl/ppapi_switches.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "widevine_cdm_version.h"  // In SHARED_INTERMEDIATE_DIR.

#if defined(OS_MACOSX)
#include "base/mac/scoped_nsautorelease_pool.h"
#endif

#if BUILDFLAG(ENABLE_LIBRARY_CDMS)
#include "chrome/browser/media/pepper_cdm_test_helper.h"
#endif

using content::BrowserThread;
using net::URLRequestMockHTTPJob;

class ClientHintsBrowserTest : public InProcessBrowserTest {
 public:
  ClientHintsBrowserTest()
      : https_server_(net::EmbeddedTestServer::TYPE_HTTPS) {
    https_server_.ServeFilesFromSourceDirectory("chrome/test/data");
  }

  void SetUpOnMainThread() override {
    BrowserThread::PostTask(
        BrowserThread::IO, FROM_HERE,
        base::BindOnce(&chrome_browser_net::SetUrlRequestMocksEnabled, true));
  }

  void SetUpCommandLine(base::CommandLine* cmd) override {
    cmd->AppendSwitch(switches::kEnableExperimentalWebPlatformFeatures);
  }

  net::EmbeddedTestServer https_server_;
};

// Loads a webpage that requests persisting of client hints. Verifies that
// the browser receives the mojo notification from the renderer and persists the
// client hints to the disk.
IN_PROC_BROWSER_TEST_F(ClientHintsBrowserTest, ClientHintsHttps) {
  https_server_.ServeFilesFromSourceDirectory("content/test/data");

  ASSERT_TRUE(https_server_.Start());
  GURL url = https_server_.GetURL("/client_hints_lifetime.html");
  ASSERT_TRUE(url.SchemeIsHTTPOrHTTPS());
  ASSERT_TRUE(url.SchemeIsCryptographic());

  base::HistogramTester histogram_tester;
  ui_test_utils::NavigateToURL(browser(), url);

  histogram_tester.ExpectUniqueSample(
      "ContentSettings.ClientHints.UpdateEventCount", 1, 1);

  content::FetchHistogramsFromChildProcesses();
  SubprocessMetricsProvider::MergeHistogramDeltasForTesting();

  // client_hints_lifetime.html sets two client hints.
  histogram_tester.ExpectUniqueSample("ContentSettings.ClientHints.UpdateSize",
                                      2, 1);
  // client_hints_lifetime.html sets client hints persist duration to 3600
  // seconds.
  histogram_tester.ExpectUniqueSample(
      "ContentSettings.ClientHints.PersistDuration", 3600 * 1000, 1);
}

// Check the client hints for the given URL in an incognito window.
// Start incognito browser twice to ensure that client hints prefs are
// not carried over.
IN_PROC_BROWSER_TEST_F(ClientHintsBrowserTest, ClientHintsHttpsIncognito) {
  https_server_.ServeFilesFromSourceDirectory("content/test/data");

  ASSERT_TRUE(https_server_.Start());
  GURL url = https_server_.GetURL("/client_hints_lifetime.html");
  ASSERT_TRUE(url.SchemeIsHTTPOrHTTPS());
  ASSERT_TRUE(url.SchemeIsCryptographic());

  for (size_t i = 0; i < 2; ++i) {
    base::HistogramTester histogram_tester;

    Browser* incognito = CreateIncognitoBrowser();
    ui_test_utils::NavigateToURL(incognito, url);

    histogram_tester.ExpectUniqueSample(
        "ContentSettings.ClientHints.UpdateEventCount", 1, 1);

    content::FetchHistogramsFromChildProcesses();
    SubprocessMetricsProvider::MergeHistogramDeltasForTesting();

    // |url| sets two client hints.
    histogram_tester.ExpectUniqueSample(
        "ContentSettings.ClientHints.UpdateSize", 2, 1);

    // At least one renderer must have been created. All the renderers created
    // must have read 0 client hints.
    EXPECT_LE(
        1u, histogram_tester
                .GetAllSamples("ContentSettings.ClientHints.CountRulesReceived")
                .size());
    for (const auto& bucket : histogram_tester.GetAllSamples(
             "ContentSettings.ClientHints.CountRulesReceived")) {
      EXPECT_EQ(0, bucket.min);
    }
    // |url| sets client hints persist duration to 3600 seconds.
    histogram_tester.ExpectUniqueSample(
        "ContentSettings.ClientHints.PersistDuration", 3600 * 1000, 1);

    CloseBrowserSynchronously(incognito);
  }
}

#endif  // BUILDFLAG(ENABLE_PLUGINS)
