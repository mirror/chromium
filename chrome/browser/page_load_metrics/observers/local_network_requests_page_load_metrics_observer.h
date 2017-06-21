// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_PAGE_LOAD_METRICS_OBSERVERS_LOCAL_NETWORK_REQUESTS_PAGE_LOAD_METRICS_OBSERVER_H_
#define CHROME_BROWSER_PAGE_LOAD_METRICS_OBSERVERS_LOCAL_NETWORK_REQUESTS_PAGE_LOAD_METRICS_OBSERVER_H_

#include <list>
#include <map>
#include <memory>

#include "base/lazy_instance.h"
#include "base/macros.h"
#include "chrome/browser/page_load_metrics/page_load_metrics_observer.h"
#include "components/ukm/public/ukm_recorder.h"
#include "net/base/ip_address.h"

using SuccessFailCounts = std::pair<uint32_t, uint32_t>;

namespace {

// The domain type of the IP address of the loaded page. We use these to
// determine what classes of resource request metrics to collect.
enum DomainType {
  DOMAIN_TYPE_UNKNOWN = 0,
  DOMAIN_TYPE_PUBLIC = 1,
  DOMAIN_TYPE_PRIVATE = 2,
  DOMAIN_TYPE_LOCALHOST = 4,
};

// The type of the IP address of the loaded resource.
enum ResourceType {
  RESOURCE_TYPE_PUBLIC = 0,
  RESOURCE_TYPE_PRIVATE = 1,
  RESOURCE_TYPE_LOCAL_SAME_SUBNET = 2,
  RESOURCE_TYPE_LOCAL_DIFF_SUBNET = 4,
  RESOURCE_TYPE_ROUTER = 8,
  RESOURCE_TYPE_LOCALHOST = 16,
};

// The types of services to distinguish between when collecting local network
// request metrics.
enum PortType {
  PORT_TYPE_WEB = 1,
  PORT_TYPE_DB = 2,
  PORT_TYPE_PRINT = 4,
  PORT_TYPE_DEV = 8,
  PORT_TYPE_OTHER = 0,
};

}  // namespace

namespace internal {

// UKM event names
const char kUkmPageLoadEventName[] = "PageDomain";
const char kUkmLocalNetworkRequestsEventName[] = "LocalNetworkRequests";

// UKM metric names
const char kUkmDomainTypeName[] = "DomainType";
const char kUkmResourceTypeName[] = "ResourceType";
const char kUkmPortTypeName[] = "PortType";
const char kUkmSuccessfulCountName[] = "Count.Successful";
const char kUkmFailedCountName[] = "Count.Failed";

// UMA public page histogram names
static const std::map<DomainType,
                      std::map<ResourceType, std::map<bool, std::string>>>&
GetNonlocalhostHistogramNames() {
  static base::LazyInstance<std::map<
      DomainType, std::map<ResourceType, std::map<bool, std::string>>>>::Leaky
      kNonlocalhostHistogramNames = LAZY_INSTANCE_INITIALIZER;

  kNonlocalhostHistogramNames
      .Get()[DOMAIN_TYPE_PUBLIC][RESOURCE_TYPE_PRIVATE][true] =
      "LocalNetworkRequests.PublicPage.PrivateRequestCount.Successful";
  kNonlocalhostHistogramNames
      .Get()[DOMAIN_TYPE_PUBLIC][RESOURCE_TYPE_PRIVATE][false] =
      "LocalNetworkRequests.PublicPage.PrivateRequestCount.Failed";
  kNonlocalhostHistogramNames
      .Get()[DOMAIN_TYPE_PUBLIC][RESOURCE_TYPE_ROUTER][true] =
      "LocalNetworkRequests.PublicPage.RouterRequestCount.Successful";
  kNonlocalhostHistogramNames
      .Get()[DOMAIN_TYPE_PUBLIC][RESOURCE_TYPE_ROUTER][false] =
      "LocalNetworkRequests.PublicPage.RouterRequestCount.Failed";

  kNonlocalhostHistogramNames
      .Get()[DOMAIN_TYPE_PRIVATE][RESOURCE_TYPE_PUBLIC][true] =
      "LocalNetworkRequests.PrivatePage.PublicRequestCount.Successful";
  kNonlocalhostHistogramNames
      .Get()[DOMAIN_TYPE_PRIVATE][RESOURCE_TYPE_PUBLIC][false] =
      "LocalNetworkRequests.PrivatePage.PublicRequestCount.Failed";
  kNonlocalhostHistogramNames
      .Get()[DOMAIN_TYPE_PRIVATE][RESOURCE_TYPE_LOCAL_SAME_SUBNET][true] =
      "LocalNetworkRequests.PrivatePage.SameSubnetRequestCount.Successful";
  kNonlocalhostHistogramNames
      .Get()[DOMAIN_TYPE_PRIVATE][RESOURCE_TYPE_LOCAL_SAME_SUBNET][false] =
      "LocalNetworkRequests.PrivatePage.SameSubnetRequestCount.Failed";
  kNonlocalhostHistogramNames
      .Get()[DOMAIN_TYPE_PRIVATE][RESOURCE_TYPE_LOCAL_DIFF_SUBNET][true] =
      "LocalNetworkRequests.PrivatePage."
      "DifferentSubnetRequestCount.Successful";
  kNonlocalhostHistogramNames
      .Get()[DOMAIN_TYPE_PRIVATE][RESOURCE_TYPE_LOCAL_DIFF_SUBNET][false] =
      "LocalNetworkRequests.PrivatePage.DifferentSubnetRequestCount.Failed";

  return kNonlocalhostHistogramNames.Get();
}

static const std::map<DomainType,
                      std::map<PortType, std::map<bool, std::string>>>&
GetLocalhostHistogramNames() {
  static base::LazyInstance<std::map<
      DomainType, std::map<PortType, std::map<bool, std::string>>>>::Leaky
      kLocalhostHistogramNames = LAZY_INSTANCE_INITIALIZER;

  kLocalhostHistogramNames.Get()[DOMAIN_TYPE_PUBLIC][PORT_TYPE_WEB][true] =
      "LocalNetworkRequests.PublicPage.Localhost."
      "WebRequestCount.Successful";
  kLocalhostHistogramNames.Get()[DOMAIN_TYPE_PUBLIC][PORT_TYPE_WEB][false] =
      "LocalNetworkRequests.PublicPage.Localhost."
      "WebRequestCount.Failed";
  kLocalhostHistogramNames.Get()[DOMAIN_TYPE_PUBLIC][PORT_TYPE_DB][true] =
      "LocalNetworkRequests.PublicPage.Localhost."
      "DatabaseRequestCount.Successful";
  kLocalhostHistogramNames.Get()[DOMAIN_TYPE_PUBLIC][PORT_TYPE_DB][false] =
      "LocalNetworkRequests.PublicPage.Localhost."
      "DatabaseRequestCount.Failed";
  kLocalhostHistogramNames.Get()[DOMAIN_TYPE_PUBLIC][PORT_TYPE_PRINT][true] =
      "LocalNetworkRequests.PublicPage.Localhost."
      "PrinterRequestCount.Successful";
  kLocalhostHistogramNames.Get()[DOMAIN_TYPE_PUBLIC][PORT_TYPE_PRINT][false] =
      "LocalNetworkRequests.PublicPage.Localhost."
      "PrinterRequestCount.Failed";
  kLocalhostHistogramNames.Get()[DOMAIN_TYPE_PUBLIC][PORT_TYPE_DEV][true] =
      "LocalNetworkRequests.PublicPage.Localhost."
      "DevelopmentRequestCount.Successful";
  kLocalhostHistogramNames.Get()[DOMAIN_TYPE_PUBLIC][PORT_TYPE_DEV][false] =
      "LocalNetworkRequests.PublicPage.Localhost."
      "DevelopmentRequestCount.Failed";
  kLocalhostHistogramNames.Get()[DOMAIN_TYPE_PUBLIC][PORT_TYPE_OTHER][true] =
      "LocalNetworkRequests.PublicPage.Localhost."
      "OtherRequestCount.Successful";
  kLocalhostHistogramNames.Get()[DOMAIN_TYPE_PUBLIC][PORT_TYPE_OTHER][false] =
      "LocalNetworkRequests.PublicPage.Localhost."
      "OtherRequestCount.Failed";

  kLocalhostHistogramNames.Get()[DOMAIN_TYPE_PRIVATE][PORT_TYPE_WEB][true] =
      "LocalNetworkRequests.PrivatePage.Localhost."
      "WebRequestCount.Successful";
  kLocalhostHistogramNames.Get()[DOMAIN_TYPE_PRIVATE][PORT_TYPE_WEB][false] =
      "LocalNetworkRequests.PrivatePage.Localhost."
      "WebRequestCount.Failed";
  kLocalhostHistogramNames.Get()[DOMAIN_TYPE_PRIVATE][PORT_TYPE_DB][true] =
      "LocalNetworkRequests.PrivatePage.Localhost."
      "DatabaseRequestCount.Successful";
  kLocalhostHistogramNames.Get()[DOMAIN_TYPE_PRIVATE][PORT_TYPE_DB][false] =
      "LocalNetworkRequests.PrivatePage.Localhost."
      "DatabaseRequestCount.Failed";
  kLocalhostHistogramNames.Get()[DOMAIN_TYPE_PRIVATE][PORT_TYPE_PRINT][true] =
      "LocalNetworkRequests.PrivatePage.Localhost."
      "PrinterRequestCount.Successful";
  kLocalhostHistogramNames.Get()[DOMAIN_TYPE_PRIVATE][PORT_TYPE_PRINT][false] =
      "LocalNetworkRequests.PrivatePage.Localhost."
      "PrinterRequestCount.Failed";
  kLocalhostHistogramNames.Get()[DOMAIN_TYPE_PRIVATE][PORT_TYPE_DEV][true] =
      "LocalNetworkRequests.PrivatePage.Localhost."
      "DevelopmentRequestCount.Successful";
  kLocalhostHistogramNames.Get()[DOMAIN_TYPE_PRIVATE][PORT_TYPE_DEV][false] =
      "LocalNetworkRequests.PrivatePage.Localhost."
      "DevelopmentRequestCount.Failed";
  kLocalhostHistogramNames.Get()[DOMAIN_TYPE_PRIVATE][PORT_TYPE_OTHER][true] =
      "LocalNetworkRequests.PrivatePage.Localhost."
      "OtherRequestCount.Successful";
  kLocalhostHistogramNames.Get()[DOMAIN_TYPE_PRIVATE][PORT_TYPE_OTHER][false] =
      "LocalNetworkRequests.PrivatePage.Localhost."
      "OtherRequestCount.Failed";
  return kLocalhostHistogramNames.Get();
}

// For simple access during UMA histogram logging, the names are in a
// multidimensional map indexed by [DomainType][ResourceType][Status].
static const std::map<DomainType,
                      std::map<ResourceType, std::map<bool, std::string>>>&
    kNonlocalhostHistogramNames = GetNonlocalhostHistogramNames();
// For localhost histogram names, the map is indexed by
// [DomainType][PortType][Status].
static const std::map<DomainType,
                      std::map<PortType, std::map<bool, std::string>>>&
    kLocalhostHistogramNames = GetLocalhostHistogramNames();

}  // namespace internal

// This observer is for observing local network requests.
// TODO(uthakore): Add description.
class LocalNetworkRequestsPageLoadMetricsObserver
    : public page_load_metrics::PageLoadMetricsObserver {
 public:
  LocalNetworkRequestsPageLoadMetricsObserver();
  ~LocalNetworkRequestsPageLoadMetricsObserver() override;

  // page_load_metrics::PageLoadMetricsObserver
  ObservePolicy OnCommit(content::NavigationHandle* navigation_handle,
                         ukm::SourceId source_id) override;
  ObservePolicy FlushMetricsOnAppEnterBackground(
      const page_load_metrics::mojom::PageLoadTiming& timing,
      const page_load_metrics::PageLoadExtraInfo& extra_info) override;
  void OnLoadedResource(const page_load_metrics::ExtraRequestCompleteInfo&
                            extra_request_info) override;
  void OnComplete(const page_load_metrics::mojom::PageLoadTiming& timing,
                  const page_load_metrics::PageLoadExtraInfo& info) override;

 private:
  // Stores all requisite information for each resource request that is made by
  // the page.
  void ProcessLoadedResource(
      const page_load_metrics::ExtraRequestCompleteInfo& extra_request_info);

  // Clears all local resource request counts. Only used if we decide to log
  // metrics but the observer may stay in scope and capture additional resource
  // requests.
  void ClearLocalState();
  // Determines the resource type for the |ip_address| based on the page load
  // type.
  ResourceType DetermineResourceType(net::IPAddress ip_address);
  // Determines the port type for the localhost |port|.
  PortType DeterminePortType(int port);
  // Resolves the resource types to report for all IP addresses in
  // |resource_request_counts_|.
  void ResolveResourceTypes();

  void RecordUkmDomainType(ukm::SourceId source_id);
  void RecordHistograms();
  void RecordUkmMetrics(ukm::SourceId source_id);

  // Stores the counts of resource requests for each non-localhost IP address as
  // pairs of (successful, failed) request counts.
  std::map<net::IPAddress, SuccessFailCounts> resource_request_counts_;

  std::unique_ptr<std::map<net::IPAddress, ResourceType>>
      requested_resource_types_ = nullptr;

  // Stores the counts of resource requests for each localhost port as
  // pairs of (successful, failed) request counts.
  std::map<int, SuccessFailCounts> localhost_request_counts_;

  // The page load type. This is used to determine what resource requests to
  // monitor while the page is committed and to determine the UMA histogram name
  // to use.
  DomainType page_load_type_ = DOMAIN_TYPE_UNKNOWN;

  // The IP address of the page that was loaded.
  net::IPAddress page_ip_address_;

  // For private page loads, the IP prefix defining the largest reserved subnet
  // the page could belong to. Used to distinguish between same subnet and
  // different subnet private network queries.
  size_t page_ip_prefix_length_ = 0;

  DISALLOW_COPY_AND_ASSIGN(LocalNetworkRequestsPageLoadMetricsObserver);
};

#endif  // CHROME_BROWSER_PAGE_LOAD_METRICS_OBSERVERS_LOCAL_NETWORK_REQUESTS_PAGE_LOAD_METRICS_OBSERVER_H_
