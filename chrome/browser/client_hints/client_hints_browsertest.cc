// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/command_line.h"
#include "base/test/histogram_tester.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/browser/metrics/subprocess_metrics_provider.h"
#include "chrome/browser/net/url_request_mock_util.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/common/content_switches.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/test_utils.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "net/test/embedded_test_server/http_request.h"

class ClientHintsBrowserTest : public InProcessBrowserTest {
 public:
  ClientHintsBrowserTest()
      : https_server_(net::EmbeddedTestServer::TYPE_HTTPS),
        expect_dpr_client_hint_present_(false),
        expect_viewport_width_client_hint_present_(false) {
    https_server_.ServeFilesFromSourceDirectory(
        "chrome/test/data/client_hints");

    https_server_.RegisterRequestMonitor(
        base::Bind(&ClientHintsBrowserTest::MonitorResourceRequest,
                   base::Unretained(this)));

    EXPECT_TRUE(https_server_.Start());

    client_hint_url_ = https_server_.GetURL("/client_hints_lifetime.html");
    EXPECT_TRUE(client_hint_url_.SchemeIsHTTPOrHTTPS());
    EXPECT_TRUE(client_hint_url_.SchemeIsCryptographic());

    client_hint_no_lifetime_url_ =
        https_server_.GetURL("/client_hints_no_lifetime.html");
    EXPECT_TRUE(client_hint_url_.SchemeIsHTTPOrHTTPS());
    EXPECT_TRUE(client_hint_url_.SchemeIsCryptographic());

    no_client_hint_url_ =
        https_server_.GetURL("/no_client_hints_lifetime.html");
    EXPECT_TRUE(no_client_hint_url_.SchemeIsHTTPOrHTTPS());
    EXPECT_TRUE(no_client_hint_url_.SchemeIsCryptographic());
  }

  ~ClientHintsBrowserTest() override {}

  void SetUpOnMainThread() override {
    content::BrowserThread::PostTask(
        content::BrowserThread::IO, FROM_HERE,
        base::BindOnce(&chrome_browser_net::SetUrlRequestMocksEnabled, true));
  }

  void SetUpCommandLine(base::CommandLine* cmd) override {
    // cmd->AppendSwitch(switches::kSingleProcess);
    cmd->AppendSwitch(switches::kEnableExperimentalWebPlatformFeatures);
  }

  void SetExpectations(bool expect_dpr_client_hint_present,
                       bool expect_viewport_width_client_hint_present) {
    expect_dpr_client_hint_present_ = expect_dpr_client_hint_present;
    expect_viewport_width_client_hint_present_ =
        expect_viewport_width_client_hint_present;
  }

  // A URL whose response headers include Accept-CH and Accept-CH-Lifetime
  // headers.
  const GURL& client_hint_url() const { return client_hint_url_; }

  // A URL whose response headers include only Accept-CH header.
  const GURL& client_hint_no_lifetime_url() const {
    return client_hint_no_lifetime_url_;
  }

  // A URL whose response headers do not include either Accept-CH or
  // Accept-CH-Lifetime headers.
  const GURL& no_client_hint_url() const { return no_client_hint_url_; }

  GURL some_other_host() const { return GURL("https://www.example.com:443"); }

 private:
  // Called by |https_server_|.
  void MonitorResourceRequest(
      const net::test_server::HttpRequest& request) const {
    EXPECT_EQ(expect_dpr_client_hint_present_,
              request.headers.find("dpr") != request.headers.end())
        << "\nheaders=" << request.all_headers << " \nurl=" << request.GetURL()
        << " \nbase_url=" << request.base_url;
    EXPECT_EQ(expect_viewport_width_client_hint_present_,
              request.headers.find("viewport-width") != request.headers.end())
        << "\nheaders=" << request.all_headers << " \nurl=" << request.GetURL()
        << " \nbase_url=" << request.base_url;
  }

  net::EmbeddedTestServer https_server_;
  GURL client_hint_url_;
  GURL client_hint_no_lifetime_url_;
  GURL no_client_hint_url_;

  bool expect_dpr_client_hint_present_;
  bool expect_viewport_width_client_hint_present_;
};

// Loads a webpage that requests persisting of client hints. Verifies that
// the browser receives the mojo notification from the renderer and persists the
// client hints to the disk.
IN_PROC_BROWSER_TEST_F(ClientHintsBrowserTest, ClientHintsHttps) {
  base::HistogramTester histogram_tester;
  ui_test_utils::NavigateToURL(browser(), client_hint_url());

  histogram_tester.ExpectUniqueSample("ClientHints.UpdateEventCount", 1, 1);

  content::FetchHistogramsFromChildProcesses();
  SubprocessMetricsProvider::MergeHistogramDeltasForTesting();

  // client_hints_url() sets two client hints.
  histogram_tester.ExpectUniqueSample("ClientHints.UpdateSize", 2, 1);
  // client_hint_url() sets client hints persist duration to 3600 seconds.
  histogram_tester.ExpectUniqueSample("ClientHints.PersistDuration",
                                      3600 * 1000, 1);
}

// Loads a webpage that does not request persisting of client hints.
IN_PROC_BROWSER_TEST_F(ClientHintsBrowserTest, NoClientHintsHttps) {
  base::HistogramTester histogram_tester;
  ui_test_utils::NavigateToURL(browser(), no_client_hint_url());

  histogram_tester.ExpectTotalCount("ClientHints.UpdateEventCount", 0);

  content::FetchHistogramsFromChildProcesses();
  SubprocessMetricsProvider::MergeHistogramDeltasForTesting();

  // no_client_hints_url() does not sets the client hints.
  histogram_tester.ExpectTotalCount("ClientHints.UpdateSize", 0);
  histogram_tester.ExpectTotalCount("ClientHints.PersistDuration", 0);
}

IN_PROC_BROWSER_TEST_F(ClientHintsBrowserTest,
                       ClientHintsLifetimeFollowedByNoClientHint) {
  LOG(WARNING) << "xxx cp0";
  base::HistogramTester histogram_tester;
  ContentSettingsForOneType host_settings;

  HostContentSettingsMapFactory::GetForProfile(browser()->profile())
      ->GetSettingsForOneType(CONTENT_SETTINGS_TYPE_CLIENT_HINTS, std::string(),
                              &host_settings);
  EXPECT_EQ(0u, host_settings.size());

  // Fetching client_hint_url() should persist the request for client hints.
  ui_test_utils::NavigateToURL(browser(), client_hint_url());

  histogram_tester.ExpectUniqueSample("ClientHints.UpdateEventCount", 1, 1);

  content::FetchHistogramsFromChildProcesses();
  SubprocessMetricsProvider::MergeHistogramDeltasForTesting();

  // client_hints_url() sets two client hints.
  histogram_tester.ExpectUniqueSample("ClientHints.UpdateSize", 2, 1);
  // client_hint_url() sets client hints persist duration to 3600 seconds.
  histogram_tester.ExpectUniqueSample("ClientHints.PersistDuration",
                                      3600 * 1000, 1);
  base::RunLoop().RunUntilIdle();
  LOG(WARNING) << "xxx cp1";

  HostContentSettingsMapFactory::GetForProfile(browser()->profile())
      ->GetSettingsForOneType(CONTENT_SETTINGS_TYPE_CLIENT_HINTS, std::string(),
                              &host_settings);
  EXPECT_EQ(1u, host_settings.size());

  // Next request should have client hint headers attached.
  ui_test_utils::NavigateToURL(browser(), some_other_host());
  LOG(WARNING) << "xxx cp2";

  SetExpectations(true, true);
  ui_test_utils::NavigateToURL(browser(), no_client_hint_url());
}

IN_PROC_BROWSER_TEST_F(ClientHintsBrowserTest,
                       ClientHintsNoLifetimeFollowedByNoClientHint) {
  base::HistogramTester histogram_tester;
  ContentSettingsForOneType host_settings;

  HostContentSettingsMapFactory::GetForProfile(browser()->profile())
      ->GetSettingsForOneType(CONTENT_SETTINGS_TYPE_CLIENT_HINTS, std::string(),
                              &host_settings);
  EXPECT_EQ(0u, host_settings.size());

  // Fetching client_hint_no_lifetime_url() should not persist the request for
  // client hints.
  ui_test_utils::NavigateToURL(browser(), client_hint_no_lifetime_url());

  histogram_tester.ExpectTotalCount("ClientHints.UpdateEventCount", 0);

  content::FetchHistogramsFromChildProcesses();
  SubprocessMetricsProvider::MergeHistogramDeltasForTesting();

  histogram_tester.ExpectTotalCount("ClientHints.UpdateSize", 0);
  histogram_tester.ExpectTotalCount("ClientHints.PersistDuration", 0);
  base::RunLoop().RunUntilIdle();

  HostContentSettingsMapFactory::GetForProfile(browser()->profile())
      ->GetSettingsForOneType(CONTENT_SETTINGS_TYPE_CLIENT_HINTS, std::string(),
                              &host_settings);
  EXPECT_EQ(0u, host_settings.size());

  // Next request should not have client hint headers attached.
  ui_test_utils::NavigateToURL(browser(), no_client_hint_url());
}

// Check the client hints for the given URL in an incognito window.
// Start incognito browser twice to ensure that client hints prefs are
// not carried over.
IN_PROC_BROWSER_TEST_F(ClientHintsBrowserTest, ClientHintsHttpsIncognito) {
  for (size_t i = 0; i < 2; ++i) {
    base::HistogramTester histogram_tester;

    Browser* incognito = CreateIncognitoBrowser();
    ui_test_utils::NavigateToURL(incognito, client_hint_url());

    histogram_tester.ExpectUniqueSample("ClientHints.UpdateEventCount", 1, 1);

    content::FetchHistogramsFromChildProcesses();
    SubprocessMetricsProvider::MergeHistogramDeltasForTesting();

    // client_hint_url() sets two client hints.
    histogram_tester.ExpectUniqueSample("ClientHints.UpdateSize", 2, 1);

    // At least one renderer must have been created. All the renderers created
    // must have read 0 client hints.
    EXPECT_LE(1u,
              histogram_tester.GetAllSamples("ClientHints.CountRulesReceived")
                  .size());
    for (const auto& bucket :
         histogram_tester.GetAllSamples("ClientHints.CountRulesReceived")) {
      EXPECT_EQ(0, bucket.min);
    }
    // |url| sets client hints persist duration to 3600 seconds.
    histogram_tester.ExpectUniqueSample("ClientHints.PersistDuration",
                                        3600 * 1000, 1);

    CloseBrowserSynchronously(incognito);
  }
}
