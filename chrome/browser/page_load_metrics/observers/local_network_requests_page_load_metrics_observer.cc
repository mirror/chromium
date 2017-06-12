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
#include "url/gurl.h"

LocalNetworkRequestsPageLoadMetricsObserver::
    LocalNetworkRequestsPageLoadMetricsObserver() {}
LocalNetworkRequestsPageLoadMetricsObserver::
    ~LocalNetworkRequestsPageLoadMetricsObserver() {}

page_load_metrics::PageLoadMetricsObserver::ObservePolicy
LocalNetworkRequestsPageLoadMetricsObserver::OnCommit(
    content::NavigationHandle* navigation_handle,
    ukm::SourceId source_id) {
  // Upon page load, we want to determine whether the page loaded was a public
  // domain or private domain.
  HostPortPair address = navigation_handle->GetSocketAddress();
  IPAddress ip_address;
  DCHECK(net::ParseURLHostnameToAddress(address.host(), &ip_addr));
  // Check if localhost first.
  // IPv4: https://tools.ietf.org/html/rfc1122#section-3.2.1.3
  // IPv6: https://tools.ietf.org/html/rfc4291#section-2.5.3
  if (ip_address.IsIPv6() && ip_address == IPAddress::IPv6Localhost() ||
      IPAddressMatchesPrefix(ip_address, IPAddress::Ipv4Localhost(), 8)) {
    page_load_type = HOST_TYPE_LOCALHOST;
    // If the load was localhost, we don't track it because it isn't
    // meaningful for our purposes.
    return STOP_OBSERVING;
  }

  if (ip_address.IsReserved()) {
    page_load_type = HOST_TYPE_PRIVATE;
  } else {
    page_load_type = HOST_TYPE_PUBLIC;
  }

  // For public and private (non-localhost) domains, we also generate an event
  // describing the domain type of the page load.
  RecordUkmDomainType();

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
  // TODO(uthakore): to implement
}

void LocalNetworkRequestsPageLoadMetricsObserver::RecordHistograms() {
  // TODO(uthakore): to implement

  //  if (page_bytes_ == 0)
  //    return;
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

void LocalNetworkRequestsPageLoadMetricsObserver::RecordUkmMetrics() {
  ukm::UkmRecorder* ukm_recorder = g_browser_process->ukm_recorder();
  std::unique_ptr<ukm::UkmEntryBuilder> builder = ukm_recorder->GetEntryBuilder(
      source_id, internal::kUkmLocalNetworkRequestsEventName);
  builder->AddMetric(internal::kUkmParseStartName,
                     timing.parse_timing->parse_start.value().InMilliseconds());
  // TODO(uthakore): to implement
}
