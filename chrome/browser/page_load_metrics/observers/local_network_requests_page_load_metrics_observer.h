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

namespace {

// The types of host IP addresses to distinguish between when collecting local
// network request metrics. Routers are a subset of private addresses.
enum HostType {
  HOST_TYPE_PUBLIC = 0,
  HOST_TYPE_PRIVATE = 1,
  HOST_TYPE_LOCALHOST = 2,
  HOST_TYPE_ROUTER = 5,
};

// The types of services to distinguish between when collecting local network
// request metrics.
enum PortType {
  PORT_TYPE_WEB,
  PORT_TYPE_DB,
  PORT_TYPE_PRINT,
  PORT_TYPE_OTHER,
};

// TODO(uthakore): Update router regex based on further study.
std::regex kRouterIpRegex = std::regex(
    "^192\.168\.(0|10?)\.(1(00?)?)|^10\.(0|1)\.(0|10?)\.(1(00?)?|2)");

}  // namespace

namespace internal {

// UKM event names
const char kUkmLocalNetworkRequestsPublicPageEvent[] =
    "LocalNetworkRequests.PublicPage";
const char kUkmLocalNetworkRequestsPrivatePageEvent[] =
    "LocalNetworkRequests.PrivatePage";

// UKM metric names
const char kUkmSuccessfulPrivateRequestMetric[] =
    "PrivateRequestCount.Successful";
const char kUkmFailedPrivateRequestMetric[] = "PrivateRequestCount.Failed";
const char kUkmSuccessfulRouterRequestMetric[] =
    "RouterRequestCount.Successful";
const char kUkmFailedRouterRequestMetric[] = "RouterRequestCount.Failed";
const char kUkmSuccessfulLocalhostWebRequestMetric[] =
    "Localhost.WebRequestCount.Successful";
const char kUkmFailedLocalhostWebRequestMetric[] =
    "Localhost.WebRequestCount.Failed";
const char kUkmSuccessfulLocalhostDBRequestMetric[] =
    "Localhost.DBRequestCount.Successful";
const char kUkmFailedLocalhostDBRequestMetric[] =
    "Localhost.DBRequestCount.Failed";
const char kUkmSuccessfulLocalhostPrinterRequestMetric[] =
    "Localhost.PrinterRequestCount.Successful";
const char kUkmFailedLocalhostPrinterRequestMetric[] =
    "Localhost.PrinterRequestCount.Failed";
const char kUkmSuccessfulLocalhostOtherRequestMetric[] =
    "Localhost.OtherRequestCount.Successful";
const char kUkmFailedLocalhostOtherRequestMetric[] =
    "Localhost.OtherRequestCount.Failed";
const char kUkmSuccessfulPublicRequestMetric[] =
    "PublicRequestCount.Successful";
const char kUkmFailedPublicRequestMetric[] = "PublicRequestCount.Failed";
const char kUkmSuccessfulSameSubnetRequestMetric[] =
    "SameSubnetRequestCount.Successful";
const char kUkmFailedSameSubnetRequestMetric[] =
    "SameSubnetRequestCount.Failed";
const char kUkmSuccessfulDifferentSubnetRequestMetric[] =
    "DifferentSubnetRequestCount.Successful";
const char kUkmFailedDifferentSubnetRequestMetric[] =
    "DifferentSubnetRequestCount.Failed";

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
const char kHistogramPublicPageSuccessfulLocalhostDatabaseRequest[] =
    "LocalNetworkRequests.PublicPage.Localhost."
    "DatabaseRequestCount.Successful";
const char kHistogramPublicPageFailedLocalhostDatabaseRequest[] =
    "LocalNetworkRequests.PublicPage.Localhost."
    "DatabaseRequestCount.Failed";
const char kHistogramPublicPageSuccessfulLocalhostPrinterRequest[] =
    "LocalNetworkRequests.PublicPage.Localhost."
    "PrinterRequestCount.Successful";
const char kHistogramPublicPageFailedLocalhostPrinterRequest[] =
    "LocalNetworkRequests.PublicPage.Localhost."
    "PrinterRequestCount.Failed";
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
const char kHistogramPrivatePageSuccessfulDifferentSubnetRequest[] =
    "LocalNetworkRequests.PrivatePage."
    "DifferentSubnetRequestCount.Successful";
const char kHistogramPrivatePageFailedDifferentSubnetRequest[] =
    "LocalNetworkRequests.PrivatePage."
    "DifferentSubnetRequestCount.Failed";
const char kHistogramPrivatePageSuccessfulLocalhostWebRequest[] =
    "LocalNetworkRequests.PrivatePage.Localhost."
    "WebRequestCount.Successful";
const char kHistogramPrivatePageFailedLocalhostWebRequest[] =
    "LocalNetworkRequests.PrivatePage.Localhost."
    "WebRequestCount.Failed";
const char kHistogramPrivatePageSuccessfulLocalhostDatabaseRequest[] =
    "LocalNetworkRequests.PrivatePage.Localhost."
    "DatabaseRequestCount.Successful";
const char kHistogramPrivatePageFailedLocalhostDatabaseRequest[] =
    "LocalNetworkRequests.PrivatePage.Localhost."
    "DatabaseRequestCount.Failed";
const char kHistogramPrivatePageSuccessfulLocalhostPrinterRequest[] =
    "LocalNetworkRequests.PrivatePage.Localhost."
    "PrinterRequestCount.Successful";
const char kHistogramPrivatePageFailedLocalhostPrinterRequest[] =
    "LocalNetworkRequests.PrivatePage.Localhost."
    "PrinterRequestCount.Failed";
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
  // Clears all local resource request counts. Only used if we decide to log
  // metrics but the observer may stay in scope and capture additional resource
  // requests.
  void ClearLocalState();

  // Stores all requisite information for each resource request that is made by
  // the page.
  void ProcessLoadedResource(
      const page_load_metrics::ExtraRequestCompleteInfo& extra_request_info);

  void RecordUkmDomainType();
  void RecordHistograms();
  void RecordUkmMetrics();

  // Stores the counts of resource requests for each non-localhost IP address as
  // pairs of (successful, failed) request counts.
  std::unordered_map<IPAddress, std::pair<int, int>> resource_request_counts_;

  // Stores the counts of resource requests for each localhost port as
  // pairs of (successful, failed) request counts.
  std::unordered_map<int, std::pair<int, int>> localhost_request_counts_;

  // The page load type. This is used to determine whether the UKM/UMA event is
  // PublicPage or PrivatePage.
  HostType page_load_type_;

  // List of mappings for localhost ports that belong to special categories that
  // we want to track.
  const std::unordered_map<int, PortType> localhost_port_categories_ = {
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
