// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/page_load_metrics/observers/local_network_requests_page_load_metrics_observer.h"

//#include <regex>
//#include <string>
//#include <utility>
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
// #include "content/common/frame_messages.h"
// #include "content/common/navigation_gesture.h"
// #include "content/test/test_render_frame_host.h"
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

static const PageAddressInfo
    kPublicPage = {(char*)"https://foo.com/", (char*)"216.58.195.78", 443},
    kPublicPageIPv6 = {(char*)"https://google.com/",
                       (char*)"[2607:f8b0:4005:809::200e]", 443},
    kPrivatePage = {(char*)"http://test.local/", (char*)"192.168.10.123", 80},
    kLocalhostPage = {(char*)"http://localhost:8080/", (char*)"127.0.0.1",
                      8080},
    kLocalhostPageIPv6 = {(char*)"http://[::1]/", (char*)"[::1]", 80},
    kPublicRequest1 = {(char*)"http://bar.com/", (char*)"100.150.200.250", 80},
    kPublicRequest2 = {(char*)"https://www.baz.com/", (char*)"192.10.20.30",
                       443},
    kSameSubnetRequest = {(char*)"http://test2.local/", (char*)"192.168.10.200",
                          9000},
    kDiffSubnetRequest1 = {(char*)"http://10.0.10.200/", (char*)"10.0.10.200",
                           80},
    kDiffSubnetRequest2 = {(char*)"http://172.16.0.85/", (char*)"172.16.0.85",
                           8181},
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
        -1 /* frame_tree_node_id */, true /*was_cached*/,
        1024 * 20 /* raw_body_bytes */, 0 /* original_network_content_length */,
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
  void ExpectHistogramsEmpty(DomainType domain_type) {
    for (const auto& port :
         internal::kLocalhostHistogramNames.at(domain_type)) {
      for (const auto& histogramName : port.second) {
        histogram_tester().ExpectTotalCount(histogramName.second, 0);
      }
    }
    for (const auto& resource :
         internal::kNonlocalhostHistogramNames.at(domain_type)) {
      for (const auto& histogramName : resource.second) {
        histogram_tester().ExpectTotalCount(histogramName.second, 0);
      }
    }
  }

  void ExpectHistogramsEmpty(ResourceType resource_type) {
    for (const auto& domain : internal::kNonlocalhostHistogramNames) {
      for (const auto& histogramName : domain.second.at(resource_type)) {
        histogram_tester().ExpectTotalCount(histogramName.second, 0);
      }
    }
  }

  void ExpectHistogramsEmpty(PortType port_type) {
    for (const auto& domain : internal::kLocalhostHistogramNames) {
      for (const auto& histogramName : domain.second.at(port_type)) {
        histogram_tester().ExpectTotalCount(histogramName.second, 0);
      }
    }
  }

  void ExpectHistogramsEmpty(bool status) {
    for (const auto& domain : internal::kLocalhostHistogramNames) {
      for (const auto& port : domain.second) {
        histogram_tester().ExpectTotalCount(port.second.at(status), 0);
      }
    }
    for (const auto& domain : internal::kNonlocalhostHistogramNames) {
      for (const auto& resource : domain.second) {
        histogram_tester().ExpectTotalCount(resource.second.at(status), 0);
      }
    }
  }

  // Copied from UkmPageLoadMetricsObserverTest

  size_t ukm_source_count() { return test_ukm_recorder_.sources_count(); }

  size_t ukm_entry_count() { return test_ukm_recorder_.entries_count(); }

  const ukm::UkmSource* GetUkmSourceForUrl(const char* url) {
    return test_ukm_recorder_.GetSourceForUrl(url);
  }

  const ukm::mojom::UkmEntry* GetUkmEntry(size_t entry_index) {
    return test_ukm_recorder_.GetEntry(entry_index);
  }

  std::vector<const ukm::mojom::UkmEntry*> GetUkmEntriesForSourceID(
      ukm::SourceId source_id) {
    std::vector<const ukm::mojom::UkmEntry*> entries;
    for (size_t i = 0; i < ukm_entry_count(); ++i) {
      const ukm::mojom::UkmEntry* entry = GetUkmEntry(i);
      if (entry->source_id == source_id)
        entries.push_back(entry);
    }
    return entries;
  }

  static bool HasMetric(const char* name,
                        const ukm::mojom::UkmEntry* entry) WARN_UNUSED_RESULT {
    return ukm::TestUkmRecorder::FindMetric(entry, name) != nullptr;
  }

  static void ExpectMetric(const char* name,
                           int64_t expected_value,
                           const ukm::mojom::UkmEntry* entry) {
    const ukm::mojom::UkmMetric* metric =
        ukm::TestUkmRecorder::FindMetric(entry, name);
    EXPECT_NE(nullptr, metric) << "Failed to find metric: " << name;
    EXPECT_EQ(expected_value, metric->value);
  }

 private:
  ukm::TestUkmRecorder test_ukm_recorder_;
};

TEST_F(LocalNetworkRequestsPageLoadMetricsObserverTest, NoMetrics) {
  EXPECT_EQ(0ul, ukm_source_count());
  EXPECT_EQ(0ul, ukm_entry_count());

  // Sanity check
  ExpectHistogramsEmpty(DOMAIN_TYPE_PUBLIC);
}

TEST_F(LocalNetworkRequestsPageLoadMetricsObserverTest,
       PublicPageIPv6PublicRequests) {
  const internal::PageAddressInfo page = internal::kPublicPageIPv6;
  // Navigate to a public page and make only public resource requests.
  NavigateToPageAndLoadResources(
      page, {internal::kPublicRequest1, internal::kPublicPageIPv6},
      {true, true});

  // Should generate only a domain type UKM entry and nothing else.
  EXPECT_EQ(1ul, ukm_source_count());
  const ukm::UkmSource* source = GetUkmSourceForUrl(page.url);
  EXPECT_EQ(GURL(page.url), source->url());

  EXPECT_EQ(1ul, ukm_entry_count());
  auto* entry = GetUkmEntriesForSourceID(source->id())[0];
  EXPECT_EQ(entry->source_id, source->id());
  EXPECT_EQ(entry->event_hash,
            base::HashMetricName(internal::kUkmPageLoadEventName));
  EXPECT_FALSE(entry->metrics.empty());
  ExpectMetric(internal::kUkmDomainTypeName,
               static_cast<int>(DOMAIN_TYPE_PUBLIC), entry);

  ExpectHistogramsEmpty(true);
}

TEST_F(LocalNetworkRequestsPageLoadMetricsObserverTest,
       PublicPagePublicRequests) {
  const internal::PageAddressInfo page = internal::kPublicPage;
  // Navigate to a public page and make only public resource requests.
  NavigateToPageAndLoadResources(
      page,
      {internal::kPublicRequest1, internal::kPublicRequest2,
       internal::kPublicPageIPv6},
      {true, true, true});

  // Should generate only a domain type UKM entry and nothing else.
  EXPECT_EQ(1ul, ukm_source_count());
  const ukm::UkmSource* source = GetUkmSourceForUrl(page.url);
  EXPECT_EQ(GURL(page.url), source->url());

  EXPECT_EQ(1ul, ukm_entry_count());
  auto* entry = GetUkmEntriesForSourceID(source->id())[0];
  EXPECT_EQ(entry->source_id, source->id());
  EXPECT_EQ(entry->event_hash,
            base::HashMetricName(internal::kUkmPageLoadEventName));
  EXPECT_FALSE(entry->metrics.empty());
  ExpectMetric(internal::kUkmDomainTypeName,
               static_cast<int>(DOMAIN_TYPE_PUBLIC), entry);

  ExpectHistogramsEmpty(true);
}

TEST_F(LocalNetworkRequestsPageLoadMetricsObserverTest,
       PrivatePageSelfRequests) {
  const internal::PageAddressInfo page = internal::kPrivatePage;
  // Navigate to a private page and make resource requests only to the page
  // itself.
  NavigateToPageAndLoadResources(page, {page}, {true});

  // Should generate only a domain type UKM entry and nothing else.
  EXPECT_EQ(1ul, ukm_source_count());
  const ukm::UkmSource* source = GetUkmSourceForUrl(page.url);
  EXPECT_EQ(GURL(page.url), source->url());

  EXPECT_EQ(1ul, ukm_entry_count());
  auto* entry = GetUkmEntriesForSourceID(source->id())[0];
  EXPECT_EQ(entry->source_id, source->id());
  EXPECT_EQ(entry->event_hash,
            base::HashMetricName(internal::kUkmPageLoadEventName));
  EXPECT_FALSE(entry->metrics.empty());
  ExpectMetric(internal::kUkmDomainTypeName,
               static_cast<int>(DOMAIN_TYPE_PRIVATE), entry);

  ExpectHistogramsEmpty(true);
}

TEST_F(LocalNetworkRequestsPageLoadMetricsObserverTest, PrivatePageNoRequests) {
  const internal::PageAddressInfo page = internal::kPrivatePage;
  // Navigate to a private page and make no resource requests.
  NavigateToPageAndLoadResources(page, {{}}, {{}});

  // Should generate only a domain type UKM entry and nothing else.
  EXPECT_EQ(1ul, ukm_source_count());
  const ukm::UkmSource* source = GetUkmSourceForUrl(page.url);
  EXPECT_EQ(GURL(page.url), source->url());

  EXPECT_EQ(1ul, ukm_entry_count());
  auto* entry = GetUkmEntriesForSourceID(source->id())[0];
  EXPECT_EQ(entry->source_id, source->id());
  EXPECT_EQ(entry->event_hash,
            base::HashMetricName(internal::kUkmPageLoadEventName));
  EXPECT_FALSE(entry->metrics.empty());
  ExpectMetric(internal::kUkmDomainTypeName,
               static_cast<int>(DOMAIN_TYPE_PRIVATE), entry);

  ExpectHistogramsEmpty(true);
}

TEST_F(LocalNetworkRequestsPageLoadMetricsObserverTest, LocalhostPage) {
  const internal::PageAddressInfo page = internal::kLocalhostPage;
  // Navigate to a localhost page and make some arbitrary resource requests.
  NavigateToPageAndLoadResources(
      page,
      {internal::kPublicRequest1, internal::kPublicRequest2,
       internal::kSameSubnetRequest, internal::kLocalhostRequest5},
      {true, false, true, true});

  // Should generate no UKM entries or UMA histograms.
  EXPECT_EQ(0ul, ukm_source_count());
  EXPECT_EQ(0ul, ukm_entry_count());

  ExpectHistogramsEmpty(true);
}

TEST_F(LocalNetworkRequestsPageLoadMetricsObserverTest, LocalhostPageIPv6) {
  const internal::PageAddressInfo page = internal::kLocalhostPageIPv6;
  // Navigate to a localhost page with an IPv6 address and make some arbitrary
  // resource requests.
  NavigateToPageAndLoadResources(
      page,
      {internal::kLocalhostRequest1, internal::kLocalhostRequest2,
       internal::kLocalhostRequest3, internal::kLocalhostRequest4},
      {false, true, false, true});

  // Should generate no UKM entries or UMA histograms.
  EXPECT_EQ(0ul, ukm_source_count());
  EXPECT_EQ(0ul, ukm_entry_count());

  ExpectHistogramsEmpty(false);
}

TEST_F(LocalNetworkRequestsPageLoadMetricsObserverTest,
       PublicPageAllSuccessfulRequests) {
  // Navigate to a public page with an IPv6 address.
  SimulateNavigateAndCommit(internal::kPublicPageIPv6);

  // Should generate only a domain type UKM entry by this point.
  EXPECT_EQ(1ul, ukm_source_count());
  const ukm::UkmSource* source =
      GetUkmSourceForUrl(internal::kPublicPageIPv6.url);

  EXPECT_EQ(GURL(internal::kPublicPageIPv6.url), source->url());

  EXPECT_EQ(1ul, ukm_entry_count());
  auto* entry = GetUkmEntriesForSourceID(source->id())[0];
  EXPECT_EQ(entry->source_id, source->id());
  EXPECT_EQ(entry->event_hash,
            base::HashMetricName(internal::kUkmPageLoadEventName));
  EXPECT_FALSE(entry->metrics.empty());
  ExpectMetric(internal::kUkmDomainTypeName,
               static_cast<int>(DOMAIN_TYPE_PUBLIC), entry);

  // Make all of the types of requests, with all being successful.
  std::vector<internal::PageAddressInfo> requests = {
      internal::kPublicRequest1,     internal::kPublicRequest2,
      internal::kSameSubnetRequest,  internal::kDiffSubnetRequest1,
      internal::kDiffSubnetRequest2, internal::kLocalhostRequest1,
      internal::kLocalhostRequest2,  internal::kLocalhostRequest3,
      internal::kLocalhostRequest4,  internal::kLocalhostRequest5,
      internal::kRouterRequest1,     internal::kRouterRequest2};
  for (auto request : requests) {
    SimulateLoadedSuccessfulResource(request);
  }
  DeleteContents();

  // Should now have generated UKM entries for each of the types of resources
  // requested except for public resources.
  EXPECT_EQ(11ul, ukm_entry_count());

  // TODO(uthakore): Finish verification (code below)

  // ExpectHistogramsEmpty(true);
}

TEST_F(LocalNetworkRequestsPageLoadMetricsObserverTest,
       PrivatePageAllSuccessfulRequests) {
  /*
    const internal::PageAddressInfo page = internal::k__X__Page;
    // Navigate to a __X__ page and make __Y__ resource requests.
    NavigateToPageAndLoadResources(page, {}, {});

    // Should now have generated UKM entries for each of the types of resources
    // requested except for __Z__ resources.
    EXPECT_EQ(1ul, ukm_source_count());
    const ukm::UkmSource* source = GetUkmSourceForUrl(page.url);
    EXPECT_EQ(GURL(page.url), source->url());

    struct  // TODO(uthakore): define struct of resource type, port type
            // (optional), success count, failed count
        expected_values;  // list of values, sorted by IP address

    EXPECT_EQ(________, ukm_entry_count());
    auto entries = GetUkmEntriesForSourceID(source->id());
    for (auto* entry : entries) {
      EXPECT_EQ(entry->source_id, source->id());
      EXPECT_EQ(
          entry->event_hash,
          base::HashMetricName(internal::kUkmLocalNetworkRequestsEventName));
      EXPECT_FALSE(entry->metrics.empty());

      std::vector<UkmMetricPtr> metrics = entry->metrics;
      EXPECT_EQ(metrics[0]->metric_hash,
                base::HashMetricName(internal::kUkmResourceTypeName));
      if (metrics[0]->value == static_cast<int>(RESOURCE_TYPE_LOCALHOST)) {
        ExpectMetric(internal::kUkmPortTypeName, expected_values___________,
                     entry);
        ExpectMetric(internal::kUkmSuccessfulCountName,
                     expected_values___________, entry);
        histogram_tester().ExpectTotalCount(____________, ____);
        ExpectMetric(internal::kUkmFailedCountName, expected_values___________,
                     entry);
        histogram_tester().ExpectTotalCount(____________, ____);
      } else {
        EXPECT_EQ(metrics.size(), 3);
        auto resource_type = static_cast<ResourceType>(metrics[0]->value);
        ExpectMetric(internal::kUkmSuccessfulCountName,
                     expected_values___________, entry);
        histogram_tester().ExpectTotalCount(____________, ____);
        ExpectMetric(internal::kUkmFailedCountName, expected_values___________,
                     entry);
        histogram_tester().ExpectTotalCount(____________, ____);
      }
    }

    // ExpectHistogramsEmpty(true);
  */
}

TEST_F(LocalNetworkRequestsPageLoadMetricsObserverTest,
       PrivatePageAllFailedRequests) {
  /*  const internal::PageAddressInfo page = internal::k__X__Page;
    // Navigate to a __X__ page and make __Y__ resource requests.
    NavigateToPageAndLoadResources(page, {}, {});

    // Should now have generated UKM entries for each of the types of resources
    // requested except for __Z__ resources.
    EXPECT_EQ(1ul, ukm_source_count());
    const ukm::UkmSource* source = GetUkmSourceForUrl(page.url);
    EXPECT_EQ(GURL(page.url), source->url());

    std::map<ResourceType, std::pair<uint32_t, uint32_t>>
        expected_resource_values = {};
    std::map<PortType, std::pair<uint32_t, uint32_t>> expected_port_values = {};

    EXPECT_EQ(________, ukm_entry_count());
    auto entries = GetUkmEntriesForSourceID(source->id());
    for (auto* entry : entries) {
      EXPECT_EQ(entry->source_id, source->id());
      EXPECT_EQ(
          entry->event_hash,
          base::HashMetricName(internal::kUkmLocalNetworkRequestsEventName));
      EXPECT_FALSE(entry->metrics.empty());

      std::vector<UkmMetricPtr> metrics = entry->metrics;
      EXPECT_EQ(metrics[0]->metric_hash,
                base::HashMetricName(internal::kUkmResourceTypeName));
      if (metrics[0]->value == static_cast<int>(RESOURCE_TYPE_LOCALHOST)) {
        EXPECT_EQ(metrics.size(), 4);
        EXPECT_EQ(metrics[1]->metric_hash,
                  base::HashMetricName(internal::kUkmPortTypeName));
        auto port_type = static_cast<PortType>(metrics[1]->value);

        metrics[2];  // should be Successful Count
        metrics[3];  // should be Failed Count
      } else {
        EXPECT_EQ(metrics.size(), 3);
        auto resource_type = static_cast<ResourceType>(metrics[0]->value);
        ExpectMetric(internal::kUkmSuccessfulCountName,
    expected_resource_values[resource_type].first, entry);
        ExpectMetric(internal::kUkmFailedCountName,
    expected_resource_values[resource_type].second, entry);

        metrics[1];  // should be Successful Count
        metrics[2];  // should be Failed Count
      }
      histogram_tester().ExpectTotalCount(port.second.at(status), 0);
    }
  */
}

TEST_F(LocalNetworkRequestsPageLoadMetricsObserverTest,
       PublicPageMixedStatusRequests) {}

TEST_F(LocalNetworkRequestsPageLoadMetricsObserverTest,
       PublicPageRequestIpInUrlAndSocketAddress) {}

TEST_F(LocalNetworkRequestsPageLoadMetricsObserverTest,
       PublicPageRequestIpInUrlOnly) {}

TEST_F(LocalNetworkRequestsPageLoadMetricsObserverTest,
       PublicPageRequestIpNotPresent) {}

TEST_F(LocalNetworkRequestsPageLoadMetricsObserverTest, PublicPageNoCommit) {}

TEST_F(LocalNetworkRequestsPageLoadMetricsObserverTest, PrivatePageFailedLoad) {
  std::vector<internal::PageAddressInfo> temp = {
      internal::kPublicPage,         internal::kPublicPageIPv6,
      internal::kPrivatePage,        internal::kLocalhostPage,
      internal::kLocalhostPageIPv6,  internal::kPublicRequest1,
      internal::kPublicRequest2,     internal::kSameSubnetRequest,
      internal::kDiffSubnetRequest1, internal::kDiffSubnetRequest2,
      internal::kLocalhostRequest1,  internal::kLocalhostRequest2,
      internal::kLocalhostRequest3,  internal::kLocalhostRequest4,
      internal::kLocalhostRequest5,  internal::kRouterRequest1,
      internal::kRouterRequest2};
  EXPECT_EQ(temp.size(), 17u);
}

/*
TEST_F(UkmPageLoadMetricsObserverTest, Basic) {
  // PageLoadTiming with all recorded metrics other than FMP. This allows us to
  // verify both that all metrics are logged, and that we don't log metrics that
  // aren't present in the PageLoadTiming struct. Logging of FMP is verified in
  // the FirstMeaningfulPaint test below.
  page_load_metrics::mojom::PageLoadTiming timing;
  page_load_metrics::InitPageLoadTimingForTest(&timing);
  timing.navigation_start = base::Time::FromDoubleT(1);
  timing.parse_timing->parse_start = base::TimeDelta::FromMilliseconds(100);
  timing.document_timing->dom_content_loaded_event_start =
      base::TimeDelta::FromMilliseconds(200);
  timing.paint_timing->first_paint = base::TimeDelta::FromMilliseconds(250);
  timing.paint_timing->first_contentful_paint =
      base::TimeDelta::FromMilliseconds(300);
  timing.document_timing->load_event_start =
      base::TimeDelta::FromMilliseconds(500);
  PopulateRequiredTimingFields(&timing);

  NavigateAndCommit(GURL(kTestUrl1));
  SimulateTimingUpdate(timing);

  // Simulate closing the tab.
  DeleteContents();

  EXPECT_EQ(1ul, ukm_source_count());
  const ukm::UkmSource* source = GetUkmSourceForUrl(kTestUrl1);
  EXPECT_EQ(GURL(kTestUrl1), source->url());

  EXPECT_GE(ukm_entry_count(), 1ul);
  ukm::mojom::UkmEntryPtr entry = GetMergedEntryForSourceID(source->id());
  EXPECT_EQ(entry->source_id, source->id());
  EXPECT_EQ(entry->event_hash,
            base::HashMetricName(internal::kUkmPageLoadEventName));
  EXPECT_FALSE(entry->metrics.empty());
  ExpectMetric(internal::kUkmPageTransition, ui::PAGE_TRANSITION_LINK,
               entry.get());
  ExpectMetric(internal::kUkmParseStartName, 100, entry.get());
  ExpectMetric(internal::kUkmDomContentLoadedName, 200, entry.get());
  ExpectMetric(internal::kUkmFirstPaintName, 250, entry.get());
  ExpectMetric(internal::kUkmFirstContentfulPaintName, 300, entry.get());
  ExpectMetric(internal::kUkmLoadEventName, 500, entry.get());
  EXPECT_FALSE(HasMetric(internal::kUkmFirstMeaningfulPaintName, entry.get()));
  EXPECT_TRUE(HasMetric(internal::kUkmForegroundDurationName, entry.get()));
}

TEST_F(UkmPageLoadMetricsObserverTest, FailedProvisionalLoad) {
  EXPECT_CALL(mock_network_quality_provider(), GetEffectiveConnectionType())
      .WillRepeatedly(Return(net::EFFECTIVE_CONNECTION_TYPE_2G));

  GURL url(kTestUrl1);
  content::RenderFrameHostTester* rfh_tester =
      content::RenderFrameHostTester::For(main_rfh());
  rfh_tester->SimulateNavigationStart(url);
  rfh_tester->SimulateNavigationError(url, net::ERR_TIMED_OUT);
  rfh_tester->SimulateNavigationStop();

  // Simulate closing the tab.
  DeleteContents();

  EXPECT_EQ(1ul, ukm_source_count());
  const ukm::UkmSource* source = GetUkmSourceForUrl(kTestUrl1);
  EXPECT_EQ(GURL(kTestUrl1), source->url());

  EXPECT_GE(ukm_entry_count(), 1ul);
  ukm::mojom::UkmEntryPtr entry = GetMergedEntryForSourceID(source->id());
  EXPECT_EQ(entry->source_id, source->id());
  EXPECT_EQ(entry->event_hash,
            base::HashMetricName(internal::kUkmPageLoadEventName));

  // Make sure that only the following metrics are logged. In particular, no
  // paint/document/etc timing metrics should be logged for failed provisional
  // loads.
  EXPECT_EQ(5ul, entry->metrics.size());
  ExpectMetric(internal::kUkmPageTransition, ui::PAGE_TRANSITION_LINK,
               entry.get());
  ExpectMetric(internal::kUkmEffectiveConnectionType,
               net::EFFECTIVE_CONNECTION_TYPE_2G, entry.get());
  ExpectMetric(internal::kUkmNetErrorCode,
               static_cast<int64_t>(net::ERR_TIMED_OUT) * -1, entry.get());
  EXPECT_TRUE(HasMetric(internal::kUkmForegroundDurationName, entry.get()));
  EXPECT_TRUE(HasMetric(internal::kUkmFailedProvisionaLoadName, entry.get()));
}

TEST_F(UkmPageLoadMetricsObserverTest, FirstMeaningfulPaint) {
  page_load_metrics::mojom::PageLoadTiming timing;
  page_load_metrics::InitPageLoadTimingForTest(&timing);
  timing.navigation_start = base::Time::FromDoubleT(1);
  timing.paint_timing->first_meaningful_paint =
      base::TimeDelta::FromMilliseconds(600);
  PopulateRequiredTimingFields(&timing);

  NavigateAndCommit(GURL(kTestUrl1));
  SimulateTimingUpdate(timing);

  // Simulate closing the tab.
  DeleteContents();

  EXPECT_EQ(1ul, ukm_source_count());
  const ukm::UkmSource* source = GetUkmSourceForUrl(kTestUrl1);
  EXPECT_EQ(GURL(kTestUrl1), source->url());

  EXPECT_GE(ukm_entry_count(), 1ul);
  ukm::mojom::UkmEntryPtr entry = GetMergedEntryForSourceID(source->id());
  EXPECT_EQ(entry->source_id, source->id());
  EXPECT_EQ(entry->event_hash,
            base::HashMetricName(internal::kUkmPageLoadEventName));
  EXPECT_FALSE(entry->metrics.empty());
  ExpectMetric(internal::kUkmFirstMeaningfulPaintName, 600, entry.get());
  EXPECT_TRUE(HasMetric(internal::kUkmForegroundDurationName, entry.get()));
}

TEST_F(UkmPageLoadMetricsObserverTest, MultiplePageLoads) {
  page_load_metrics::mojom::PageLoadTiming timing1;
  page_load_metrics::InitPageLoadTimingForTest(&timing1);
  timing1.navigation_start = base::Time::FromDoubleT(1);
  timing1.paint_timing->first_contentful_paint =
      base::TimeDelta::FromMilliseconds(200);
  PopulateRequiredTimingFields(&timing1);

  // Second navigation reports no timing metrics.
  page_load_metrics::mojom::PageLoadTiming timing2;
  page_load_metrics::InitPageLoadTimingForTest(&timing2);
  timing2.navigation_start = base::Time::FromDoubleT(1);
  PopulateRequiredTimingFields(&timing2);

  NavigateAndCommit(GURL(kTestUrl1));
  SimulateTimingUpdate(timing1);

  NavigateAndCommit(GURL(kTestUrl2));
  SimulateTimingUpdate(timing2);

  // Simulate closing the tab.
  DeleteContents();

  EXPECT_EQ(2ul, ukm_source_count());
  const ukm::UkmSource* source1 = GetUkmSourceForUrl(kTestUrl1);
  const ukm::UkmSource* source2 = GetUkmSourceForUrl(kTestUrl2);
  EXPECT_EQ(GURL(kTestUrl1), source1->url());
  EXPECT_EQ(GURL(kTestUrl2), source2->url());
  EXPECT_NE(source1->id(), source2->id());

  EXPECT_GE(ukm_entry_count(), 2ul);
  ukm::mojom::UkmEntryPtr entry1 = GetMergedEntryForSourceID(source1->id());
  ukm::mojom::UkmEntryPtr entry2 = GetMergedEntryForSourceID(source2->id());
  EXPECT_NE(entry1->source_id, entry2->source_id);

  EXPECT_EQ(entry1->source_id, source1->id());
  EXPECT_EQ(entry1->event_hash,
            base::HashMetricName(internal::kUkmPageLoadEventName));
  EXPECT_FALSE(entry1->metrics.empty());
  ExpectMetric(internal::kUkmFirstContentfulPaintName, 200, entry1.get());
  EXPECT_FALSE(HasMetric(internal::kUkmFirstMeaningfulPaintName, entry2.get()));
  EXPECT_TRUE(HasMetric(internal::kUkmForegroundDurationName, entry1.get()));

  EXPECT_EQ(entry2->source_id, source2->id());
  EXPECT_EQ(entry2->event_hash,
            base::HashMetricName(internal::kUkmPageLoadEventName));
  EXPECT_FALSE(entry2->metrics.empty());
  EXPECT_FALSE(HasMetric(internal::kUkmParseStartName, entry2.get()));
  EXPECT_FALSE(HasMetric(internal::kUkmFirstContentfulPaintName, entry2.get()));
  EXPECT_FALSE(HasMetric(internal::kUkmFirstMeaningfulPaintName, entry2.get()));
  EXPECT_TRUE(HasMetric(internal::kUkmForegroundDurationName, entry2.get()));
}

TEST_F(UkmPageLoadMetricsObserverTest, NetworkQualityEstimates) {
  EXPECT_CALL(mock_network_quality_provider(), GetEffectiveConnectionType())
      .WillRepeatedly(Return(net::EFFECTIVE_CONNECTION_TYPE_3G));
  EXPECT_CALL(mock_network_quality_provider(), GetHttpRTT())
      .WillRepeatedly(Return(base::TimeDelta::FromMilliseconds(100)));
  EXPECT_CALL(mock_network_quality_provider(), GetTransportRTT())
      .WillRepeatedly(Return(base::TimeDelta::FromMilliseconds(200)));

  NavigateAndCommit(GURL(kTestUrl1));

  // Simulate closing the tab.
  DeleteContents();

  EXPECT_EQ(1ul, ukm_source_count());
  const ukm::UkmSource* source = GetUkmSourceForUrl(kTestUrl1);
  EXPECT_EQ(GURL(kTestUrl1), source->url());

  EXPECT_GE(ukm_entry_count(), 1ul);
  ukm::mojom::UkmEntryPtr entry = GetMergedEntryForSourceID(source->id());
  EXPECT_EQ(entry->source_id, source->id());
  EXPECT_EQ(entry->event_hash,
            base::HashMetricName(internal::kUkmPageLoadEventName));
  EXPECT_FALSE(entry->metrics.empty());
  ExpectMetric(internal::kUkmEffectiveConnectionType,
               net::EFFECTIVE_CONNECTION_TYPE_3G, entry.get());
  ExpectMetric(internal::kUkmHttpRttEstimate, 100, entry.get());
  ExpectMetric(internal::kUkmTransportRttEstimate, 200, entry.get());
}

TEST_F(UkmPageLoadMetricsObserverTest, PageTransitionReload) {
  GURL url(kTestUrl1);
  NavigateWithPageTransitionAndCommit(GURL(kTestUrl1),
                                      ui::PAGE_TRANSITION_RELOAD);

  // Simulate closing the tab.
  DeleteContents();

  EXPECT_EQ(1ul, ukm_source_count());
  const ukm::UkmSource* source = GetUkmSourceForUrl(kTestUrl1);
  EXPECT_EQ(GURL(kTestUrl1), source->url());

  EXPECT_GE(ukm_entry_count(), 1ul);
  ukm::mojom::UkmEntryPtr entry = GetMergedEntryForSourceID(source->id());
  EXPECT_EQ(entry->source_id, source->id());
  EXPECT_EQ(entry->event_hash,
            base::HashMetricName(internal::kUkmPageLoadEventName));
  EXPECT_FALSE(entry->metrics.empty());
  ExpectMetric(internal::kUkmPageTransition, ui::PAGE_TRANSITION_RELOAD,
               entry.get());
}


page_load_metrics::PageLoadMetricsObserver::ObservePolicy
LocalNetworkRequestsPageLoadMetricsObserver::OnCommit(
    content::NavigationHandle* navigation_handle,
    ukm::SourceId source_id) {
  // Upon page load, we want to determine whether the page loaded was a public
  // domain or private domain and generate an event describing the domain type.
  net::HostPortPair address = navigation_handle->GetSocketAddress();
  if (net::IsLocalhost(address.host())) {
    page_load_type_ = DOMAIN_TYPE_LOCALHOST;
    page_ip_address_ = net::IPAddress::IPv4Localhost();
  } else {
    DCHECK(net::ParseURLHostnameToAddress(address.host(), &page_ip_address_));

    // In cases where the page loaded was not a network resource (e.g.,
    // extensions), we don't want to track the page load. Such resources will
    // return an empty IP address.
    if (page_ip_address_.IsZero()) {
      return STOP_OBSERVING;
    }

    if (page_ip_address_.IsReserved()) {
      page_load_type_ = DOMAIN_TYPE_PRIVATE;
      static const uint8_t kReservedIPv4Prefixes[][2] = {
        {10, 8}, {100, 10}, {169, 16}, {172, 12}, {192, 16}, {198, 15}};

      for (const auto& entry : kReservedIPv4Prefixes) {
        if (page_ip_address_.bytes()[0] == entry[0]) {
          page_ip_prefix_length_ = entry[1];
        }
      }

    } else {
      page_load_type_ = DOMAIN_TYPE_PUBLIC;
    }
  }

  RecordUkmDomainType(source_id);

  // If the load was localhost, we don't track it because it isn't
  // meaningful for our purposes.
  if (page_load_type_ == DOMAIN_TYPE_LOCALHOST) {
    return STOP_OBSERVING;
  }
  return CONTINUE_OBSERVING;
}

page_load_metrics::PageLoadMetricsObserver::ObservePolicy
LocalNetworkRequestsPageLoadMetricsObserver::FlushMetricsOnAppEnterBackground(
    const page_load_metrics::mojom::PageLoadTiming& timing,
    const page_load_metrics::PageLoadExtraInfo& extra_info) {
  // The browser may come back, but there is no guarantee. To be safe, we record
  // what we have now and treat changes to this navigation as new page loads.
  if (extra_info.did_commit) {
    RecordHistograms();
    RecordUkmMetrics(extra_info.source_id);
    ClearLocalState();
  }

  return CONTINUE_OBSERVING;
}

void LocalNetworkRequestsPageLoadMetricsObserver::OnLoadedResource(
    const page_load_metrics::ExtraRequestCompleteInfo& extra_request_info) {
  ProcessLoadedResource(extra_request_info);
}

void LocalNetworkRequestsPageLoadMetricsObserver::OnComplete(
    const page_load_metrics::mojom::PageLoadTiming& timing,
    const page_load_metrics::PageLoadExtraInfo& info) {
  if (info.did_commit) {
    RecordHistograms();
    RecordUkmMetrics(info.source_id);
  }
}

void LocalNetworkRequestsPageLoadMetricsObserver::ProcessLoadedResource(
    const page_load_metrics::ExtraRequestCompleteInfo& extra_request_info) {
  // If the resource request was successful, then the IP address should be in
  // |extra_request_info|.
  net::IPAddress resource_ip;
  bool ip_exists = net::ParseURLHostnameToAddress(
      extra_request_info.host_port_pair.host(), &resource_ip);
  int resource_port = extra_request_info.host_port_pair.port();

  // If the request failed, it's possible we didn't receive the IP address,
  // possibly because domain resolution failed. As a backup, try getting the IP
  // from the URL. If none was returned, try matching the hostname from the URL
  // itself as it might be an IP address if it is a local network request, which
  // is what we care about.
  if (!ip_exists && extra_request_info.url.is_valid()) {
    if (net::IsLocalhost(extra_request_info.url.host())) {
      resource_ip = net::IPAddress::IPv4Localhost();
      ip_exists = true;
    } else {
      ip_exists = net::ParseURLHostnameToAddress(extra_request_info.url.host(),
                                                 &resource_ip);
      resource_port = extra_request_info.url.EffectiveIntPort();
    }
  }

  // We can't track anything if we don't have an IP address for the resource.
  // We also don't want to track any requests to the page's IP address itself.
  if (resource_ip.IsZero() || resource_ip == page_ip_address_) {
    return;
  }

  // We monitor localhost resource requests for both public and private page
  // loads.
  if (resource_ip == net::IPAddress::IPv4Localhost()) {
    if (extra_request_info.net_error) {
      localhost_request_counts_[resource_port].second++;
    } else {
      localhost_request_counts_[resource_port].first++;
    }
  }
  // We only track public resource requests for private pages.
  else if (resource_ip.IsReserved() || page_load_type_ == DOMAIN_TYPE_PRIVATE) {
    if (extra_request_info.net_error) {
      resource_request_counts_[resource_ip].second++;
    } else {
      resource_request_counts_[resource_ip].first++;
    }
  }
}

void LocalNetworkRequestsPageLoadMetricsObserver::ClearLocalState() {
  localhost_request_counts_.clear();
  resource_request_counts_.clear();
  requested_resource_types_.reset();
}

void LocalNetworkRequestsPageLoadMetricsObserver::RecordHistograms() {
  if (page_load_type_ == DOMAIN_TYPE_LOCALHOST) {
    return;
  }
  ResolveResourceTypes();

  // For each resource, log a histogram for all associated type of request.
  for (const auto& entry : resource_request_counts_) {
    UMA_HISTOGRAM_COUNTS_1000(
        internal::kNonlocalhostHistogramNames.at(page_load_type_)
            .at(requested_resource_types_->at(entry.first))
            .at(true),
        entry.second.first);
    UMA_HISTOGRAM_COUNTS_1000(
        internal::kNonlocalhostHistogramNames.at(page_load_type_)
            .at(requested_resource_types_->at(entry.first))
            .at(false),
        entry.second.second);
  }

  // Log an entry for each localhost resource (one per port).
  for (const auto& entry : localhost_request_counts_) {
    UMA_HISTOGRAM_COUNTS_1000(
        internal::kLocalhostHistogramNames.at(page_load_type_)
            .at(DeterminePortType(entry.first))
            .at(true),
        entry.second.first);
    UMA_HISTOGRAM_COUNTS_1000(
        internal::kLocalhostHistogramNames.at(page_load_type_)
            .at(DeterminePortType(entry.first))
            .at(false),
        entry.second.second);
  }
}

ResourceType LocalNetworkRequestsPageLoadMetricsObserver::DetermineResourceType(
    net::IPAddress resource_ip) {
  if (page_load_type_ == DOMAIN_TYPE_PUBLIC) {
    DCHECK(resource_ip.IsReserved());
    if (IsLikelyRouterIP(resource_ip)) {
      return RESOURCE_TYPE_ROUTER;
    } else {
      return RESOURCE_TYPE_PRIVATE;
    }
  } else {
    if (resource_ip.IsReserved()) {  // PRIVATE
      if (net::CommonPrefixLength(page_ip_address_, resource_ip) >=
          page_ip_prefix_length_) {
        return RESOURCE_TYPE_LOCAL_SAME_SUBNET;
      }
      return RESOURCE_TYPE_LOCAL_DIFF_SUBNET;
    } else {  // PUBLIC
      return RESOURCE_TYPE_PUBLIC;
    }
  }
}

PortType LocalNetworkRequestsPageLoadMetricsObserver::DeterminePortType(
    int port) {
  auto lookup = localhost_port_categories.find(port);
  if (lookup == localhost_port_categories.end()) {
    return PORT_TYPE_OTHER;
  } else {
    return lookup->second;
  }
}

void LocalNetworkRequestsPageLoadMetricsObserver::ResolveResourceTypes() {
  // Lazy instantiation.
  if (requested_resource_types_) {
    return;
  }

  requested_resource_types_ =
      base::MakeUnique<std::map<net::IPAddress, ResourceType>>();
  for (const auto& entry : resource_request_counts_) {
    requested_resource_types_->insert(
        {entry.first, DetermineResourceType(entry.first)});
  }
}

void LocalNetworkRequestsPageLoadMetricsObserver::RecordUkmMetrics(
    ukm::SourceId source_id) {
  if (page_load_type_ == DOMAIN_TYPE_LOCALHOST) {
    return;
  }
  ResolveResourceTypes();

  // Log an entry for each non-localhost resource (one per IP address).
  for (const auto& entry : resource_request_counts_) {
    ukm::UkmRecorder* ukm_recorder = g_browser_process->ukm_recorder();
    if (!ukm_recorder) {
      break;
    }
    std::unique_ptr<ukm::UkmEntryBuilder> builder =
        ukm_recorder->GetEntryBuilder(
            source_id, internal::kUkmLocalNetworkRequestsEventName);
    builder->AddMetric(
        internal::kUkmResourceTypeName,
        static_cast<int>(requested_resource_types_->at(entry.first)));
    builder->AddMetric(internal::kUkmSuccessfulCountName, entry.second.first);
    builder->AddMetric(internal::kUkmFailedCountName, entry.second.second);
  }

  // Log an entry for each localhost resource (one per port).
  for (const auto& entry : localhost_request_counts_) {
    ukm::UkmRecorder* ukm_recorder = g_browser_process->ukm_recorder();
    if (!ukm_recorder) {
      break;
    }
    std::unique_ptr<ukm::UkmEntryBuilder> builder =
        ukm_recorder->GetEntryBuilder(
            source_id, internal::kUkmLocalNetworkRequestsEventName);
    builder->AddMetric(internal::kUkmResourceTypeName,
                       static_cast<int>(RESOURCE_TYPE_LOCALHOST));
    builder->AddMetric(internal::kUkmPortTypeName,
                       static_cast<int>(DeterminePortType(entry.first)));
    builder->AddMetric(internal::kUkmSuccessfulCountName, entry.second.first);
    builder->AddMetric(internal::kUkmFailedCountName, entry.second.second);
  }
}

void LocalNetworkRequestsPageLoadMetricsObserver::RecordUkmDomainType(
    ukm::SourceId source_id) {
  ukm::UkmRecorder* ukm_recorder = g_browser_process->ukm_recorder();
  if (!ukm_recorder) {
    return;
  }

  std::unique_ptr<ukm::UkmEntryBuilder> builder =
      ukm_recorder->GetEntryBuilder(source_id, internal::kUkmPageLoadEventName);
  builder->AddMetric(internal::kUkmDomainTypeName,
                     static_cast<int>(page_load_type_));
}*/