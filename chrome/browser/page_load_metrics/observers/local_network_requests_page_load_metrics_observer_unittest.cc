// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/page_load_metrics/observers/local_network_requests_page_load_metrics_observer.h"

#include <vector>

#include "base/memory/ptr_util.h"
#include "base/metrics/metrics_hashes.h"
#include "base/test/histogram_tester.h"
#include "chrome/browser/page_load_metrics/metrics_web_contents_observer.h"
#include "chrome/browser/page_load_metrics/observers/page_load_metrics_observer_test_harness.h"
#include "chrome/browser/page_load_metrics/page_load_metrics_observer.h"
#include "chrome/browser/page_load_metrics/page_load_metrics_util.h"
#include "chrome/browser/page_load_metrics/page_load_tracker.h"
#include "chrome/test/base/testing_browser_process.h"
#include "components/ukm/test_ukm_recorder.h"
#include "components/ukm/ukm_source.h"
//#include "content/common/frame_messages.h"
//#include "content/common/navigation_gesture.h"
//#include "content/test/test_render_frame_host.h"
#include "net/base/host_port_pair.h"
#include "net/base/ip_address.h"
#include "ui/base/page_transition_types.h"
#include "url/gurl.h"

struct FrameHostMsg_DidCommitProvisionalLoad_Params;

namespace internal {

typedef struct {
  char* url;
  char* host_ip;
  uint16_t port;
} PageAddressInfo;

typedef struct {
  ResourceType resource_type;
  PortType port_type;
  int success_count, failed_count;
} UkmMetricInfo;

static const PageAddressInfo
    kPublicPage = {(char*)"https://foo.com/", (char*)"216.58.195.78", 443},
    kPublicPageIPv6 = {(char*)"https://google.com/",
                       (char*)"[2607:f8b0:4005:809::200e]", 443},
    kPrivatePage = {(char*)"http://test.local/", (char*)"192.168.10.123", 80},
    kLocalhostPage = {(char*)"http://localhost/", (char*)"127.0.0.1", 80},
    kLocalhostPageIPv6 = {(char*)"http://[::1]/", (char*)"[::1]", 80},
    kPublicRequest1 = {(char*)"http://bar.com/", (char*)"100.150.200.250", 80},
    kPublicRequest2 = {(char*)"https://www.baz.com/", (char*)"192.10.20.30",
                       443},
    kSameSubnetRequest1 = {(char*)"http://test2.local:9000/",
                           (char*)"192.168.10.200", 9000},
    kSameSubnetRequest2 = {(char*)"http://test2.local:8000/index.html",
                           (char*)"192.168.10.200", 8000},
    kSameSubnetRequest3 = {(char*)"http://test2.local:8000/bar.html",
                           (char*)"192.168.10.200", 8000},
    kDiffSubnetRequest1 = {(char*)"http://10.0.10.200/", (char*)"10.0.10.200",
                           80},
    kDiffSubnetRequest2 = {(char*)"http://172.16.0.85:8181/",
                           (char*)"172.16.0.85", 8181},
    kDiffSubnetRequest3 = {(char*)"http://10.15.20.25:12345/",
                           (char*)"10.15.20.25", 12345},
    kDiffSubnetRequest4 = {(char*)"http://172.31.100.20:515/",
                           (char*)"172.31.100.20", 515},
    kLocalhostRequest1 = {(char*)"http://localhost:8080/", (char*)"127.0.0.1",
                          8080},  // WEB
    kLocalhostRequest2 = {(char*)"http://127.0.1.1:3306/", (char*)"127.0.1.1",
                          3306},  // DB
    kLocalhostRequest3 = {(char*)"http://localhost:515/", (char*)"127.0.2.1",
                          515},  // PRINT
    kLocalhostRequest4 = {(char*)"http://127.100.150.200:9000/",
                          (char*)"127.100.150.200", 9000},  // DEV
    kLocalhostRequest5 = {(char*)"http://127.0.0.1:9876/", (char*)"127.0.0.1",
                          9876},  // OTHER
    kRouterRequest1 = {(char*)"http://10.0.0.1/", (char*)"10.0.0.1", 80},
    kRouterRequest2 = {(char*)"https://192.168.10.1/", (char*)"192.168.10.1",
                       443};

// const char kSameSubnetURLSuffix[] = "index.html";

}  // namespace internal

class LocalNetworkRequestsPageLoadMetricsObserverTest
    :  // public UkmPageLoadMetricsObserverTest,
       public page_load_metrics::PageLoadMetricsObserverTestHarness {
 protected:
  void RegisterObservers(page_load_metrics::PageLoadTracker* tracker) override {
    tracker->AddObserver(
        base::MakeUnique<LocalNetworkRequestsPageLoadMetricsObserver>());
    TestingBrowserProcess::GetGlobal()->SetUkmRecorder(&test_ukm_recorder_);
  }

  void SetUp() override {
    page_load_metrics::PageLoadMetricsObserverTestHarness::SetUp();
  }

  void SimulateNavigateAndCommit(const internal::PageAddressInfo& page) {
    GURL url(page.url);
    net::HostPortPair socket_address(page.host_ip, page.port);
    content::TestRenderFrameHost* rfh_tester =
        static_cast<content::TestRenderFrameHost*>(
            content::RenderFrameHostTester::For(main_rfh()));

    rfh_tester->SimulateNavigationStart(url);

    FrameHostMsg_DidCommitProvisionalLoad_Params params;
    params.nav_entry_id = 0;
    params.url = url;
    params.origin = url::Origin(url);
    params.transition = ui::PAGE_TRANSITION_LINK;
    params.should_update_history = false;
    params.did_create_new_entry = false;
    params.gesture = content::NavigationGesture::NavigationGestureUser;
    params.contents_mime_type = "text/html";
    params.method = "GET";
    params.http_status_code = 200;
    params.socket_address = socket_address;
    params.original_request_url = url;
    params.page_state = content::PageState::CreateFromURL(url);

    rfh_tester->SendNavigateWithParams(&params);
  }

  void SimulateLoadedSuccessfulResource(
      const internal::PageAddressInfo& resource) {
    SimulateLoadedResource(resource, 0);
  }

  void SimulateLoadedFailedResource(const internal::PageAddressInfo& resource) {
    SimulateLoadedResource(resource, -102);
  }

  void SimulateLoadedResource(const internal::PageAddressInfo& resource,
                              const int net_error) {
    page_load_metrics::ExtraRequestCompleteInfo request_info(
        GURL(resource.url), net::HostPortPair(resource.host_ip, resource.port),
        -1 /* frame_tree_node_id */, !net_error /*was_cached*/,
        (net_error ? 1024 * 20 : 0) /* raw_body_bytes */,
        0 /* original_network_content_length */,
        nullptr /* data_reduction_proxy_data */,
        content::ResourceType::RESOURCE_TYPE_MAIN_FRAME, net_error);

    PageLoadMetricsObserverTestHarness::SimulateLoadedResource(request_info);
  }

  void NavigateToPageAndLoadResources(
      const internal::PageAddressInfo& page,
      const std::vector<internal::PageAddressInfo>& resources,
      const std::vector<bool>& resource_succeeded) {
    SimulateNavigateAndCommit(page);
    for (size_t i = 0; i < resources.size(); ++i) {
      if (resource_succeeded[i]) {
        SimulateLoadedSuccessfulResource(resources[i]);
      } else {
        SimulateLoadedFailedResource(resources[i]);
      }
    }
    DeleteContents();
  }

  // Helper functions to verify that particular slices of UMA histograms are
  // empty.
  void ExpectEmptyHistograms(DomainType domain_type) {
    for (const auto& port :
         internal::kLocalhostHistogramNames.at(domain_type)) {
      for (const auto& histogramName : port.second) {
        histogram_tester().ExpectTotalCount(histogramName.second, 1);
        histogram_tester().ExpectUniqueSample(histogramName.second, 0, 1);
      }
    }
    for (const auto& resource :
         internal::kNonlocalhostHistogramNames.at(domain_type)) {
      for (const auto& histogramName : resource.second) {
        histogram_tester().ExpectTotalCount(histogramName.second, 1);
        histogram_tester().ExpectUniqueSample(histogramName.second, 0, 1);
      }
    }
  }

  void ExpectNoHistograms() {
    for (const auto& domain : internal::kLocalhostHistogramNames) {
      for (const auto& port : domain.second) {
        for (const auto& status : port.second) {
          histogram_tester().ExpectTotalCount(status.second, 0);
        }
      }
    }
    for (const auto& domain : internal::kNonlocalhostHistogramNames) {
      for (const auto& resource : domain.second) {
        for (const auto& status : resource.second) {
          histogram_tester().ExpectTotalCount(status.second, 0);
        }
      }
    }
  }

  void ExpectPageLoadMetric(const internal::PageAddressInfo& page,
                            const DomainType domain_type) {
    EXPECT_EQ(1ul, ukm_source_count());
    const ukm::UkmSource* source = GetUkmSourceForUrl(page.url);
    EXPECT_EQ(GURL(page.url), source->url());

    auto* entry = GetUkmEntriesForSourceID(source->id())[0];
    EXPECT_EQ(entry->source_id, source->id());
    EXPECT_EQ(entry->event_hash,
              base::HashMetricName(internal::kUkmPageLoadEventName));
    EXPECT_FALSE(entry->metrics.empty());
    ExpectMetric(internal::kUkmDomainTypeName, static_cast<int>(domain_type),
                 entry);
  }

  void ExpectMetricsAndHistograms(
      const internal::PageAddressInfo& page,
      const std::vector<internal::UkmMetricInfo>& expected_metrics,
      const std::map<std::string, int>& expected_histograms) {
    const ukm::UkmSource* source = GetUkmSourceForUrl(page.url);
    EXPECT_EQ(expected_metrics.size() + 1, ukm_entry_count());
    auto entries = GetUkmEntriesForSourceID(source->id());
    for (size_t i = 1; i < entries.size(); ++i) {
      EXPECT_EQ(entries[i]->source_id, source->id());
      EXPECT_EQ(
          entries[i]->event_hash,
          base::HashMetricName(internal::kUkmLocalNetworkRequestsEventName));
      EXPECT_FALSE(entries[i]->metrics.empty());

      EXPECT_EQ(entries[i]->metrics[0]->metric_hash,
                base::HashMetricName(internal::kUkmResourceTypeName));
      if (entries[i]->metrics[0]->value ==
          static_cast<int>(RESOURCE_TYPE_LOCALHOST)) {
        // Localhost page load
        EXPECT_EQ(entries[i]->metrics.size(), 4ul);
        ExpectMetric(internal::kUkmPortTypeName,
                     static_cast<int>(expected_metrics[i - 1].port_type),
                     entries[i]);
        ExpectMetric(internal::kUkmSuccessfulCountName,
                     expected_metrics[i - 1].success_count, entries[i]);
        ExpectMetric(internal::kUkmFailedCountName,
                     expected_metrics[i - 1].failed_count, entries[i]);
      } else {
        // Nonlocalhost page load
        EXPECT_EQ(entries[i]->metrics.size(), 3ul);
        EXPECT_EQ(expected_metrics[i - 1].resource_type,
                  static_cast<ResourceType>(entries[i]->metrics[0]->value));
        ExpectMetric(internal::kUkmSuccessfulCountName,
                     expected_metrics[i - 1].success_count, entries[i]);
        ExpectMetric(internal::kUkmFailedCountName,
                     expected_metrics[i - 1].failed_count, entries[i]);
      }
    }

    // Should have generated UMA histograms for all requests made.
    for (auto hist : expected_histograms) {
      histogram_tester().ExpectTotalCount(hist.first, 1);
      histogram_tester().ExpectUniqueSample(hist.first, hist.second, 1);
    }
  }

  // Copied from UkmPageLoadMetricsObserverTest

  size_t ukm_source_count() { return test_ukm_recorder_.sources_count(); }

  size_t ukm_entry_count() { return test_ukm_recorder_.entries_count(); }

  const ukm::UkmSource* GetUkmSourceForUrl(const char* url) {
    return test_ukm_recorder_.GetSourceForUrl(url);
  }

  std::vector<const ukm::mojom::UkmEntry*> GetUkmEntriesForSourceID(
      ukm::SourceId source_id) {
    std::vector<const ukm::mojom::UkmEntry*> entries;
    for (size_t i = 0; i < ukm_entry_count(); ++i) {
      const ukm::mojom::UkmEntry* entry = test_ukm_recorder_.GetEntry(i);
      if (entry->source_id == source_id)
        entries.push_back(entry);
    }
    return entries;
  }

  static void ExpectMetric(const char* name,
                           int64_t expected_value,
                           const ukm::mojom::UkmEntry* entry) {
    const ukm::mojom::UkmMetric* metric =
        ukm::TestUkmRecorder::FindMetric(entry, name);
    EXPECT_NE(nullptr, metric) << "Failed to find metric: " << name;
    EXPECT_EQ(expected_value, metric->value)
        << "Value of metric " << name << " is incorrect.";
  }

 private:
  ukm::TestUkmRecorder test_ukm_recorder_;
};

TEST_F(LocalNetworkRequestsPageLoadMetricsObserverTest, NoMetrics) {
  EXPECT_EQ(0ul, ukm_source_count());
  EXPECT_EQ(0ul, ukm_entry_count());

  // Sanity check
  ExpectNoHistograms();
}

TEST_F(LocalNetworkRequestsPageLoadMetricsObserverTest,
       PublicPageIPv6PublicRequests) {
  // Navigate to a public page and make only public resource requests.
  const internal::PageAddressInfo& page = internal::kPublicPageIPv6;
  NavigateToPageAndLoadResources(
      page, {internal::kPublicRequest1, internal::kPublicPageIPv6},
      {true, true});

  // Should generate only a domain type UKM entry and nothing else.
  ExpectPageLoadMetric(page, DOMAIN_TYPE_PUBLIC);
  ExpectEmptyHistograms(DOMAIN_TYPE_PUBLIC);
}

TEST_F(LocalNetworkRequestsPageLoadMetricsObserverTest,
       PublicPagePublicRequests) {
  // Navigate to a public page and make only public resource requests.
  const internal::PageAddressInfo& page = internal::kPublicPage;
  NavigateToPageAndLoadResources(
      page,
      {internal::kPublicRequest1, internal::kPublicRequest2,
       internal::kPublicPageIPv6},
      {true, true, true});

  // Should generate only a domain type UKM entry and nothing else.
  ExpectPageLoadMetric(page, DOMAIN_TYPE_PUBLIC);
  ExpectEmptyHistograms(DOMAIN_TYPE_PUBLIC);
}

TEST_F(LocalNetworkRequestsPageLoadMetricsObserverTest,
       PrivatePageSelfRequests) {
  // Navigate to a private page and make resource requests only to the page
  // itself.
  const internal::PageAddressInfo& page = internal::kSameSubnetRequest1;
  NavigateToPageAndLoadResources(
      page,
      {internal::kSameSubnetRequest2, internal::kSameSubnetRequest3, page},
      {true, false, true});

  // Should generate only a domain type UKM entry and nothing else.
  ExpectPageLoadMetric(page, DOMAIN_TYPE_PRIVATE);
  ExpectEmptyHistograms(DOMAIN_TYPE_PRIVATE);
}

TEST_F(LocalNetworkRequestsPageLoadMetricsObserverTest, PrivatePageNoRequests) {
  // Navigate to a private page and make no resource requests.
  const internal::PageAddressInfo& page = internal::kPrivatePage;
  NavigateToPageAndLoadResources(page, std::vector<internal::PageAddressInfo>{},
                                 std::vector<bool>{});

  // Should generate only a domain type UKM entry and nothing else.
  ExpectPageLoadMetric(page, DOMAIN_TYPE_PRIVATE);
  ExpectEmptyHistograms(DOMAIN_TYPE_PRIVATE);
}

TEST_F(LocalNetworkRequestsPageLoadMetricsObserverTest, LocalhostPage) {
  // Navigate to a localhost page and make some arbitrary resource requests.
  const internal::PageAddressInfo& page = internal::kLocalhostPage;
  NavigateToPageAndLoadResources(
      page,
      {internal::kPublicRequest1, internal::kPublicRequest2,
       internal::kSameSubnetRequest1, internal::kLocalhostRequest5},
      {true, false, true, true});

  // Should generate only a domain type UKM entry and nothing else.
  ExpectPageLoadMetric(page, DOMAIN_TYPE_LOCALHOST);
  ExpectNoHistograms();
}

TEST_F(LocalNetworkRequestsPageLoadMetricsObserverTest, LocalhostPageIPv6) {
  // Navigate to a localhost page with an IPv6 address and make some arbitrary
  // resource requests.
  const internal::PageAddressInfo& page = internal::kLocalhostPageIPv6;
  NavigateToPageAndLoadResources(
      page,
      {internal::kPublicRequest1, internal::kLocalhostRequest2,
       internal::kDiffSubnetRequest1, internal::kLocalhostRequest4},
      {false, true, false, true});

  // Should generate only a domain type UKM entry and nothing else.
  ExpectPageLoadMetric(page, DOMAIN_TYPE_LOCALHOST);
  ExpectNoHistograms();
}

TEST_F(LocalNetworkRequestsPageLoadMetricsObserverTest,
       PublicPageAllSuccessfulRequests) {
  // Navigate to a public page and make successful resource requests to all
  // resource types.
  const internal::PageAddressInfo& page = internal::kPublicPage;
  NavigateToPageAndLoadResources(
      page,
      {
          internal::kPublicPage,         internal::kPublicPageIPv6,
          internal::kPrivatePage,        internal::kLocalhostPage,
          internal::kLocalhostPageIPv6,  internal::kPublicRequest1,
          internal::kPublicRequest2,     internal::kSameSubnetRequest1,
          internal::kSameSubnetRequest2, internal::kSameSubnetRequest3,
          internal::kDiffSubnetRequest1, internal::kDiffSubnetRequest2,
          internal::kLocalhostRequest1,  internal::kLocalhostRequest2,
          internal::kLocalhostRequest3,  internal::kLocalhostRequest4,
          internal::kLocalhostRequest5,  internal::kRouterRequest1,
          internal::kRouterRequest2,
      },
      std::vector<bool>(19, true));

  // Should now have generated UKM entries for each of the types of resources
  // requested except for the public resources.
  ExpectPageLoadMetric(page, DOMAIN_TYPE_PUBLIC);

  // We should now see UKM entries and UMA histograms for each of the types of
  // resources requested except for public resources.
  ExpectMetricsAndHistograms(
      page,
      // List of expected UKM metric values, in the sorted order the metrics are
      // expected to be generated in.
      {
          {RESOURCE_TYPE_ROUTER, PORT_TYPE_WEB, 1, 0},   // 10.0.0.1:80
          {RESOURCE_TYPE_PRIVATE, PORT_TYPE_WEB, 1, 0},  // 10.0.10.200:80
          {RESOURCE_TYPE_PRIVATE, PORT_TYPE_WEB, 1, 0},  // 172.16.0.85:8181
          {RESOURCE_TYPE_ROUTER, PORT_TYPE_WEB, 1, 0},   // 192.168.10.1:443
          {RESOURCE_TYPE_PRIVATE, PORT_TYPE_WEB, 1, 0},  // 192.168.10.123:80
          {RESOURCE_TYPE_PRIVATE, PORT_TYPE_WEB, 3, 0},  // 192.168.10.200:8000
          {RESOURCE_TYPE_LOCALHOST, PORT_TYPE_WEB, 2, 0},    // 127.0.0.1:80
          {RESOURCE_TYPE_LOCALHOST, PORT_TYPE_PRINT, 1, 0},  // 127.0.2.1:515
          {RESOURCE_TYPE_LOCALHOST, PORT_TYPE_DB, 1, 0},     // 127.0.1.1:3306
          {RESOURCE_TYPE_LOCALHOST, PORT_TYPE_WEB, 1, 0},    // 127.0.0.1:8080
          {RESOURCE_TYPE_LOCALHOST, PORT_TYPE_DEV, 1,
           0},  // 127.100.150.200:9000
          {RESOURCE_TYPE_LOCALHOST, PORT_TYPE_OTHER, 1, 0},  // 127.0.0.1:9876
      },
      // List of expected nonzero UMA histogram values.
      {
          {internal::kNonlocalhostHistogramNames.at(DOMAIN_TYPE_PUBLIC)
               .at(RESOURCE_TYPE_ROUTER)
               .at(true),
           2},
          {internal::kNonlocalhostHistogramNames.at(DOMAIN_TYPE_PUBLIC)
               .at(RESOURCE_TYPE_PRIVATE)
               .at(true),
           6},
          {internal::kLocalhostHistogramNames.at(DOMAIN_TYPE_PUBLIC)
               .at(PORT_TYPE_WEB)
               .at(true),
           3},
          {internal::kLocalhostHistogramNames.at(DOMAIN_TYPE_PUBLIC)
               .at(PORT_TYPE_PRINT)
               .at(true),
           1},
          {internal::kLocalhostHistogramNames.at(DOMAIN_TYPE_PUBLIC)
               .at(PORT_TYPE_DB)
               .at(true),
           1},
          {internal::kLocalhostHistogramNames.at(DOMAIN_TYPE_PUBLIC)
               .at(PORT_TYPE_DEV)
               .at(true),
           1},
          {internal::kLocalhostHistogramNames.at(DOMAIN_TYPE_PUBLIC)
               .at(PORT_TYPE_OTHER)
               .at(true),
           1},
      });
}

TEST_F(LocalNetworkRequestsPageLoadMetricsObserverTest,
       PrivatePageAllSuccessfulRequests) {
  // Navigate to a private page and make successful resource requests to all
  // resource types.
  const internal::PageAddressInfo& page = internal::kPrivatePage;
  NavigateToPageAndLoadResources(
      page,
      {
          internal::kPublicPage,         internal::kPublicPageIPv6,
          internal::kPrivatePage,        internal::kLocalhostPage,
          internal::kLocalhostPageIPv6,  internal::kPublicRequest1,
          internal::kPublicRequest2,     internal::kSameSubnetRequest1,
          internal::kSameSubnetRequest2, internal::kSameSubnetRequest3,
          internal::kDiffSubnetRequest1, internal::kDiffSubnetRequest2,
          internal::kLocalhostRequest1,  internal::kLocalhostRequest2,
          internal::kLocalhostRequest3,  internal::kLocalhostRequest4,
          internal::kLocalhostRequest5,  internal::kRouterRequest1,
          internal::kRouterRequest2,
      },
      std::vector<bool>(19, true));

  // Should now have generated UKM entries for each of the types of resources
  // requested except for the public resources.
  ExpectPageLoadMetric(page, DOMAIN_TYPE_PRIVATE);

  // We should now see UKM entries and UMA histograms for each of the types of
  // resources requested except for the request to the page itself.
  ExpectMetricsAndHistograms(
      page,
      // List of expected UKM metric values, in the sorted order the metrics are
      // expected to be generated in.
      {
          {RESOURCE_TYPE_LOCAL_DIFF_SUBNET, PORT_TYPE_WEB, 1,
           0},  // 10.0.0.1:80
          {RESOURCE_TYPE_LOCAL_DIFF_SUBNET, PORT_TYPE_WEB, 1,
           0},                                          // 10.0.10.200:80
          {RESOURCE_TYPE_PUBLIC, PORT_TYPE_WEB, 1, 0},  // 100.150.200.250:80
          {RESOURCE_TYPE_LOCAL_DIFF_SUBNET, PORT_TYPE_WEB, 1,
           0},                                          // 172.16.0.85:8181
          {RESOURCE_TYPE_PUBLIC, PORT_TYPE_WEB, 1, 0},  // 192.10.20.30:443
          {RESOURCE_TYPE_LOCAL_SAME_SUBNET, PORT_TYPE_WEB, 1,
           0},  // 192.168.10.1:443
          {RESOURCE_TYPE_LOCAL_SAME_SUBNET, PORT_TYPE_WEB, 3,
           0},                                          // 192.168.10.200:8000
          {RESOURCE_TYPE_PUBLIC, PORT_TYPE_WEB, 1, 0},  // 216.58.195.78:443
          {RESOURCE_TYPE_PUBLIC, PORT_TYPE_WEB, 1,
           0},  // [2607:f8b0:4005:809::200e]:443
          {RESOURCE_TYPE_LOCALHOST, PORT_TYPE_WEB, 2, 0},    // 127.0.0.1:80
          {RESOURCE_TYPE_LOCALHOST, PORT_TYPE_PRINT, 1, 0},  // 127.0.2.1:515
          {RESOURCE_TYPE_LOCALHOST, PORT_TYPE_DB, 1, 0},     // 127.0.1.1:3306
          {RESOURCE_TYPE_LOCALHOST, PORT_TYPE_WEB, 1, 0},    // 127.0.0.1:8080
          {RESOURCE_TYPE_LOCALHOST, PORT_TYPE_DEV, 1,
           0},  // 127.100.150.200:9000
          {RESOURCE_TYPE_LOCALHOST, PORT_TYPE_OTHER, 1, 0},  // 127.0.0.1:9876
      },
      // List of expected nonzero UMA histogram values.
      {
          {internal::kNonlocalhostHistogramNames.at(DOMAIN_TYPE_PRIVATE)
               .at(RESOURCE_TYPE_PUBLIC)
               .at(true),
           4},
          {internal::kNonlocalhostHistogramNames.at(DOMAIN_TYPE_PRIVATE)
               .at(RESOURCE_TYPE_LOCAL_DIFF_SUBNET)
               .at(true),
           3},
          {internal::kNonlocalhostHistogramNames.at(DOMAIN_TYPE_PRIVATE)
               .at(RESOURCE_TYPE_LOCAL_SAME_SUBNET)
               .at(true),
           4},
          {internal::kLocalhostHistogramNames.at(DOMAIN_TYPE_PRIVATE)
               .at(PORT_TYPE_WEB)
               .at(true),
           3},
          {internal::kLocalhostHistogramNames.at(DOMAIN_TYPE_PRIVATE)
               .at(PORT_TYPE_PRINT)
               .at(true),
           1},
          {internal::kLocalhostHistogramNames.at(DOMAIN_TYPE_PRIVATE)
               .at(PORT_TYPE_DB)
               .at(true),
           1},
          {internal::kLocalhostHistogramNames.at(DOMAIN_TYPE_PRIVATE)
               .at(PORT_TYPE_DEV)
               .at(true),
           1},
          {internal::kLocalhostHistogramNames.at(DOMAIN_TYPE_PRIVATE)
               .at(PORT_TYPE_OTHER)
               .at(true),
           1},
      });
}

TEST_F(LocalNetworkRequestsPageLoadMetricsObserverTest,
       PrivatePageAllFailedRequests) {
  // Navigate to a private page and make successful resource requests to all
  // resource types.
  const internal::PageAddressInfo& page = internal::kPrivatePage;
  NavigateToPageAndLoadResources(
      page,
      {
          internal::kPublicPage,         internal::kPublicPageIPv6,
          internal::kPrivatePage,        internal::kLocalhostPage,
          internal::kLocalhostPageIPv6,  internal::kPublicRequest1,
          internal::kPublicRequest2,     internal::kSameSubnetRequest1,
          internal::kSameSubnetRequest2, internal::kSameSubnetRequest3,
          internal::kDiffSubnetRequest1, internal::kDiffSubnetRequest2,
          internal::kLocalhostRequest1,  internal::kLocalhostRequest2,
          internal::kLocalhostRequest3,  internal::kLocalhostRequest4,
          internal::kLocalhostRequest5,  internal::kRouterRequest1,
          internal::kRouterRequest2,
      },
      std::vector<bool>(19, false));

  // Should now have generated UKM entries for each of the types of resources
  // requested except for the public resources.
  ExpectPageLoadMetric(page, DOMAIN_TYPE_PRIVATE);

  ExpectMetricsAndHistograms(
      page,
      // List of expected UKM metric values, in the sorted order the metrics are
      // expected to be generated in.
      {
          {RESOURCE_TYPE_LOCAL_DIFF_SUBNET, PORT_TYPE_WEB, 0,
           1},  // 10.0.0.1:80
          {RESOURCE_TYPE_LOCAL_DIFF_SUBNET, PORT_TYPE_WEB, 0,
           1},                                          // 10.0.10.200:80
          {RESOURCE_TYPE_PUBLIC, PORT_TYPE_WEB, 0, 1},  // 100.150.200.250:80
          {RESOURCE_TYPE_LOCAL_DIFF_SUBNET, PORT_TYPE_WEB, 0,
           1},                                          // 172.16.0.85:8181
          {RESOURCE_TYPE_PUBLIC, PORT_TYPE_WEB, 0, 1},  // 192.10.20.30:443
          {RESOURCE_TYPE_LOCAL_SAME_SUBNET, PORT_TYPE_WEB, 0,
           1},  // 192.168.10.1:443
          {RESOURCE_TYPE_LOCAL_SAME_SUBNET, PORT_TYPE_WEB, 0,
           3},                                          // 192.168.10.200:8000
          {RESOURCE_TYPE_PUBLIC, PORT_TYPE_WEB, 0, 1},  // 216.58.195.78:443
          {RESOURCE_TYPE_PUBLIC, PORT_TYPE_WEB, 0,
           1},  // [2607:f8b0:4005:809::200e]:443
          {RESOURCE_TYPE_LOCALHOST, PORT_TYPE_WEB, 0, 2},    // 127.0.0.1:80
          {RESOURCE_TYPE_LOCALHOST, PORT_TYPE_PRINT, 0, 1},  // 127.0.2.1:515
          {RESOURCE_TYPE_LOCALHOST, PORT_TYPE_DB, 0, 1},     // 127.0.1.1:3306
          {RESOURCE_TYPE_LOCALHOST, PORT_TYPE_WEB, 0, 1},    // 127.0.0.1:8080
          {RESOURCE_TYPE_LOCALHOST, PORT_TYPE_DEV, 0,
           1},  // 127.100.150.200:9000
          {RESOURCE_TYPE_LOCALHOST, PORT_TYPE_OTHER, 0, 1},  // 127.0.0.1:9876
      },
      // List of expected nonzero UMA histogram values.
      {
          {internal::kNonlocalhostHistogramNames.at(DOMAIN_TYPE_PRIVATE)
               .at(RESOURCE_TYPE_PUBLIC)
               .at(false),
           4},
          {internal::kNonlocalhostHistogramNames.at(DOMAIN_TYPE_PRIVATE)
               .at(RESOURCE_TYPE_LOCAL_DIFF_SUBNET)
               .at(false),
           3},
          {internal::kNonlocalhostHistogramNames.at(DOMAIN_TYPE_PRIVATE)
               .at(RESOURCE_TYPE_LOCAL_SAME_SUBNET)
               .at(false),
           4},
          {internal::kLocalhostHistogramNames.at(DOMAIN_TYPE_PRIVATE)
               .at(PORT_TYPE_WEB)
               .at(false),
           3},
          {internal::kLocalhostHistogramNames.at(DOMAIN_TYPE_PRIVATE)
               .at(PORT_TYPE_PRINT)
               .at(false),
           1},
          {internal::kLocalhostHistogramNames.at(DOMAIN_TYPE_PRIVATE)
               .at(PORT_TYPE_DB)
               .at(false),
           1},
          {internal::kLocalhostHistogramNames.at(DOMAIN_TYPE_PRIVATE)
               .at(PORT_TYPE_DEV)
               .at(false),
           1},
          {internal::kLocalhostHistogramNames.at(DOMAIN_TYPE_PRIVATE)
               .at(PORT_TYPE_OTHER)
               .at(false),
           1},
      });
}

TEST_F(LocalNetworkRequestsPageLoadMetricsObserverTest,
       PublicPageMixedStatusRequests) {
  // Navigate to a public page and make mixed status resource requests.
  const internal::PageAddressInfo& page = internal::kPublicPage;
  NavigateToPageAndLoadResources(
      page,
      {internal::kPublicRequest1, internal::kSameSubnetRequest1,
       internal::kLocalhostRequest2, internal::kDiffSubnetRequest2,
       internal::kLocalhostRequest5, internal::kDiffSubnetRequest2,
       internal::kRouterRequest1},
      {true, true, false, true, false, false, true});

  // Should now have generated UKM entries for each of the types of resources
  // requested except for the public resources.
  ExpectPageLoadMetric(page, DOMAIN_TYPE_PUBLIC);

  ExpectMetricsAndHistograms(
      page,
      // List of expected UKM metric values, in the sorted order the metrics are
      // expected to be generated in.
      {
          {RESOURCE_TYPE_ROUTER, PORT_TYPE_WEB, 1, 0},    // 10.0.0.1:80
          {RESOURCE_TYPE_PRIVATE, PORT_TYPE_WEB, 1, 1},   // 172.16.0.85:8181
          {RESOURCE_TYPE_PRIVATE, PORT_TYPE_DEV, 1, 0},   // 192.168.10.200:8000
          {RESOURCE_TYPE_LOCALHOST, PORT_TYPE_DB, 0, 1},  // 127.0.1.1:3306
          {RESOURCE_TYPE_LOCALHOST, PORT_TYPE_OTHER, 0, 1},  // 127.0.0.1:9876
      },
      // List of expected nonzero UMA histogram values.
      {
          {internal::kNonlocalhostHistogramNames.at(DOMAIN_TYPE_PUBLIC)
               .at(RESOURCE_TYPE_ROUTER)
               .at(true),
           1},
          {internal::kNonlocalhostHistogramNames.at(DOMAIN_TYPE_PUBLIC)
               .at(RESOURCE_TYPE_PRIVATE)
               .at(true),
           2},
          {internal::kNonlocalhostHistogramNames.at(DOMAIN_TYPE_PUBLIC)
               .at(RESOURCE_TYPE_PRIVATE)
               .at(false),
           1},
          {internal::kLocalhostHistogramNames.at(DOMAIN_TYPE_PUBLIC)
               .at(PORT_TYPE_DB)
               .at(false),
           1},
          {internal::kLocalhostHistogramNames.at(DOMAIN_TYPE_PUBLIC)
               .at(PORT_TYPE_OTHER)
               .at(false),
           1},
      });
}

TEST_F(LocalNetworkRequestsPageLoadMetricsObserverTest,
       PublicPageLargeNumberOfRequests) {
  // This test also verifies the sequence and timing of UKM metric generation.

  // Navigate to a public page with an IPv6 address.
  const internal::PageAddressInfo& page = internal::kPublicPageIPv6;
  SimulateNavigateAndCommit(page);

  // Should generate only a domain type UKM entry by this point.
  ExpectPageLoadMetric(page, DOMAIN_TYPE_PUBLIC);

  // Make 100 each of many different types of requests, with 1000 of a single
  // type.
  std::vector<std::pair<internal::PageAddressInfo, bool>> requests = {
      {internal::kPublicRequest1, true},
      {internal::kLocalhostPage, true},
      {internal::kLocalhostPageIPv6, false},
      {internal::kSameSubnetRequest1, false},
      {internal::kDiffSubnetRequest2, false},
  };
  for (auto request : requests) {
    for (int i = 0; i < 100; ++i) {
      SimulateLoadedResource(request.first, (request.second ? 0 : -1));
    }
  }
  for (int i = 0; i < 1000; ++i) {
    SimulateLoadedSuccessfulResource(internal::kDiffSubnetRequest1);
  }

  // At this point, we should still only see the domain type UKM entry.
  EXPECT_EQ(1ul, ukm_entry_count());

  // Close the page.
  DeleteContents();

  ExpectMetricsAndHistograms(
      page,
      // List of expected UKM metric values, in the sorted order the metrics are
      // expected to be generated in.
      {
          {RESOURCE_TYPE_PRIVATE, PORT_TYPE_WEB, 1000, 0},  // 10.0.10.200:80
          {RESOURCE_TYPE_PRIVATE, PORT_TYPE_WEB, 0, 100},   // 172.16.0.85:8181
          {RESOURCE_TYPE_PRIVATE, PORT_TYPE_DEV, 0,
           100},  // 192.168.10.200:9000
          {RESOURCE_TYPE_LOCALHOST, PORT_TYPE_WEB, 100, 100},  // 127.0.0.1:80
      },
      // List of expected nonzero UMA histogram values.
      {
          {internal::kNonlocalhostHistogramNames.at(DOMAIN_TYPE_PUBLIC)
               .at(RESOURCE_TYPE_PRIVATE)
               .at(true),
           1000},
          {internal::kNonlocalhostHistogramNames.at(DOMAIN_TYPE_PUBLIC)
               .at(RESOURCE_TYPE_PRIVATE)
               .at(false),
           200},
          {internal::kLocalhostHistogramNames.at(DOMAIN_TYPE_PUBLIC)
               .at(PORT_TYPE_WEB)
               .at(true),
           100},
          {internal::kLocalhostHistogramNames.at(DOMAIN_TYPE_PUBLIC)
               .at(PORT_TYPE_WEB)
               .at(false),
           100},
      });
}

TEST_F(LocalNetworkRequestsPageLoadMetricsObserverTest,
       PublicPageRequestIpInUrlOnly) {
  const internal::PageAddressInfo& page = internal::kPublicPage;
  SimulateNavigateAndCommit(page);

  // Load a resource that has the IP address in the URL but returned an empty
  // socket address for some reason.
  PageLoadMetricsObserverTestHarness::SimulateLoadedResource({
      GURL(internal::kDiffSubnetRequest2.url), net::HostPortPair(),
      -1 /* frame_tree_node_id */, true /*was_cached*/,
      1024 * 20 /* raw_body_bytes */, 0 /* original_network_content_length */,
      nullptr /* data_reduction_proxy_data */,
      content::ResourceType::RESOURCE_TYPE_MAIN_FRAME, 0,
  });
  DeleteContents();

  // We should still see a UKM entry and UMA histogram for the resource request.
  ExpectPageLoadMetric(page, DOMAIN_TYPE_PUBLIC);
  ExpectMetricsAndHistograms(
      page, {{RESOURCE_TYPE_PRIVATE, PORT_TYPE_WEB, 1, 0}},
      {{internal::kNonlocalhostHistogramNames.at(DOMAIN_TYPE_PUBLIC)
            .at(RESOURCE_TYPE_PRIVATE)
            .at(true),
        1}});
}

TEST_F(LocalNetworkRequestsPageLoadMetricsObserverTest,
       PublicPageRequestIpNotPresent) {
  const internal::PageAddressInfo& page = internal::kPublicPage;
  SimulateNavigateAndCommit(page);

  // Load a resource that doesn't have the IP address in the URL and returned an
  // empty socket address (e.g., failed DNS resolution).
  PageLoadMetricsObserverTestHarness::SimulateLoadedResource({
      GURL(internal::kPrivatePage.url), net::HostPortPair(),
      -1 /* frame_tree_node_id */, false /*was_cached*/, 0 /* raw_body_bytes */,
      0 /* original_network_content_length */,
      nullptr /* data_reduction_proxy_data */,
      content::ResourceType::RESOURCE_TYPE_MAIN_FRAME, -20,
  });
  DeleteContents();

  // We shouldn't see any UKM entries or UMA histograms this time.
  ExpectPageLoadMetric(page, DOMAIN_TYPE_PUBLIC);
  ExpectEmptyHistograms(DOMAIN_TYPE_PUBLIC);
}

TEST_F(LocalNetworkRequestsPageLoadMetricsObserverTest, PrivatePageSubnet10) {
  // Navigate to a private page on the 10.0.0.0/8 subnet and make requests to
  // other 10.0.0.0/8 subnet resources.
  const internal::PageAddressInfo& page = internal::kRouterRequest1;
  NavigateToPageAndLoadResources(
      page,
      {internal::kDiffSubnetRequest1, internal::kDiffSubnetRequest3,
       internal::kRouterRequest2},
      {false, false, false});
  ExpectPageLoadMetric(page, DOMAIN_TYPE_PRIVATE);

  // The first two requests should be on the same subnet and the last request
  // should be on a different subnet.
  ExpectMetricsAndHistograms(
      page,
      {
          {RESOURCE_TYPE_LOCAL_SAME_SUBNET, PORT_TYPE_WEB, 0,
           1},  // 10.0.10.200:80
          {RESOURCE_TYPE_LOCAL_SAME_SUBNET, PORT_TYPE_OTHER, 0,
           1},  // 10.15.20.25:12345
          {RESOURCE_TYPE_LOCAL_DIFF_SUBNET, PORT_TYPE_WEB, 0,
           1},  // 192.168.10.1:443
      },
      {
          {internal::kNonlocalhostHistogramNames.at(DOMAIN_TYPE_PRIVATE)
               .at(RESOURCE_TYPE_LOCAL_SAME_SUBNET)
               .at(false),
           2},
          {internal::kNonlocalhostHistogramNames.at(DOMAIN_TYPE_PRIVATE)
               .at(RESOURCE_TYPE_LOCAL_DIFF_SUBNET)
               .at(false),
           1},
      });
}

TEST_F(LocalNetworkRequestsPageLoadMetricsObserverTest, PrivatePageSubnet172) {
  // Navigate to a private page on the 10.0.0.0/8 subnet and make requests to
  // other 10.0.0.0/8 subnet resources.
  const internal::PageAddressInfo& page = internal::kDiffSubnetRequest2;
  NavigateToPageAndLoadResources(
      page, {internal::kDiffSubnetRequest4, internal::kRouterRequest1},
      {false, false});
  ExpectPageLoadMetric(page, DOMAIN_TYPE_PRIVATE);

  // The first two requests should be on the same subnet and the last request
  // should be on a different subnet.
  ExpectMetricsAndHistograms(
      page,
      {
          {RESOURCE_TYPE_LOCAL_DIFF_SUBNET, PORT_TYPE_WEB, 0,
           1},  // 10.0.10.200:80
          {RESOURCE_TYPE_LOCAL_SAME_SUBNET, PORT_TYPE_PRINT, 0,
           1},  // 172.31.100.20:515
      },
      {
          {internal::kNonlocalhostHistogramNames.at(DOMAIN_TYPE_PRIVATE)
               .at(RESOURCE_TYPE_LOCAL_SAME_SUBNET)
               .at(false),
           1},
          {internal::kNonlocalhostHistogramNames.at(DOMAIN_TYPE_PRIVATE)
               .at(RESOURCE_TYPE_LOCAL_DIFF_SUBNET)
               .at(false),
           1},
      });
}

TEST_F(LocalNetworkRequestsPageLoadMetricsObserverTest, PrivatePageFailedLoad) {
  GURL url(internal::kPrivatePage.url);
  content::TestRenderFrameHost* rfh_tester =
      static_cast<content::TestRenderFrameHost*>(
          content::RenderFrameHostTester::For(main_rfh()));
  rfh_tester->SimulateNavigationStart(url);
  rfh_tester->SimulateNavigationError(url, -20);

  // Nothing should have been generated.
  EXPECT_EQ(0ul, ukm_source_count());
  EXPECT_EQ(0ul, ukm_entry_count());
  ExpectNoHistograms();
}
