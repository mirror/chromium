// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_PAGE_LOAD_METRICS_OBSERVERS_LOCAL_NETWORK_REQUESTS_PAGE_LOAD_METRICS_OBSERVER_H_
#define CHROME_BROWSER_PAGE_LOAD_METRICS_OBSERVERS_LOCAL_NETWORK_REQUESTS_PAGE_LOAD_METRICS_OBSERVER_H_

#include <list>
#include <map>
#include <memory>

#include "base/macros.h"
#include "chrome/browser/page_load_metrics/page_load_metrics_observer.h"
#include "net/base/ip_address.h"

namespace {

// The domain type of the IP address of the loaded page. We use these to
// determine what classes of resource request metrics to collect.
enum DomainType {
  HOST_TYPE_PUBLIC = 0,
  HOST_TYPE_PRIVATE = 1,
  HOST_TYPE_LOCALHOST = 2,
};

// The type of the IP address of the loaded resource.
enum ResourceType {
  RESOURCE_TYPE_PUBLIC = 0,
  RESOURCE_TYPE_PRIVATE = 1,
  RESOURCE_TYPE_LOCAL_SAME_SUBNET = 2,
  RESOURCE_TYPE_LOCAL_DIFF_SUBNET = 4,
  RESOURCE_TYPE_ROUTER = 8,
  RESOURCE_TYPE_LOCALHOST = 16,
}

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
const char kUkmPageLoadEventName[] = "PageLoad";
const char kUkmLocalNetworkRequestsEventName[] = "LocalNetworkRequests";

// UKM metric names
const char kUkmDomainTypeName[] = "DomainType";
const char kUkmResourceTypeName[] = "ResourceType";
const char kUkmPortTypeName[] = "PortType";
const char kUkmSuccessfulCountName[] = "Count.Successful";
const char kUkmFailedCountName[] = "Count.Failed";

// UMA public page histogram names
const char kHistogramPublicPageSuccessfulPrivateRequest[] =
    "LocalNetworkRequests.PublicPage.PrivateRequestCount.Successful";
const char kHistogramPublicPageFailedPrivateRequest[] =
    "LocalNetworkRequests.PublicPage.PrivateRequestCount.Failed";
const char kHistogramPublicPageSuccessfulRouterRequest[] =
    "LocalNetworkRequests.PublicPage.RouterRequestCount.Successful";
const char kHistogramPublicPageFailedRouterRequest[] =
    "LocalNetworkRequests.PublicPage.RouterRequestCount.Failed";
const char kHistogramPublicPageSuccessfulLocalhostWebRequest[] =
    "LocalNetworkRequests.PublicPage.Localhost."
    "WebRequestCount.Successful";
const char kHistogramPublicPageFailedLocalhostWebRequest[] =
    "LocalNetworkRequests.PublicPage.Localhost."
    "WebRequestCount.Failed";
const char kHistogramPublicPageSuccessfulLocalhostDBRequest[] =
    "LocalNetworkRequests.PublicPage.Localhost."
    "DatabaseRequestCount.Successful";
const char kHistogramPublicPageFailedLocalhostDBRequest[] =
    "LocalNetworkRequests.PublicPage.Localhost."
    "DatabaseRequestCount.Failed";
const char kHistogramPublicPageSuccessfulLocalhostPrinterRequest[] =
    "LocalNetworkRequests.PublicPage.Localhost."
    "PrinterRequestCount.Successful";
const char kHistogramPublicPageFailedLocalhostPrinterRequest[] =
    "LocalNetworkRequests.PublicPage.Localhost."
    "PrinterRequestCount.Failed";
const char kHistogramPublicPageSuccessfulLocalhostDevRequest[] =
    "LocalNetworkRequests.PublicPage.Localhost."
    "DevelopmentRequestCount.Successful";
const char kHistogramPublicPageFailedLocalhostDevRequest[] =
    "LocalNetworkRequests.PublicPage.Localhost."
    "DevelopmentRequestCount.Failed";
const char kHistogramPublicPageSuccessfulLocalhostOtherRequest[] =
    "LocalNetworkRequests.PublicPage.Localhost."
    "OtherRequestCount.Successful";
const char kHistogramPublicPageFailedLocalhostOtherRequest[] =
    "LocalNetworkRequests.PublicPage.Localhost."
    "OtherRequestCount.Failed";

// UMA private page histogram names
const char kHistogramPrivatePageSuccessfulPublicRequest[] =
    "LocalNetworkRequests.PrivatePage.PublicRequestCount.Successful";
const char kHistogramPrivatePageFailedPublicRequest[] =
    "LocalNetworkRequests.PrivatePage.PublicRequestCount.Failed";
const char kHistogramPrivatePageSuccessfulSameSubnetRequest[] =
    "LocalNetworkRequests.PrivatePage.SameSubnetRequestCount.Successful";
const char kHistogramPrivatePageFailedSameSubnetRequest[] =
    "LocalNetworkRequests.PrivatePage.SameSubnetRequestCount.Failed";
const char kHistogramPrivatePageSuccessfulDiffSubnetRequest[] =
    "LocalNetworkRequests.PrivatePage."
    "DifferentSubnetRequestCount.Successful";
const char kHistogramPrivatePageFailedDiffSubnetRequest[] =
    "LocalNetworkRequests.PrivatePage."
    "DifferentSubnetRequestCount.Failed";
const char kHistogramPrivatePageSuccessfulLocalhostWebRequest[] =
    "LocalNetworkRequests.PrivatePage.Localhost."
    "WebRequestCount.Successful";
const char kHistogramPrivatePageFailedLocalhostWebRequest[] =
    "LocalNetworkRequests.PrivatePage.Localhost."
    "WebRequestCount.Failed";
const char kHistogramPrivatePageSuccessfulLocalhostDBRequest[] =
    "LocalNetworkRequests.PrivatePage.Localhost."
    "DatabaseRequestCount.Successful";
const char kHistogramPrivatePageFailedLocalhostDBRequest[] =
    "LocalNetworkRequests.PrivatePage.Localhost."
    "DatabaseRequestCount.Failed";
const char kHistogramPrivatePageSuccessfulLocalhostPrinterRequest[] =
    "LocalNetworkRequests.PrivatePage.Localhost."
    "PrinterRequestCount.Successful";
const char kHistogramPrivatePageFailedLocalhostPrinterRequest[] =
    "LocalNetworkRequests.PrivatePage.Localhost."
    "PrinterRequestCount.Failed";
const char kHistogramPrivatePageSuccessfulLocalhostDevRequest[] =
    "LocalNetworkRequests.PublicPage.Localhost."
    "DevelopmentRequestCount.Successful";
const char kHistogramPrivatePageFailedLocalhostDevRequest[] =
    "LocalNetworkRequests.PublicPage.Localhost."
    "DevelopmentRequestCount.Failed";
const char kHistogramPrivatePageSuccessfulLocalhostOtherRequest[] =
    "LocalNetworkRequests.PrivatePage.Localhost."
    "OtherRequestCount.Successful";
const char kHistogramPrivatePageFailedLocalhostOtherRequest[] =
    "LocalNetworkRequests.PrivatePage.Localhost."
    "OtherRequestCount.Failed";

}  // namespace internal

// This observer is for observing local network requests
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
  ResourceType DetermineResourceType(IPAddress ip_address);
  // Determines the port type for the localhost |port|.
  PortType DeterminePortType(int port);
  // Resolves the resource types to report for all IP addresses in
  // |resource_request_counts_|.
  void ResolveResourceTypes();

  void RecordUkmDomainType();
  void RecordHistograms();
  void RecordUkmMetrics();
  void RecordSingleUkmMetric();

  // Stores the counts of resource requests for each non-localhost IP address as
  // pairs of (successful, failed) request counts.
  std::unordered_map<IPAddress, std::pair<int32_t, int32_t>>
      resource_request_counts_;

  std::unique_ptr<std::unordered_map<IPAddress, ResourceType>>
      requested_resource_types_ = nullptr;

  // Stores the counts of resource requests for each localhost port as
  // pairs of (successful, failed) request counts.
  std::unordered_map<int, std::pair<int32_t, int32_t>>
      localhost_request_counts_;

  // The page load type. This is used to determine what resource requests to
  // monitor while the page is committed and to determine the UMA histogram name
  // to use.
  DomainType page_load_type_;

  // The IP address of the page that was loaded.
  IPAddress page_ip_address_;

  // List of mappings for localhost ports that belong to special categories that
  // we want to track.
  const std::unordered_map<uint16_t, PortType> localhost_port_categories_ = {
      {80, PortType::PORT_TYPE_WEB},    {8000, PortType::PORT_TYPE_WEB},
      {8008, PortType::PORT_TYPE_WEB},  {8080, PortType::PORT_TYPE_WEB},
      {8081, PortType::PORT_TYPE_WEB},  {8088, PortType::PORT_TYPE_WEB},
      {3306, PortType::PORT_TYPE_DB},   {5432, PortType::PORT_TYPE_DB},
      {27017, PortType::PORT_TYPE_DB},  {27018, PortType::PORT_TYPE_DB},
      {27019, PortType::PORT_TYPE_DB},  {515, PortType::PORT_TYPE_PRINT},
      {631, PortType::PORT_TYPE_PRINT},
      // TODO(uthakore): Add additional port mappings based on further study.
  };

  DISALLOW_COPY_AND_ASSIGN(LocalNetworkRequestsPageLoadMetricsObserver);
};

#endif  // CHROME_BROWSER_PAGE_LOAD_METRICS_OBSERVERS_LOCAL_NETWORK_REQUESTS_PAGE_LOAD_METRICS_OBSERVER_H_
