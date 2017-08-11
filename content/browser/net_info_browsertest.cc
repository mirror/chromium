// Copyright 2014 The Chromium Authors. All rights reserved.
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
#include "net/dns/mock_host_resolver.h"
#include "net/log/test_net_log.h"
#include "net/nqe/effective_connection_type.h"
#include "net/nqe/network_quality_estimator_test_util.h"

namespace {

// Returns the total count of samples in |histogram|.
int GetTotalSampleCount(base::HistogramTester* tester,
                        const std::string& histogram) {
  int count = 0;
  std::vector<base::Bucket> buckets = tester->GetAllSamples(histogram);
  for (const auto& bucket : buckets)
    count += bucket.count;
  return count;
}

void VerifyRtt(base::TimeDelta expected_rtt, int32_t got_rtt_milliseconds) {
  EXPECT_EQ(0, got_rtt_milliseconds % 50)
      << " got_rtt_milliseconds=" << got_rtt_milliseconds;

  if (expected_rtt > base::TimeDelta::FromMilliseconds(3000))
    expected_rtt = base::TimeDelta::FromMilliseconds(3000);
  // The difference between the actual and the estimate value should be within
  // 10%.
  EXPECT_GE(expected_rtt.InMilliseconds() * 0.1,
            std::abs(expected_rtt.InMilliseconds() - got_rtt_milliseconds));
}

void VerifyDownlinkKbps(double expected_kbps, double got_kbps) {
  EXPECT_EQ(0, static_cast<int>(got_kbps) % 50) << " got_kbps=" << got_kbps;
  if (expected_kbps > 10000)
    expected_kbps = 10000;
  // The difference between the actual and the estimate value should be within
  // 10%.
  EXPECT_GE(expected_kbps * 0.1, std::abs(expected_kbps - got_kbps));
}

}  // namespace

namespace content {

class NetInfoBrowserTest : public content::ContentBrowserTest {
 protected:
  void SetUpCommandLine(base::CommandLine* command_line) override {
    // TODO(jkarlin): Once NetInfo is enabled on all platforms remove this
    // switch.
    command_line->AppendSwitch(switches::kEnableNetworkInformationDownlinkMax);

    // TODO(jkarlin): Remove this once downlinkMax is no longer
    // experimental.
    command_line->AppendSwitch(
        switches::kEnableExperimentalWebPlatformFeatures);
  }

  void SetUp() override {
    net::NetworkChangeNotifier::SetTestNotificationsOnly(true);

#if defined(OS_CHROMEOS)
    // ChromeOS's NetworkChangeNotifier isn't known to content and therefore
    // doesn't get created in content_browsertests. Insert a mock
    // NetworkChangeNotifier.
    net::NetworkChangeNotifier::CreateMock();
#endif

    content::ContentBrowserTest::SetUp();
  }

  void SetUpOnMainThread() override {
    host_resolver()->AddRule("*", "127.0.0.1");
    base::RunLoop().RunUntilIdle();
  }

  static void SetConnectionType(
      net::NetworkChangeNotifier::ConnectionType type,
      net::NetworkChangeNotifier::ConnectionSubtype subtype) {
    net::NetworkChangeNotifier::NotifyObserversOfMaxBandwidthChangeForTests(
        net::NetworkChangeNotifier::GetMaxBandwidthForConnectionSubtype(
            subtype),
        type);
    base::RunLoop().RunUntilIdle();
  }

  std::string RunScriptExtractString(const std::string& script) {
    std::string data;
    EXPECT_TRUE(ExecuteScriptAndExtractString(shell(), script, &data));
    return data;
  }

  bool RunScriptExtractBool(const std::string& script) {
    bool data;
    EXPECT_TRUE(ExecuteScriptAndExtractBool(shell(), script, &data));
    return data;
  }

  double RunScriptExtractDouble(const std::string& script) {
    double data = 0.0;
    EXPECT_TRUE(ExecuteScriptAndExtractDouble(shell(), script, &data));
    return data;
  }

  int RunScriptExtractInt(const std::string& script) {
    int data = 0;
    EXPECT_TRUE(ExecuteScriptAndExtractInt(shell(), script, &data));
    return data;
  }
};

// Make sure the type is correct when the page is first opened.
IN_PROC_BROWSER_TEST_F(NetInfoBrowserTest, VerifyNetworkStateInitialized) {
  SetConnectionType(net::NetworkChangeNotifier::CONNECTION_ETHERNET,
                    net::NetworkChangeNotifier::SUBTYPE_GIGABIT_ETHERNET);
  NavigateToURL(shell(), content::GetTestUrl("", "net_info.html"));
  EXPECT_TRUE(RunScriptExtractBool("getOnLine()"));
  EXPECT_EQ("ethernet", RunScriptExtractString("getType()"));
  EXPECT_EQ(net::NetworkChangeNotifier::GetMaxBandwidthForConnectionSubtype(
                net::NetworkChangeNotifier::SUBTYPE_GIGABIT_ETHERNET),
            RunScriptExtractDouble("getDownlinkMax()"));
}

// Make sure that type changes in the browser make their way to
// navigator.connection.type.
IN_PROC_BROWSER_TEST_F(NetInfoBrowserTest, NetworkChangePlumbsToNavigator) {
  NavigateToURL(shell(), content::GetTestUrl("", "net_info.html"));
  SetConnectionType(net::NetworkChangeNotifier::CONNECTION_WIFI,
                    net::NetworkChangeNotifier::SUBTYPE_WIFI_N);
  EXPECT_EQ("wifi", RunScriptExtractString("getType()"));
  EXPECT_EQ(net::NetworkChangeNotifier::GetMaxBandwidthForConnectionSubtype(
                net::NetworkChangeNotifier::SUBTYPE_WIFI_N),
            RunScriptExtractDouble("getDownlinkMax()"));

  SetConnectionType(net::NetworkChangeNotifier::CONNECTION_ETHERNET,
                    net::NetworkChangeNotifier::SUBTYPE_GIGABIT_ETHERNET);
  EXPECT_EQ("ethernet", RunScriptExtractString("getType()"));
  EXPECT_EQ(net::NetworkChangeNotifier::GetMaxBandwidthForConnectionSubtype(
                net::NetworkChangeNotifier::SUBTYPE_GIGABIT_ETHERNET),
            RunScriptExtractDouble("getDownlinkMax()"));
}

// Make sure that type changes in the browser make their way to
// navigator.isOnline.
IN_PROC_BROWSER_TEST_F(NetInfoBrowserTest, IsOnline) {
  NavigateToURL(shell(), content::GetTestUrl("", "net_info.html"));
  SetConnectionType(net::NetworkChangeNotifier::CONNECTION_ETHERNET,
                    net::NetworkChangeNotifier::SUBTYPE_GIGABIT_ETHERNET);
  EXPECT_TRUE(RunScriptExtractBool("getOnLine()"));
  SetConnectionType(net::NetworkChangeNotifier::CONNECTION_NONE,
                    net::NetworkChangeNotifier::SUBTYPE_NONE);
  EXPECT_FALSE(RunScriptExtractBool("getOnLine()"));
  SetConnectionType(net::NetworkChangeNotifier::CONNECTION_WIFI,
                    net::NetworkChangeNotifier::SUBTYPE_WIFI_N);
  EXPECT_TRUE(RunScriptExtractBool("getOnLine()"));
}

// Creating a new render view shouldn't reinitialize Blink's
// NetworkStateNotifier. See https://crbug.com/535081.
IN_PROC_BROWSER_TEST_F(NetInfoBrowserTest, TwoRenderViewsInOneProcess) {
  SetConnectionType(net::NetworkChangeNotifier::CONNECTION_ETHERNET,
                    net::NetworkChangeNotifier::SUBTYPE_GIGABIT_ETHERNET);
  NavigateToURL(shell(), content::GetTestUrl("", "net_info.html"));
  EXPECT_TRUE(RunScriptExtractBool("getOnLine()"));

  SetConnectionType(net::NetworkChangeNotifier::CONNECTION_NONE,
                    net::NetworkChangeNotifier::SUBTYPE_NONE);
  EXPECT_FALSE(RunScriptExtractBool("getOnLine()"));

  // Open the same page in a new window on the same process.
  EXPECT_TRUE(ExecuteScript(shell(), "window.open(\"net_info.html\")"));

  // The network state should not have reinitialized to what it was when opening
  // the first window (online).
  EXPECT_FALSE(RunScriptExtractBool("getOnLine()"));
}

// Verify that when the network quality notifications are not sent, the
// Javascript API returns a valid estimate that is multiple of 50 msec and 50
// kbps.
IN_PROC_BROWSER_TEST_F(NetInfoBrowserTest,
                       NetworkQualityEstimatorNotInitialized) {
  base::HistogramTester histogram_tester;
  net::TestNetworkQualityEstimator estimator(
      nullptr, std::map<std::string, std::string>(), false, false, true, true,
      base::MakeUnique<net::BoundTestNetLog>());
  NetworkQualityObserverImpl impl(&estimator);

  LOG(WARNING) << "xxx cp0";

  EXPECT_TRUE(embedded_test_server()->Start());
  LOG(WARNING) << "xxx cp1";
  EXPECT_TRUE(
      NavigateToURL(shell(), embedded_test_server()->GetURL("/net_info.html")));
  LOG(WARNING) << "xxx cp2";

  EXPECT_EQ(0, RunScriptExtractInt("getRtt()"));
  LOG(WARNING) << "xxx cp3";

  VerifyDownlinkKbps(10000, RunScriptExtractDouble("getDownlink()") * 1000);
}

// Make sure the changes in the effective connection typeare notified to the
// render thread.
IN_PROC_BROWSER_TEST_F(NetInfoBrowserTest,
                       EffectiveConnectionTypeChangeNotfied) {
  base::HistogramTester histogram_tester;
  net::TestNetworkQualityEstimator estimator(
      nullptr, std::map<std::string, std::string>(), false, false, true, true,
      base::MakeUnique<net::BoundTestNetLog>());
  NetworkQualityObserverImpl impl(&estimator);

  net::nqe::internal::NetworkQuality network_quality_1(
      base::TimeDelta::FromSeconds(1), base::TimeDelta::FromSeconds(2), 300);
  estimator.NotifyObserversOfRTTOrThroughputEstimatesComputed(
      network_quality_1);

  EXPECT_TRUE(embedded_test_server()->Start());
  EXPECT_TRUE(
      NavigateToURL(shell(), embedded_test_server()->GetURL("/net_info.html")));

  FetchHistogramsFromChildProcesses();

  int samples =
      GetTotalSampleCount(&histogram_tester, "NQE.RenderThreadNotified");
  EXPECT_LT(0, samples);

  // Change effective connection type so that the renderer process is notified.
  // Changing the effective connection type from 2G to 3G is guaranteed to
  // generate the notification to the renderers, irrespective of the current
  // effective connection type.
  estimator.NotifyObserversOfEffectiveConnectionType(
      net::EFFECTIVE_CONNECTION_TYPE_2G);
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ("2g", RunScriptExtractString("getEffectiveType()"));
  estimator.NotifyObserversOfEffectiveConnectionType(
      net::EFFECTIVE_CONNECTION_TYPE_3G);
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ("3g", RunScriptExtractString("getEffectiveType()"));
  FetchHistogramsFromChildProcesses();
  base::RunLoop().RunUntilIdle();
  EXPECT_GT(GetTotalSampleCount(&histogram_tester, "NQE.RenderThreadNotified"),
            samples);
}

// Make sure the changes in the network quality are notified to the render
// thread, and the changed network quality is accessible via Javascript API.
IN_PROC_BROWSER_TEST_F(NetInfoBrowserTest, NetworkQualityChangeNotified) {
  base::HistogramTester histogram_tester;
  net::TestNetworkQualityEstimator estimator(
      nullptr, std::map<std::string, std::string>(), false, false, true, true,
      base::MakeUnique<net::BoundTestNetLog>());
  NetworkQualityObserverImpl impl(&estimator);

  net::nqe::internal::NetworkQuality network_quality_1(
      base::TimeDelta::FromSeconds(1), base::TimeDelta::FromSeconds(2), 300);
  estimator.NotifyObserversOfRTTOrThroughputEstimatesComputed(
      network_quality_1);

  EXPECT_TRUE(embedded_test_server()->Start());
  EXPECT_TRUE(
      NavigateToURL(shell(), embedded_test_server()->GetURL("/net_info.html")));

  FetchHistogramsFromChildProcesses();
  EXPECT_FALSE(
      histogram_tester.GetAllSamples("NQE.RenderThreadNotified").empty());

  VerifyRtt(network_quality_1.http_rtt(), RunScriptExtractInt("getRtt()"));
  VerifyDownlinkKbps(network_quality_1.downstream_throughput_kbps(),
                     RunScriptExtractDouble("getDownlink()") * 1000);

  // Verify that the network quality change is accessible via Javascript API.
  net::nqe::internal::NetworkQuality network_quality_2(
      base::TimeDelta::FromSeconds(10), base::TimeDelta::FromSeconds(20), 3000);
  estimator.NotifyObserversOfRTTOrThroughputEstimatesComputed(
      network_quality_2);
  base::RunLoop().RunUntilIdle();
  VerifyRtt(network_quality_2.http_rtt(), RunScriptExtractInt("getRtt()"));
  VerifyDownlinkKbps(network_quality_2.downstream_throughput_kbps(),
                     RunScriptExtractDouble("getDownlink()") * 1000);
}

// Make sure the changes in the network quality are rounded to the nearest
// 50 milliseconds or 50 kbps.
IN_PROC_BROWSER_TEST_F(NetInfoBrowserTest, NetworkQualityChangeRounded) {
  base::HistogramTester histogram_tester;
  net::TestNetworkQualityEstimator estimator(
      std::unique_ptr<net::ExternalEstimateProvider>(),
      std::map<std::string, std::string>(), false, false, true, true,
      base::MakeUnique<net::BoundTestNetLog>());
  NetworkQualityObserverImpl impl(&estimator);

  // Verify that the network quality is rounded properly.
  net::nqe::internal::NetworkQuality network_quality_1(
      base::TimeDelta::FromMilliseconds(103),
      base::TimeDelta::FromMilliseconds(212), 8303);
  estimator.NotifyObserversOfRTTOrThroughputEstimatesComputed(
      network_quality_1);

  EXPECT_TRUE(embedded_test_server()->Start());
  EXPECT_TRUE(
      NavigateToURL(shell(), embedded_test_server()->GetURL("/net_info.html")));
  VerifyRtt(base::TimeDelta::FromMilliseconds(100),
            RunScriptExtractInt("getRtt()"));
  VerifyDownlinkKbps(8300, RunScriptExtractDouble("getDownlink()") * 1000);

  net::nqe::internal::NetworkQuality network_quality_2(
      base::TimeDelta::FromMilliseconds(1103),
      base::TimeDelta::FromMilliseconds(212), 1307);
  estimator.NotifyObserversOfRTTOrThroughputEstimatesComputed(
      network_quality_2);
  base::RunLoop().RunUntilIdle();
  VerifyRtt(base::TimeDelta::FromMilliseconds(1100),
            RunScriptExtractInt("getRtt()"));
  VerifyDownlinkKbps(1300, RunScriptExtractDouble("getDownlink()") * 1000);

  net::nqe::internal::NetworkQuality network_quality_3(
      base::TimeDelta::FromMilliseconds(12),
      base::TimeDelta::FromMilliseconds(12), 12);
  estimator.NotifyObserversOfRTTOrThroughputEstimatesComputed(
      network_quality_3);
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(0, RunScriptExtractInt("getRtt()"));
  EXPECT_EQ(0, RunScriptExtractDouble("getDownlink()"));
}

// Make sure the network quality are rounded down when it exceeds the upper
// limit.
IN_PROC_BROWSER_TEST_F(NetInfoBrowserTest, NetworkQualityChangeUpperLimit) {
  base::HistogramTester histogram_tester;
  net::TestNetworkQualityEstimator estimator(
      std::unique_ptr<net::ExternalEstimateProvider>(),
      std::map<std::string, std::string>(), false, false, true, true,
      base::MakeUnique<net::BoundTestNetLog>());
  NetworkQualityObserverImpl impl(&estimator);

  net::nqe::internal::NetworkQuality network_quality(
      base::TimeDelta::FromMilliseconds(12003),
      base::TimeDelta::FromMilliseconds(212), 30300);
  estimator.NotifyObserversOfRTTOrThroughputEstimatesComputed(network_quality);

  EXPECT_TRUE(embedded_test_server()->Start());
  EXPECT_TRUE(
      NavigateToURL(shell(), embedded_test_server()->GetURL("/net_info.html")));
  VerifyRtt(base::TimeDelta::FromMilliseconds(10000),
            RunScriptExtractInt("getRtt()"));
  VerifyDownlinkKbps(10000, RunScriptExtractDouble("getDownlink()") * 1000);
}

// Make sure the noise added to the network quality varies with the hostname
IN_PROC_BROWSER_TEST_F(NetInfoBrowserTest, NetworkQualityRandomized) {
  base::HistogramTester histogram_tester;
  net::TestNetworkQualityEstimator estimator(
      std::unique_ptr<net::ExternalEstimateProvider>(),
      std::map<std::string, std::string>(), false, false, true, true,
      base::MakeUnique<net::BoundTestNetLog>());
  NetworkQualityObserverImpl impl(&estimator);

  net::nqe::internal::NetworkQuality network_quality(
      base::TimeDelta::FromMilliseconds(2000),
      base::TimeDelta::FromMilliseconds(200), 3000);
  estimator.NotifyObserversOfRTTOrThroughputEstimatesComputed(network_quality);

  EXPECT_TRUE(embedded_test_server()->Start());

  EXPECT_TRUE(
      NavigateToURL(shell(), embedded_test_server()->GetURL("/net_info.html")));
  VerifyRtt(base::TimeDelta::FromMilliseconds(2000),
            RunScriptExtractInt("getRtt()"));
  VerifyDownlinkKbps(3000, RunScriptExtractDouble("getDownlink()") * 1000);

  const int32_t rtt_noise_milliseconds = RunScriptExtractInt("getRtt()") - 2000;
  const int32_t downlink_noise_kbps =
      RunScriptExtractDouble("getDownlink()") * 1000 - 3000;

  // When the hostname is not changed, the noise should not change.
  EXPECT_TRUE(
      NavigateToURL(shell(), embedded_test_server()->GetURL("/net_info.html")));
  VerifyRtt(base::TimeDelta::FromMilliseconds(2000),
            RunScriptExtractInt("getRtt()"));
  VerifyDownlinkKbps(3000, RunScriptExtractDouble("getDownlink()") * 1000);
  EXPECT_EQ(rtt_noise_milliseconds, RunScriptExtractInt("getRtt()") - 2000);
  EXPECT_EQ(downlink_noise_kbps,
            RunScriptExtractDouble("getDownlink()") * 1000 - 3000);

  // Verify that changing the hostname changes the noise. It is possible that
  // the hash of a different host also maps to the same bucket among 20 buckets.
  // Try 10 different hosts. This reduces the probability of failure of this
  // test to (1/20)^10 = 9,7 * 10^-14.
  for (size_t i = 0; i < 10; ++i) {
    // The noise added is a function of the hostname. Varying the hostname
    // should vary the noise.
    std::string fake_hostname = "example" + base::IntToString(i) + ".com";
    EXPECT_TRUE(NavigateToURL(shell(), embedded_test_server()->GetURL(
                                           fake_hostname, "/net_info.html")));
    VerifyRtt(base::TimeDelta::FromMilliseconds(2000),
              RunScriptExtractInt("getRtt()"));
    VerifyDownlinkKbps(3000, RunScriptExtractDouble("getDownlink()") * 1000);

    int32_t new_rtt_noise_milliseconds = RunScriptExtractInt("getRtt()") - 2000;
    int32_t new_downlink_noise_kbps =
        RunScriptExtractDouble("getDownlink()") * 1000 - 3000;

    if (rtt_noise_milliseconds != new_rtt_noise_milliseconds &&
        downlink_noise_kbps != new_downlink_noise_kbps) {
      return;
    }
  }
  NOTREACHED() << "Noise not added to the network quality estimates";
}

// Make sure the minor changes (<10%) in the network quality are not notified.
IN_PROC_BROWSER_TEST_F(NetInfoBrowserTest, NetworkQualityChangeNotNotified) {
  base::HistogramTester histogram_tester;
  net::TestNetworkQualityEstimator estimator(
      nullptr, std::map<std::string, std::string>(), false, false, true, true,
      base::MakeUnique<net::BoundTestNetLog>());
  NetworkQualityObserverImpl impl(&estimator);

  // Verify that the network quality is rounded properly.
  net::nqe::internal::NetworkQuality network_quality_1(
      base::TimeDelta::FromMilliseconds(1123),
      base::TimeDelta::FromMilliseconds(1212), 1303);
  estimator.NotifyObserversOfRTTOrThroughputEstimatesComputed(
      network_quality_1);

  EXPECT_TRUE(embedded_test_server()->Start());
  EXPECT_TRUE(
      NavigateToURL(shell(), embedded_test_server()->GetURL("/net_info.html")));
  VerifyRtt(base::TimeDelta::FromMilliseconds(1100),
            RunScriptExtractInt("getRtt()"));
  VerifyDownlinkKbps(1300, RunScriptExtractDouble("getDownlink()") * 1000);

  // All the 3 metrics change by less than 10%. So, the observers are not
  // notified.
  net::nqe::internal::NetworkQuality network_quality_2(
      base::TimeDelta::FromMilliseconds(1223),
      base::TimeDelta::FromMilliseconds(1312), 1403);
  estimator.NotifyObserversOfRTTOrThroughputEstimatesComputed(
      network_quality_2);
  base::RunLoop().RunUntilIdle();
  VerifyRtt(base::TimeDelta::FromMilliseconds(1100),
            RunScriptExtractInt("getRtt()"));
  VerifyDownlinkKbps(1300, RunScriptExtractDouble("getDownlink()") * 1000);

  // HTTP RTT has changed by more than 10% from the last notified value of
  // |network_quality_1|. The observers should be notified.
  net::nqe::internal::NetworkQuality network_quality_3(
      base::TimeDelta::FromMilliseconds(2223),
      base::TimeDelta::FromMilliseconds(1312), 1403);
  estimator.NotifyObserversOfRTTOrThroughputEstimatesComputed(
      network_quality_3);
  base::RunLoop().RunUntilIdle();
  VerifyRtt(base::TimeDelta::FromMilliseconds(2200),
            RunScriptExtractInt("getRtt()"));
  VerifyDownlinkKbps(1400, RunScriptExtractDouble("getDownlink()") * 1000);
}

}  // namespace content
