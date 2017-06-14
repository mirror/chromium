// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/page_load_metrics/observers/local_network_requests_page_load_metrics_observer.h"

#include <regex>
#include <string>
#include <utility>

#include "base/feature_list.h"
#include "base/logging.h"
#include "base/memory/ptr_util.h"
#include "base/metrics/histogram_macros.h"
#include "base/strings/string_util.h"
#include "chrome/browser/page_load_metrics/page_load_metrics_util.h"
#include "net/base/host_port_pair.h"
#include "net/base/ip_address.h"
#include "net/base/url_util.h"
#include "url/gurl.h"

namespace {

// TODO(uthakore): Update router regex based on further study.
// Returns true if the IP address matches the following regular expression for
// common router IP addresses:
// std::regex kRouterIpRegex = std::regex(
//    "^192\.168\.(0|10?)\.(1(00?)?)|^10\.(0|1)\.(0|10?)\.(1(00?)?|2)");
bool IsLikelyRouterIP(IPAddress ip_address) {
  return ip_address.IsIPv4() &&
         ((ip_address.bytes[0] == 192 && ip_address.bytes[1] == 168 &&
           (ip_address.bytes[2] == 0 || ip_address.bytes[2] == 1 ||
            ip_address.bytes[2] == 10) &&
           (ip_address.bytes[3] == 1 || ip_address.bytes[3] == 10 ||
            ip_address.bytes[3] == 100)) ||
          (ip_address.bytes[0] == 10 &&
           (ip_address.bytes[1] == 0 || ip_address.bytes[1] == 1) &&
           (ip_address.bytes[2] == 0 || ip_address.bytes[2] == 1 ||
            ip_address.bytes[2] == 10) &&
           (ip_address.bytes[3] == 1 || ip_address.bytes[3] == 10 ||
            ip_address.bytes[3] == 100 || ip_address.bytes[3] == 2)));
}

// List of mappings for localhost ports that belong to special categories that
// we want to track.
const std::unordered_map<uint16_t, PortType> localhost_port_categories = {
    {80, PortType::PORT_TYPE_WEB},    {8000, PortType::PORT_TYPE_WEB},
    {8008, PortType::PORT_TYPE_WEB},  {8080, PortType::PORT_TYPE_WEB},
    {8081, PortType::PORT_TYPE_WEB},  {8088, PortType::PORT_TYPE_WEB},
    {3306, PortType::PORT_TYPE_DB},   {5432, PortType::PORT_TYPE_DB},
    {27017, PortType::PORT_TYPE_DB},  {27018, PortType::PORT_TYPE_DB},
    {27019, PortType::PORT_TYPE_DB},  {515, PortType::PORT_TYPE_PRINT},
    {631, PortType::PORT_TYPE_PRINT},
    // TODO(uthakore): Add additional port mappings based on further study.
};
}

LocalNetworkRequestsPageLoadMetricsObserver::
    LocalNetworkRequestsPageLoadMetricsObserver() {}
LocalNetworkRequestsPageLoadMetricsObserver::
    ~LocalNetworkRequestsPageLoadMetricsObserver() {}

page_load_metrics::PageLoadMetricsObserver::ObservePolicy
LocalNetworkRequestsPageLoadMetricsObserver::OnCommit(
    content::NavigationHandle* navigation_handle,
    ukm::SourceId source_id) {
  // Upon page load, we want to determine whether the page loaded was a public
  // domain or private domain and generate an event describing the domain type.
  HostPortPair address = navigation_handle->GetSocketAddress();
  if (IsLocalhost(address.host())) {
    page_load_type_ = DOMAIN_TYPE_LOCALHOST;
    page_ip_address_ = IPAddress.IPv4Localhost();
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
    } else {
      page_load_type_ = DOMAIN_TYPE_PUBLIC;
    }
  }

  RecordUkmDomainType();

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
    RecordUkmMetrics();
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
  if (extra_info.did_commit) {
    RecordHistograms();
    RecordUkmMetrics();
  }
}

void LocalNetworkRequestsPageLoadMetricsObserver::ProcessLoadedResource(
    const page_load_metrics::ExtraRequestCompleteInfo& extra_request_info) {
  // If the resource request was successful, then the IP address should be in
  // |extra_request_info|.
  IPAddress resource_ip;
  bool ip_exists = net::ParseURLHostnameToAddress(
      extra_request_info.host_port_pair.host(), &resource_ip);
  int resource_port = host_port_pair.port();

  // If the request failed, it's possible we didn't receive the IP address,
  // possibly because domain resolution failed. As a backup, try getting the IP
  // from the URL. If none was returned, try matching the hostname from the URL
  // itself as it might be an IP address if it is a local network request, which
  // is what we care about.
  if (!ip_exists && extra_request_info.url.is_valid()) {
    if (IsLocalhost(extra_request_info.url.host())) {
      resource_ip = IPAddress.IPv4Localhost();
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
  if (resource_ip == IPAddress.IPv4Localhost()) {
    localhost_request_counts_[resource_port]
                             [extra_request_info.net_error ? 1 : 0]++;
  }
  // We only track public resource requests for private pages.
  else if (resource_ip.IsReserved() || page_load_type_ == DOMAIN_TYPE_PRIVATE) {
    resource_request_counts_[resource_ip]
                            [extra_request_info.net_error ? 1 : 0]++;
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

  // TODO(uthakore): to implement

  // For each resource, log a histogram for the associated type of request

  //
  //  int non_zero_ad_frames = 0;
  //  size_t total_ad_frame_bytes = 0;
  //  size_t uncached_ad_frame_bytes = 0;
  //
  //  for (const AdFrameData& ad_frame_data : ad_frames_data_storage_) {
  //    if (ad_frame_data.frame_bytes == 0)
  //      continue;
  //
  //    non_zero_ad_frames += 1;
  //    total_ad_frame_bytes += ad_frame_data.frame_bytes;
  //    uncached_ad_frame_bytes += ad_frame_data.frame_bytes_uncached;
  //
  //    PAGE_BYTES_HISTOGRAM(
  //        "Ads.Google.Bytes.AdFrames.PerFrame.Total",
  //        ad_frame_data.frame_bytes);
  //    PAGE_BYTES_HISTOGRAM(
  //        "Ads.Google.Bytes.AdFrames.PerFrame.Network",
  //        ad_frame_data.frame_bytes_uncached);
  //    UMA_HISTOGRAM_PERCENTAGE(
  //        "Ads.Google.Bytes.AdFrames.PerFrame.PercentNetwork",
  //        ad_frame_data.frame_bytes_uncached * 100 /
  //        ad_frame_data.frame_bytes);
  //  }
  //
  //  UMA_HISTOGRAM_COUNTS_1000(
  //      "Ads.Google.FrameCounts.AnyParentFrame.AdFrames",
  //      non_zero_ad_frames);
  //
  //  // Don't post UMA for pages that don't have ads.
  //  if (non_zero_ad_frames == 0)
  //    return;
  //
  //  PAGE_BYTES_HISTOGRAM(
  //      "Ads.Google.Bytes.NonAdFrames.Aggregate.Total",
  //      page_bytes_ - total_ad_frame_bytes);
  //
  //  PAGE_BYTES_HISTOGRAM("Ads.Google.Bytes.FullPage.Total",
  //                       page_bytes_);
  //  PAGE_BYTES_HISTOGRAM("Ads.Google.Bytes.FullPage.Network",
  //                       uncached_page_bytes_);
  //  if (page_bytes_) {
  //    UMA_HISTOGRAM_PERCENTAGE(
  //        "Ads.Google.Bytes.FullPage.Total.PercentAds",
  //        total_ad_frame_bytes * 100 / page_bytes_);
  //  }
  //  if (uncached_page_bytes_ > 0) {
  //    UMA_HISTOGRAM_PERCENTAGE(
  //        "Ads.Google.Bytes.FullPage.Network.PercentAds",
  //        uncached_ad_frame_bytes * 100 / uncached_page_bytes_);
  //  }
  //
  //  PAGE_BYTES_HISTOGRAM(
  //      "Ads.Google.Bytes.AdFrames.Aggregate.Total",
  //      total_ad_frame_bytes);
  //  PAGE_BYTES_HISTOGRAM(
  //      "Ads.Google.Bytes.AdFrames.Aggregate.Network",
  //      uncached_ad_frame_bytes);
  //
  //  if (total_ad_frame_bytes) {
  //    UMA_HISTOGRAM_PERCENTAGE(
  //        "Ads.Google.Bytes.AdFrames.Aggregate.PercentNetwork",
  //        uncached_ad_frame_bytes * 100 / total_ad_frame_bytes);
  //  }
}

ResourceType LocalNetworkRequestsPageLoadMetricsObserver::DetermineResourceType(
    IPAddress ip_address) {
  if (page_load_type_ == DOMAIN_TYPE_PUBLIC) {
    DCHECK(resource_ip.IsReserved());
    if (IsLikelyRouterIP(resource_ip)) {
      return RESOURCE_TYPE_ROUTER;
    } else {
      return RESOURCE_TYPE_PRIVATE;
    }
  } else {
    if (resource_ip.IsReserved()) {  // PRIVATE
      // CommonPrefixLength -- for subnet checking
      // TODO(uthakore): CHECKSUBNET
    } else {  // PUBLIC
      return RESOURCE_TYPE_PUBLIC
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

void ResolveResourceTypes() {
  // Lazy instantiation.
  if (requested_resource_types_) {
    return;
  }

  request_resource_types_ =
      base::MakeUnique<std::unordered_map<IPAddress, ResourceType>>();
  for (const auto& entry : resource_request_counts_) {
    request_resource_types_->insert(
        {entry.first, DetermineResourceType(entry.first)});
  }
}

void LocalNetworkRequestsPageLoadMetricsObserver::RecordUkmMetrics() {
  ukm::UkmRecorder* ukm_recorder = g_browser_process->ukm_recorder();
  if (!ukm_recorder || page_load_type_ == DOMAIN_TYPE_LOCALHOST) {
    return;
  }
  ResolveResourceTypes();

  // Log an entry for each non-localhost resource (one per IP address).
  for (const auto& entry : resource_request_counts_) {
    std::unique_ptr<ukm::UkmEntryBuilder> builder =
        ukm_recorder->GetEntryBuilder(
            source_id, internal::kUkmLocalNetworkRequestsEventName);
    builder->AddMetric(
        internal::kUkmResourceTypeName,
        static_cast<int>(requested_resource_types_[entry.first]));
    builder->AddMetric(internal::kUkmSuccessfulCountName, entry.second.first);
    builder->AddMetric(internal::kUkmFailedCountName, entry.second.second);
  }

  // Log an entry for each localhost resource (one per port).
  for (const auto& entry : localhost_request_counts_) {
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

void LocalNetworkRequestsPageLoadMetricsObserver::RecordUkmDomainType() {
  ukm::UkmRecorder* ukm_recorder = g_browser_process->ukm_recorder();
  if (!ukm_recorder) {
    return;
  }

  std::unique_ptr<ukm::UkmEntryBuilder> builder =
      ukm_recorder->GetEntryBuilder(source_id, internal::kUkmPageLoadEventName);
  builder->AddMetric(internal::kUkmDomainTypeName,
                     static_cast<int>(page_load_type_));
}