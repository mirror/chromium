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
#include "chrome/browser/browser_process.h"
#include "chrome/browser/page_load_metrics/page_load_metrics_util.h"
#include "components/ukm/public/ukm_entry_builder.h"
#include "components/ukm/public/ukm_recorder.h"
#include "net/base/host_port_pair.h"
#include "net/base/ip_address.h"
#include "net/base/net_errors.h"
#include "net/base/url_util.h"
#include "url/gurl.h"

namespace {

// TODO(uthakore): Update router regex based on further study.
// Returns true if the IP address matches the following regular expression for
// common router IP addresses:
// "^192\.168\.(0|10?|2)\.(1(00?)?)|^10\.(0|1)\.(0|10?)\.(1(00?)?|2)"
bool IsLikelyRouterIP(net::IPAddress ip_address) {
  return ip_address.IsIPv4() &&
         ((ip_address.bytes()[0] == 192 && ip_address.bytes()[1] == 168 &&
           (ip_address.bytes()[2] == 0 || ip_address.bytes()[2] == 1 ||
            ip_address.bytes()[2] == 2 || ip_address.bytes()[2] == 10) &&
           (ip_address.bytes()[3] == 1 || ip_address.bytes()[3] == 10 ||
            ip_address.bytes()[3] == 100)) ||
          (ip_address.bytes()[0] == 10 &&
           (ip_address.bytes()[1] == 0 || ip_address.bytes()[1] == 1) &&
           (ip_address.bytes()[2] == 0 || ip_address.bytes()[2] == 1 ||
            ip_address.bytes()[2] == 10) &&
           (ip_address.bytes()[3] == 1 || ip_address.bytes()[3] == 10 ||
            ip_address.bytes()[3] == 100 || ip_address.bytes()[3] == 2)));
}

// Attempts to get the IP address of a resource request from
// |extra_request_info.host_port_pair|, trying to get it from the URL string in
// |extra_request_info.url| if that fails.
// Sets the values of |resource_ip| and |port| with the extracted IP address and
// port, respectively.
// Returns true if a valid, nonempty IP address was extracted.
bool GetIPAndPort(
    const page_load_metrics::ExtraRequestCompleteInfo& extra_request_info,
    net::IPAddress& resource_ip,
    int& resource_port) {
  // If the request was successful, then the IP address should be in
  // |extra_request_info|.
  bool ip_exists = net::ParseURLHostnameToAddress(
      extra_request_info.host_port_pair.host(), &resource_ip);
  resource_port = extra_request_info.host_port_pair.port();

  // If the request failed, it's possible we didn't receive the IP address,
  // possibly because domain resolution failed. As a backup, try getting the IP
  // from the URL. If none was returned, try matching the hostname from the URL
  // itself as it might be an IP address if it is a local network request, which
  // is what we care about.
  if (!ip_exists && extra_request_info.url.is_valid()) {
    if (net::IsLocalhost(extra_request_info.url.HostNoBrackets())) {
      resource_ip = net::IPAddress::IPv4Localhost();
      ip_exists = true;
    }
    else {
      ip_exists = net::ParseURLHostnameToAddress(extra_request_info.url.host(),
                                                 &resource_ip);
    }
    resource_port = extra_request_info.url.EffectiveIntPort();
  }

  if (net::IsLocalhost(resource_ip.ToString())) {
    resource_ip = net::IPAddress::IPv4Localhost();
    ip_exists = true;
  }

  return ip_exists;
}

// List of mappings for localhost ports that belong to special categories that
// we want to track.
static const std::map<uint16_t, PortType>& localhost_port_categories =
    *new std::map<uint16_t, PortType>{
        {80, PortType::PORT_TYPE_WEB},    {8000, PortType::PORT_TYPE_WEB},
        {8008, PortType::PORT_TYPE_WEB},  {8080, PortType::PORT_TYPE_WEB},
        {8081, PortType::PORT_TYPE_WEB},  {8088, PortType::PORT_TYPE_WEB},
        {443, PortType::PORT_TYPE_WEB},   {3306, PortType::PORT_TYPE_DB},
        {5432, PortType::PORT_TYPE_DB},   {27017, PortType::PORT_TYPE_DB},
        {27018, PortType::PORT_TYPE_DB},  {27019, PortType::PORT_TYPE_DB},
        {515, PortType::PORT_TYPE_PRINT}, {631, PortType::PORT_TYPE_PRINT},
        {9000, PortType::PORT_TYPE_DEV},
        // TODO(uthakore): Add additional port mappings based on further study.
    };
}  // namespace

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
  net::HostPortPair address = navigation_handle->GetSocketAddress();
  bool parsed_successfully =
      net::ParseURLHostnameToAddress(address.host(), &page_ip_address_);

  // In cases where the page loaded does not have a socket address or was not a
  // network resource, we don't want to track the page load. Such resources will
  // fail to parse or return an empty IP address.
  if (!parsed_successfully || page_ip_address_.IsZero()) {
    return STOP_OBSERVING;
  }

  if (net::IsLocalhost(address.host()) ||
      page_ip_address_ == net::IPAddress::IPv6Localhost()) {
    page_load_type_ = DOMAIN_TYPE_LOCALHOST;
    page_ip_address_ = net::IPAddress::IPv4Localhost();
  } else {
    if (page_ip_address_.IsReserved()) {
      page_load_type_ = DOMAIN_TYPE_PRIVATE;
      // Maps from first byte of an IPv4 address to the number of bits in the
      // reserved prefix.
      static const uint8_t kReservedIPv4Prefixes[][2] = {
          {10, 8}, {100, 10}, {169, 16}, {172, 12}, {192, 16}, {198, 15}};

      for (const auto& entry : kReservedIPv4Prefixes) {
        // A reserved IP will always be a valid IPv4 or IPv6 address and will
        // thus have at least 4 bytes, so [0] is safe here.
        if (page_ip_address_.bytes()[0] == entry[0]) {
          page_ip_prefix_length_ = entry[1];
        }
      }

    } else {
      page_load_type_ = DOMAIN_TYPE_PUBLIC;
    }
  }

  RecordUkmDomainType(source_id);

  // If the load was localhost, we don't track it because it isn't meaningful
  // for our purposes.
  return (page_load_type_ == DOMAIN_TYPE_LOCALHOST ? STOP_OBSERVING
                                                   : CONTINUE_OBSERVING);
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
  net::IPAddress resource_ip;
  int resource_port;

  // We can't track anything if we don't have an IP address for the resource.
  // We also don't want to track any requests to the page's IP address itself.
  if (!GetIPAndPort(extra_request_info, resource_ip, resource_port) ||
      resource_ip.IsZero() || resource_ip == page_ip_address_) {
    return;
  }

  // We monitor localhost resource requests for both public and private page
  // loads.
  if (resource_ip == net::IPAddress::IPv4Localhost()) {
    if (extra_request_info.net_error != net::OK) {
      localhost_request_counts_[resource_port].second++;
    } else {
      localhost_request_counts_[resource_port].first++;
    }
  }
  // We only track public resource requests for private pages.
  else if (resource_ip.IsReserved() || page_load_type_ == DOMAIN_TYPE_PRIVATE) {
    if (extra_request_info.net_error != net::OK) {
      resource_request_counts_[resource_ip].second++;
    } else {
      resource_request_counts_[resource_ip].first++;
    }
  }
}

void LocalNetworkRequestsPageLoadMetricsObserver::OnComplete(
    const page_load_metrics::mojom::PageLoadTiming& timing,
    const page_load_metrics::PageLoadExtraInfo& info) {
  if (info.did_commit) {
    RecordHistograms();
    RecordUkmMetrics(info.source_id);
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

  // Compute the number of requests of each resource type for the loaded page.
  std::map<const std::string, int> counts;
  for (const auto& entry : resource_request_counts_) {
    counts[internal::kNonlocalhostHistogramNames.at(page_load_type_)
               .at(requested_resource_types_->at(entry.first))
               .at(true)] += entry.second.first;
    counts[internal::kNonlocalhostHistogramNames.at(page_load_type_)
               .at(requested_resource_types_->at(entry.first))
               .at(false)] += entry.second.second;
  }

  for (const auto& entry : localhost_request_counts_) {
    counts[internal::kLocalhostHistogramNames.at(page_load_type_)
               .at(DeterminePortType(entry.first))
               .at(true)] += entry.second.first;
    counts[internal::kLocalhostHistogramNames.at(page_load_type_)
               .at(DeterminePortType(entry.first))
               .at(false)] += entry.second.second;
  }

  // Log a histogram for each type of resource depending on the domain type of
  // the page load.
  if (page_load_type_ == DOMAIN_TYPE_PUBLIC) {
    UMA_HISTOGRAM_COUNTS_1000(
        internal::kNonlocalhostHistogramNames.at(DOMAIN_TYPE_PUBLIC)
            .at(RESOURCE_TYPE_PRIVATE)
            .at(true),
        counts[internal::kNonlocalhostHistogramNames.at(DOMAIN_TYPE_PUBLIC)
                   .at(RESOURCE_TYPE_PRIVATE)
                   .at(true)]);
    UMA_HISTOGRAM_COUNTS_1000(
        internal::kNonlocalhostHistogramNames.at(DOMAIN_TYPE_PUBLIC)
            .at(RESOURCE_TYPE_PRIVATE)
            .at(false),
        counts[internal::kNonlocalhostHistogramNames.at(DOMAIN_TYPE_PUBLIC)
                   .at(RESOURCE_TYPE_PRIVATE)
                   .at(false)]);
    UMA_HISTOGRAM_COUNTS_1000(
        internal::kNonlocalhostHistogramNames.at(DOMAIN_TYPE_PUBLIC)
            .at(RESOURCE_TYPE_ROUTER)
            .at(true),
        counts[internal::kNonlocalhostHistogramNames.at(DOMAIN_TYPE_PUBLIC)
                   .at(RESOURCE_TYPE_ROUTER)
                   .at(true)]);
    UMA_HISTOGRAM_COUNTS_1000(
        internal::kNonlocalhostHistogramNames.at(DOMAIN_TYPE_PUBLIC)
            .at(RESOURCE_TYPE_ROUTER)
            .at(false),
        counts[internal::kNonlocalhostHistogramNames.at(DOMAIN_TYPE_PUBLIC)
                   .at(RESOURCE_TYPE_ROUTER)
                   .at(false)]);
    UMA_HISTOGRAM_COUNTS_1000(
        internal::kLocalhostHistogramNames.at(DOMAIN_TYPE_PUBLIC)
            .at(PORT_TYPE_WEB)
            .at(true),
        counts[internal::kLocalhostHistogramNames.at(DOMAIN_TYPE_PUBLIC)
                   .at(PORT_TYPE_WEB)
                   .at(true)]);
    UMA_HISTOGRAM_COUNTS_1000(
        internal::kLocalhostHistogramNames.at(DOMAIN_TYPE_PUBLIC)
            .at(PORT_TYPE_WEB)
            .at(false),
        counts[internal::kLocalhostHistogramNames.at(DOMAIN_TYPE_PUBLIC)
                   .at(PORT_TYPE_WEB)
                   .at(false)]);
    UMA_HISTOGRAM_COUNTS_1000(
        internal::kLocalhostHistogramNames.at(DOMAIN_TYPE_PUBLIC)
            .at(PORT_TYPE_DB)
            .at(true),
        counts[internal::kLocalhostHistogramNames.at(DOMAIN_TYPE_PUBLIC)
                   .at(PORT_TYPE_DB)
                   .at(true)]);
    UMA_HISTOGRAM_COUNTS_1000(
        internal::kLocalhostHistogramNames.at(DOMAIN_TYPE_PUBLIC)
            .at(PORT_TYPE_DB)
            .at(false),
        counts[internal::kLocalhostHistogramNames.at(DOMAIN_TYPE_PUBLIC)
                   .at(PORT_TYPE_DB)
                   .at(false)]);
    UMA_HISTOGRAM_COUNTS_1000(
        internal::kLocalhostHistogramNames.at(DOMAIN_TYPE_PUBLIC)
            .at(PORT_TYPE_PRINT)
            .at(true),
        counts[internal::kLocalhostHistogramNames.at(DOMAIN_TYPE_PUBLIC)
                   .at(PORT_TYPE_PRINT)
                   .at(true)]);
    UMA_HISTOGRAM_COUNTS_1000(
        internal::kLocalhostHistogramNames.at(DOMAIN_TYPE_PUBLIC)
            .at(PORT_TYPE_PRINT)
            .at(false),
        counts[internal::kLocalhostHistogramNames.at(DOMAIN_TYPE_PUBLIC)
                   .at(PORT_TYPE_PRINT)
                   .at(false)]);
    UMA_HISTOGRAM_COUNTS_1000(
        internal::kLocalhostHistogramNames.at(DOMAIN_TYPE_PUBLIC)
            .at(PORT_TYPE_DEV)
            .at(true),
        counts[internal::kLocalhostHistogramNames.at(DOMAIN_TYPE_PUBLIC)
                   .at(PORT_TYPE_DEV)
                   .at(true)]);
    UMA_HISTOGRAM_COUNTS_1000(
        internal::kLocalhostHistogramNames.at(DOMAIN_TYPE_PUBLIC)
            .at(PORT_TYPE_DEV)
            .at(false),
        counts[internal::kLocalhostHistogramNames.at(DOMAIN_TYPE_PUBLIC)
                   .at(PORT_TYPE_DEV)
                   .at(false)]);
    UMA_HISTOGRAM_COUNTS_1000(
        internal::kLocalhostHistogramNames.at(DOMAIN_TYPE_PUBLIC)
            .at(PORT_TYPE_OTHER)
            .at(true),
        counts[internal::kLocalhostHistogramNames.at(DOMAIN_TYPE_PUBLIC)
                   .at(PORT_TYPE_OTHER)
                   .at(true)]);
    UMA_HISTOGRAM_COUNTS_1000(
        internal::kLocalhostHistogramNames.at(DOMAIN_TYPE_PUBLIC)
            .at(PORT_TYPE_OTHER)
            .at(false),
        counts[internal::kLocalhostHistogramNames.at(DOMAIN_TYPE_PUBLIC)
                   .at(PORT_TYPE_OTHER)
                   .at(false)]);
  } else {
    UMA_HISTOGRAM_COUNTS_1000(
        internal::kNonlocalhostHistogramNames.at(DOMAIN_TYPE_PRIVATE)
            .at(RESOURCE_TYPE_PUBLIC)
            .at(true),
        counts[internal::kNonlocalhostHistogramNames.at(DOMAIN_TYPE_PRIVATE)
                   .at(RESOURCE_TYPE_PUBLIC)
                   .at(true)]);
    UMA_HISTOGRAM_COUNTS_1000(
        internal::kNonlocalhostHistogramNames.at(DOMAIN_TYPE_PRIVATE)
            .at(RESOURCE_TYPE_PUBLIC)
            .at(false),
        counts[internal::kNonlocalhostHistogramNames.at(DOMAIN_TYPE_PRIVATE)
                   .at(RESOURCE_TYPE_PUBLIC)
                   .at(false)]);
    UMA_HISTOGRAM_COUNTS_1000(
        internal::kNonlocalhostHistogramNames.at(DOMAIN_TYPE_PRIVATE)
            .at(RESOURCE_TYPE_LOCAL_SAME_SUBNET)
            .at(true),
        counts[internal::kNonlocalhostHistogramNames.at(DOMAIN_TYPE_PRIVATE)
                   .at(RESOURCE_TYPE_LOCAL_SAME_SUBNET)
                   .at(true)]);
    UMA_HISTOGRAM_COUNTS_1000(
        internal::kNonlocalhostHistogramNames.at(DOMAIN_TYPE_PRIVATE)
            .at(RESOURCE_TYPE_LOCAL_SAME_SUBNET)
            .at(false),
        counts[internal::kNonlocalhostHistogramNames.at(DOMAIN_TYPE_PRIVATE)
                   .at(RESOURCE_TYPE_LOCAL_SAME_SUBNET)
                   .at(false)]);
    UMA_HISTOGRAM_COUNTS_1000(
        internal::kNonlocalhostHistogramNames.at(DOMAIN_TYPE_PRIVATE)
            .at(RESOURCE_TYPE_LOCAL_DIFF_SUBNET)
            .at(true),
        counts[internal::kNonlocalhostHistogramNames.at(DOMAIN_TYPE_PRIVATE)
                   .at(RESOURCE_TYPE_LOCAL_DIFF_SUBNET)
                   .at(true)]);
    UMA_HISTOGRAM_COUNTS_1000(
        internal::kNonlocalhostHistogramNames.at(DOMAIN_TYPE_PRIVATE)
            .at(RESOURCE_TYPE_LOCAL_DIFF_SUBNET)
            .at(false),
        counts[internal::kNonlocalhostHistogramNames.at(DOMAIN_TYPE_PRIVATE)
                   .at(RESOURCE_TYPE_LOCAL_DIFF_SUBNET)
                   .at(false)]);
    UMA_HISTOGRAM_COUNTS_1000(
        internal::kLocalhostHistogramNames.at(DOMAIN_TYPE_PRIVATE)
            .at(PORT_TYPE_WEB)
            .at(true),
        counts[internal::kLocalhostHistogramNames.at(DOMAIN_TYPE_PRIVATE)
                   .at(PORT_TYPE_WEB)
                   .at(true)]);
    UMA_HISTOGRAM_COUNTS_1000(
        internal::kLocalhostHistogramNames.at(DOMAIN_TYPE_PRIVATE)
            .at(PORT_TYPE_WEB)
            .at(false),
        counts[internal::kLocalhostHistogramNames.at(DOMAIN_TYPE_PRIVATE)
                   .at(PORT_TYPE_WEB)
                   .at(false)]);
    UMA_HISTOGRAM_COUNTS_1000(
        internal::kLocalhostHistogramNames.at(DOMAIN_TYPE_PRIVATE)
            .at(PORT_TYPE_DB)
            .at(true),
        counts[internal::kLocalhostHistogramNames.at(DOMAIN_TYPE_PRIVATE)
                   .at(PORT_TYPE_DB)
                   .at(true)]);
    UMA_HISTOGRAM_COUNTS_1000(
        internal::kLocalhostHistogramNames.at(DOMAIN_TYPE_PRIVATE)
            .at(PORT_TYPE_DB)
            .at(false),
        counts[internal::kLocalhostHistogramNames.at(DOMAIN_TYPE_PRIVATE)
                   .at(PORT_TYPE_DB)
                   .at(false)]);
    UMA_HISTOGRAM_COUNTS_1000(
        internal::kLocalhostHistogramNames.at(DOMAIN_TYPE_PRIVATE)
            .at(PORT_TYPE_PRINT)
            .at(true),
        counts[internal::kLocalhostHistogramNames.at(DOMAIN_TYPE_PRIVATE)
                   .at(PORT_TYPE_PRINT)
                   .at(true)]);
    UMA_HISTOGRAM_COUNTS_1000(
        internal::kLocalhostHistogramNames.at(DOMAIN_TYPE_PRIVATE)
            .at(PORT_TYPE_PRINT)
            .at(false),
        counts[internal::kLocalhostHistogramNames.at(DOMAIN_TYPE_PRIVATE)
                   .at(PORT_TYPE_PRINT)
                   .at(false)]);
    UMA_HISTOGRAM_COUNTS_1000(
        internal::kLocalhostHistogramNames.at(DOMAIN_TYPE_PRIVATE)
            .at(PORT_TYPE_DEV)
            .at(true),
        counts[internal::kLocalhostHistogramNames.at(DOMAIN_TYPE_PRIVATE)
                   .at(PORT_TYPE_DEV)
                   .at(true)]);
    UMA_HISTOGRAM_COUNTS_1000(
        internal::kLocalhostHistogramNames.at(DOMAIN_TYPE_PRIVATE)
            .at(PORT_TYPE_DEV)
            .at(false),
        counts[internal::kLocalhostHistogramNames.at(DOMAIN_TYPE_PRIVATE)
                   .at(PORT_TYPE_DEV)
                   .at(false)]);
    UMA_HISTOGRAM_COUNTS_1000(
        internal::kLocalhostHistogramNames.at(DOMAIN_TYPE_PRIVATE)
            .at(PORT_TYPE_OTHER)
            .at(true),
        counts[internal::kLocalhostHistogramNames.at(DOMAIN_TYPE_PRIVATE)
                   .at(PORT_TYPE_OTHER)
                   .at(true)]);
    UMA_HISTOGRAM_COUNTS_1000(
        internal::kLocalhostHistogramNames.at(DOMAIN_TYPE_PRIVATE)
            .at(PORT_TYPE_OTHER)
            .at(false),
        counts[internal::kLocalhostHistogramNames.at(DOMAIN_TYPE_PRIVATE)
                   .at(PORT_TYPE_OTHER)
                   .at(false)]);
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
}