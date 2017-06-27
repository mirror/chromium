// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/page_load_metrics/observers/local_network_requests_page_load_metrics_observer.h"

#include <regex>
#include <string>
#include <utility>

#include "base/feature_list.h"
#include "base/lazy_instance.h"
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

// Generates a histogram for a non-localhost resource using the values stored in
// |counts| given a combination of domain type, resource type, and status for
// the resource.
void CreateHistogram(std::map<const std::string, int> counts,
                     internal::DomainType domain_type,
                     internal::ResourceType resource_type,
                     bool status) {
  // Because the macro requires a single value for the maximum histogram index,
  // but we use three variables to distinguish histograms, we bitwise
  // concatenate them here to uniquely distinguish each combination. If the
  // enums for |DomainType| or |ResourceType| change, the bitwise arithmetic
  // below must also change.
  STATIC_HISTOGRAM_POINTER_GROUP(
      internal::kNonlocalhostHistogramNames.at(domain_type)
          .at(resource_type)
          .at(status),
      domain_type << 6 | resource_type << 1 | status,
      internal::DOMAIN_TYPE_LOCALHOST << 6 |
          internal::RESOURCE_TYPE_LOCALHOST << 1 | true + 1,
      Add(counts[internal::kNonlocalhostHistogramNames.at(domain_type)
                     .at(resource_type)
                     .at(status)]),
      base::Histogram::FactoryGet(
          internal::kNonlocalhostHistogramNames.at(domain_type)
              .at(resource_type)
              .at(status),
          1, 1000, 50, base::HistogramBase::kUmaTargetedHistogramFlag));
}

// Generates a histogram for a localhost resource using the values stored in
// |counts| given a combination of domain type, port type, and status for
// the resource.
void CreateHistogram(std::map<const std::string, int> counts,
                     internal::DomainType domain_type,
                     internal::PortType port_type,
                     bool status) {
  // Because the macro requires a single value for the maximum histogram index,
  // but we use three variables to distinguish histograms, we bitwise
  // concatenate them here to uniquely distinguish each combination. If the
  // enums for |DomainType| or |PortType| change, the bitwise arithmetic
  // below must also change.
  STATIC_HISTOGRAM_POINTER_GROUP(
      internal::kLocalhostHistogramNames.at(domain_type)
          .at(port_type)
          .at(status),
      domain_type << 5 | port_type << 1 | status,
      internal::DOMAIN_TYPE_LOCALHOST << 5 | internal::PORT_TYPE_DEV << 1 |
          true + 1,
      Add(counts[internal::kLocalhostHistogramNames.at(domain_type)
                     .at(port_type)
                     .at(status)]),
      base::Histogram::FactoryGet(
          internal::kLocalhostHistogramNames.at(domain_type)
              .at(port_type)
              .at(status),
          1, 1000, 50, base::HistogramBase::kUmaTargetedHistogramFlag));
}

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
    net::IPAddress* resource_ip,
    int* resource_port) {
  // If the request was successful, then the IP address should be in
  // |extra_request_info|.
  bool ip_exists = net::ParseURLHostnameToAddress(
      extra_request_info.host_port_pair.host(), resource_ip);
  *resource_port = extra_request_info.host_port_pair.port();

  // If the request failed, it's possible we didn't receive the IP address,
  // possibly because domain resolution failed. As a backup, try getting the IP
  // from the URL. If none was returned, try matching the hostname from the URL
  // itself as it might be an IP address if it is a local network request, which
  // is what we care about.
  if (!ip_exists && extra_request_info.url.is_valid()) {
    if (net::IsLocalhost(extra_request_info.url.HostNoBrackets())) {
      *resource_ip = net::IPAddress::IPv4Localhost();
      ip_exists = true;
    } else {
      ip_exists = net::ParseURLHostnameToAddress(extra_request_info.url.host(),
                                                 resource_ip);
    }
    *resource_port = extra_request_info.url.EffectiveIntPort();
  }

  if (net::IsLocalhost(resource_ip->ToString())) {
    *resource_ip = net::IPAddress::IPv4Localhost();
    ip_exists = true;
  }

  return ip_exists;
}

// List of mappings for localhost ports that belong to special categories that
// we want to track.
static const std::map<uint16_t, internal::PortType>& localhost_port_categories =
    *new std::map<uint16_t, internal::PortType>{
        {80, internal::PORT_TYPE_WEB},     {443, internal::PORT_TYPE_WEB},
        {8000, internal::PORT_TYPE_WEB},   {8008, internal::PORT_TYPE_WEB},
        {8080, internal::PORT_TYPE_WEB},   {8081, internal::PORT_TYPE_WEB},
        {8088, internal::PORT_TYPE_WEB},   {8181, internal::PORT_TYPE_WEB},
        {8888, internal::PORT_TYPE_WEB},   {3306, internal::PORT_TYPE_DB},
        {5432, internal::PORT_TYPE_DB},    {27017, internal::PORT_TYPE_DB},
        {427, internal::PORT_TYPE_PRINT},  {515, internal::PORT_TYPE_PRINT},
        {631, internal::PORT_TYPE_PRINT},  {9100, internal::PORT_TYPE_PRINT},
        {9220, internal::PORT_TYPE_PRINT}, {9500, internal::PORT_TYPE_PRINT},
        {3000, internal::PORT_TYPE_DEV},   {5000, internal::PORT_TYPE_DEV},
        {9000, internal::PORT_TYPE_DEV},
        // TODO(uthakore): Add additional port mappings based on further study.
    };

}  // namespace

namespace internal {

// Definitions of the methods to initialize the histogram names maps.
const std::map<internal::DomainType,
               std::map<internal::ResourceType, std::map<bool, std::string>>>&
GetNonlocalhostHistogramNames() {
  static base::LazyInstance<std::map<
      internal::DomainType,
      std::map<internal::ResourceType, std::map<bool, std::string>>>>::Leaky
      kNonlocalhostHistogramNames = LAZY_INSTANCE_INITIALIZER;

  kNonlocalhostHistogramNames.Get()[internal::DOMAIN_TYPE_PUBLIC]
                                   [internal::RESOURCE_TYPE_PRIVATE][true] =
      "LocalNetworkRequests.PublicPage.PrivateRequestCount.Successful";
  kNonlocalhostHistogramNames.Get()[internal::DOMAIN_TYPE_PUBLIC]
                                   [internal::RESOURCE_TYPE_PRIVATE][false] =
      "LocalNetworkRequests.PublicPage.PrivateRequestCount.Failed";
  kNonlocalhostHistogramNames.Get()[internal::DOMAIN_TYPE_PUBLIC]
                                   [internal::RESOURCE_TYPE_ROUTER][true] =
      "LocalNetworkRequests.PublicPage.RouterRequestCount.Successful";
  kNonlocalhostHistogramNames.Get()[internal::DOMAIN_TYPE_PUBLIC]
                                   [internal::RESOURCE_TYPE_ROUTER][false] =
      "LocalNetworkRequests.PublicPage.RouterRequestCount.Failed";

  kNonlocalhostHistogramNames.Get()[internal::DOMAIN_TYPE_PRIVATE]
                                   [internal::RESOURCE_TYPE_PUBLIC][true] =
      "LocalNetworkRequests.PrivatePage.PublicRequestCount.Successful";
  kNonlocalhostHistogramNames.Get()[internal::DOMAIN_TYPE_PRIVATE]
                                   [internal::RESOURCE_TYPE_PUBLIC][false] =
      "LocalNetworkRequests.PrivatePage.PublicRequestCount.Failed";
  kNonlocalhostHistogramNames
      .Get()[internal::DOMAIN_TYPE_PRIVATE]
            [internal::RESOURCE_TYPE_LOCAL_SAME_SUBNET][true] =
      "LocalNetworkRequests.PrivatePage.SameSubnetRequestCount.Successful";
  kNonlocalhostHistogramNames
      .Get()[internal::DOMAIN_TYPE_PRIVATE]
            [internal::RESOURCE_TYPE_LOCAL_SAME_SUBNET][false] =
      "LocalNetworkRequests.PrivatePage.SameSubnetRequestCount.Failed";
  kNonlocalhostHistogramNames
      .Get()[internal::DOMAIN_TYPE_PRIVATE]
            [internal::RESOURCE_TYPE_LOCAL_DIFF_SUBNET][true] =
      "LocalNetworkRequests.PrivatePage."
      "DifferentSubnetRequestCount.Successful";
  kNonlocalhostHistogramNames
      .Get()[internal::DOMAIN_TYPE_PRIVATE]
            [internal::RESOURCE_TYPE_LOCAL_DIFF_SUBNET][false] =
      "LocalNetworkRequests.PrivatePage.DifferentSubnetRequestCount.Failed";

  return kNonlocalhostHistogramNames.Get();
}

const std::map<internal::DomainType,
               std::map<internal::PortType, std::map<bool, std::string>>>&
GetLocalhostHistogramNames() {
  static base::LazyInstance<std::map<
      internal::DomainType,
      std::map<internal::PortType, std::map<bool, std::string>>>>::Leaky
      kLocalhostHistogramNames = LAZY_INSTANCE_INITIALIZER;

  kLocalhostHistogramNames
      .Get()[internal::DOMAIN_TYPE_PUBLIC][internal::PORT_TYPE_WEB][true] =
      "LocalNetworkRequests.PublicPage.Localhost."
      "WebRequestCount.Successful";
  kLocalhostHistogramNames
      .Get()[internal::DOMAIN_TYPE_PUBLIC][internal::PORT_TYPE_WEB][false] =
      "LocalNetworkRequests.PublicPage.Localhost."
      "WebRequestCount.Failed";
  kLocalhostHistogramNames
      .Get()[internal::DOMAIN_TYPE_PUBLIC][internal::PORT_TYPE_DB][true] =
      "LocalNetworkRequests.PublicPage.Localhost."
      "DatabaseRequestCount.Successful";
  kLocalhostHistogramNames
      .Get()[internal::DOMAIN_TYPE_PUBLIC][internal::PORT_TYPE_DB][false] =
      "LocalNetworkRequests.PublicPage.Localhost."
      "DatabaseRequestCount.Failed";
  kLocalhostHistogramNames
      .Get()[internal::DOMAIN_TYPE_PUBLIC][internal::PORT_TYPE_PRINT][true] =
      "LocalNetworkRequests.PublicPage.Localhost."
      "PrinterRequestCount.Successful";
  kLocalhostHistogramNames
      .Get()[internal::DOMAIN_TYPE_PUBLIC][internal::PORT_TYPE_PRINT][false] =
      "LocalNetworkRequests.PublicPage.Localhost."
      "PrinterRequestCount.Failed";
  kLocalhostHistogramNames
      .Get()[internal::DOMAIN_TYPE_PUBLIC][internal::PORT_TYPE_DEV][true] =
      "LocalNetworkRequests.PublicPage.Localhost."
      "DevelopmentRequestCount.Successful";
  kLocalhostHistogramNames
      .Get()[internal::DOMAIN_TYPE_PUBLIC][internal::PORT_TYPE_DEV][false] =
      "LocalNetworkRequests.PublicPage.Localhost."
      "DevelopmentRequestCount.Failed";
  kLocalhostHistogramNames
      .Get()[internal::DOMAIN_TYPE_PUBLIC][internal::PORT_TYPE_OTHER][true] =
      "LocalNetworkRequests.PublicPage.Localhost."
      "OtherRequestCount.Successful";
  kLocalhostHistogramNames
      .Get()[internal::DOMAIN_TYPE_PUBLIC][internal::PORT_TYPE_OTHER][false] =
      "LocalNetworkRequests.PublicPage.Localhost."
      "OtherRequestCount.Failed";

  kLocalhostHistogramNames
      .Get()[internal::DOMAIN_TYPE_PRIVATE][internal::PORT_TYPE_WEB][true] =
      "LocalNetworkRequests.PrivatePage.Localhost."
      "WebRequestCount.Successful";
  kLocalhostHistogramNames
      .Get()[internal::DOMAIN_TYPE_PRIVATE][internal::PORT_TYPE_WEB][false] =
      "LocalNetworkRequests.PrivatePage.Localhost."
      "WebRequestCount.Failed";
  kLocalhostHistogramNames
      .Get()[internal::DOMAIN_TYPE_PRIVATE][internal::PORT_TYPE_DB][true] =
      "LocalNetworkRequests.PrivatePage.Localhost."
      "DatabaseRequestCount.Successful";
  kLocalhostHistogramNames
      .Get()[internal::DOMAIN_TYPE_PRIVATE][internal::PORT_TYPE_DB][false] =
      "LocalNetworkRequests.PrivatePage.Localhost."
      "DatabaseRequestCount.Failed";
  kLocalhostHistogramNames
      .Get()[internal::DOMAIN_TYPE_PRIVATE][internal::PORT_TYPE_PRINT][true] =
      "LocalNetworkRequests.PrivatePage.Localhost."
      "PrinterRequestCount.Successful";
  kLocalhostHistogramNames
      .Get()[internal::DOMAIN_TYPE_PRIVATE][internal::PORT_TYPE_PRINT][false] =
      "LocalNetworkRequests.PrivatePage.Localhost."
      "PrinterRequestCount.Failed";
  kLocalhostHistogramNames
      .Get()[internal::DOMAIN_TYPE_PRIVATE][internal::PORT_TYPE_DEV][true] =
      "LocalNetworkRequests.PrivatePage.Localhost."
      "DevelopmentRequestCount.Successful";
  kLocalhostHistogramNames
      .Get()[internal::DOMAIN_TYPE_PRIVATE][internal::PORT_TYPE_DEV][false] =
      "LocalNetworkRequests.PrivatePage.Localhost."
      "DevelopmentRequestCount.Failed";
  kLocalhostHistogramNames
      .Get()[internal::DOMAIN_TYPE_PRIVATE][internal::PORT_TYPE_OTHER][true] =
      "LocalNetworkRequests.PrivatePage.Localhost."
      "OtherRequestCount.Successful";
  kLocalhostHistogramNames
      .Get()[internal::DOMAIN_TYPE_PRIVATE][internal::PORT_TYPE_OTHER][false] =
      "LocalNetworkRequests.PrivatePage.Localhost."
      "OtherRequestCount.Failed";
  return kLocalhostHistogramNames.Get();
}

}  // namespace internal

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

  // In cases where the page loaded does not have a socket address or was not a
  // network resource, we don't want to track the page load. Such resources will
  // fail to parse or return an empty IP address.
  if (!net::ParseURLHostnameToAddress(address.host(), &page_ip_address_) ||
      page_ip_address_.IsZero()) {
    return STOP_OBSERVING;
  }

  // |IsLocalhost| assumes (and doesn't verify) that any IPv6 address passed
  // to it does not have square brackets around it, but |HostPortPair::host|
  // retains the brackets, so we need to separately check for IPv6 localhost
  // here.
  if (net::IsLocalhost(address.host()) ||
      page_ip_address_ == net::IPAddress::IPv6Localhost()) {
    page_domain_type_ = internal::DOMAIN_TYPE_LOCALHOST;
  } else if (page_ip_address_.IsReserved()) {
    page_domain_type_ = internal::DOMAIN_TYPE_PRIVATE;
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
    page_domain_type_ = internal::DOMAIN_TYPE_PUBLIC;
  }

  RecordUkmDomainType(source_id);

  // If the load was localhost, we don't track it because it isn't meaningful
  // for our purposes.
  return (page_domain_type_ == internal::DOMAIN_TYPE_LOCALHOST)
             ? STOP_OBSERVING
             : CONTINUE_OBSERVING;
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
  if (!GetIPAndPort(extra_request_info, &resource_ip, &resource_port) ||
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
  else if (resource_ip.IsReserved() ||
           page_domain_type_ == internal::DOMAIN_TYPE_PRIVATE) {
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
  if (page_domain_type_ == internal::DOMAIN_TYPE_LOCALHOST) {
    return;
  }
  ResolveResourceTypes();

  // Compute the number of requests of each resource type for the loaded page.
  std::map<const std::string, int> counts;
  for (const auto& entry : resource_request_counts_) {
    counts[internal::kNonlocalhostHistogramNames.at(page_domain_type_)
               .at(requested_resource_types_->at(entry.first))
               .at(true)] += entry.second.first;
    counts[internal::kNonlocalhostHistogramNames.at(page_domain_type_)
               .at(requested_resource_types_->at(entry.first))
               .at(false)] += entry.second.second;
  }

  for (const auto& entry : localhost_request_counts_) {
    counts[internal::kLocalhostHistogramNames.at(page_domain_type_)
               .at(DeterminePortType(entry.first))
               .at(true)] += entry.second.first;
    counts[internal::kLocalhostHistogramNames.at(page_domain_type_)
               .at(DeterminePortType(entry.first))
               .at(false)] += entry.second.second;
  }

  // Log a histogram for each type of resource depending on the domain type of
  // the page load.
  if (page_domain_type_ == internal::DOMAIN_TYPE_PUBLIC) {
    for (auto resource_type :
         {internal::RESOURCE_TYPE_PRIVATE, internal::RESOURCE_TYPE_ROUTER}) {
      CreateHistogram(counts, page_domain_type_, resource_type, true);
      CreateHistogram(counts, page_domain_type_, resource_type, false);
    }
  } else {
    DCHECK_EQ(page_domain_type_, internal::DOMAIN_TYPE_PRIVATE);
    for (auto resource_type : {internal::RESOURCE_TYPE_PUBLIC,
                               internal::RESOURCE_TYPE_LOCAL_SAME_SUBNET,
                               internal::RESOURCE_TYPE_LOCAL_DIFF_SUBNET}) {
      CreateHistogram(counts, page_domain_type_, resource_type, true);
      CreateHistogram(counts, page_domain_type_, resource_type, false);
    }
  }
  for (auto port_type : {internal::PORT_TYPE_WEB, internal::PORT_TYPE_DB,
                         internal::PORT_TYPE_PRINT, internal::PORT_TYPE_DEV,
                         internal::PORT_TYPE_OTHER}) {
    CreateHistogram(counts, page_domain_type_, port_type, true);
    CreateHistogram(counts, page_domain_type_, port_type, false);
  }
}

internal::ResourceType
LocalNetworkRequestsPageLoadMetricsObserver::DetermineResourceType(
    net::IPAddress resource_ip) {
  if (page_domain_type_ == internal::DOMAIN_TYPE_PUBLIC) {
    DCHECK(resource_ip.IsReserved());
    return IsLikelyRouterIP(resource_ip) ? internal::RESOURCE_TYPE_ROUTER
                                         : internal::RESOURCE_TYPE_PRIVATE;
  }

  DCHECK_EQ(internal::DOMAIN_TYPE_PRIVATE, page_domain_type_);
  if (resource_ip.IsReserved()) {  // PRIVATE
    const bool is_same_subnet =
        net::CommonPrefixLength(page_ip_address_, resource_ip) >=
        page_ip_prefix_length_;
    return is_same_subnet ? internal::RESOURCE_TYPE_LOCAL_SAME_SUBNET
                          : internal::RESOURCE_TYPE_LOCAL_DIFF_SUBNET;
  }
  return internal::RESOURCE_TYPE_PUBLIC;  // PUBLIC
}

internal::PortType
LocalNetworkRequestsPageLoadMetricsObserver::DeterminePortType(int port) {
  auto lookup = localhost_port_categories.find(port);
  if (lookup == localhost_port_categories.end()) {
    return internal::PORT_TYPE_OTHER;
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
      base::MakeUnique<std::map<net::IPAddress, internal::ResourceType>>();
  for (const auto& entry : resource_request_counts_) {
    requested_resource_types_->insert(
        {entry.first, DetermineResourceType(entry.first)});
  }
}

void LocalNetworkRequestsPageLoadMetricsObserver::RecordUkmMetrics(
    ukm::SourceId source_id) {
  if (page_domain_type_ == internal::DOMAIN_TYPE_LOCALHOST ||
      g_browser_process->ukm_recorder() == nullptr) {
    return;
  }

  ResolveResourceTypes();

  // Log an entry for each non-localhost resource (one per IP address).
  for (const auto& entry : resource_request_counts_) {
    ukm::UkmRecorder* ukm_recorder = g_browser_process->ukm_recorder();
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
    std::unique_ptr<ukm::UkmEntryBuilder> builder =
        ukm_recorder->GetEntryBuilder(
            source_id, internal::kUkmLocalNetworkRequestsEventName);
    builder->AddMetric(internal::kUkmResourceTypeName,
                       static_cast<int>(internal::RESOURCE_TYPE_LOCALHOST));
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

  std::unique_ptr<ukm::UkmEntryBuilder> builder = ukm_recorder->GetEntryBuilder(
      source_id, internal::kUkmPageDomainEventName);
  builder->AddMetric(internal::kUkmDomainTypeName,
                     static_cast<int>(page_domain_type_));
}
