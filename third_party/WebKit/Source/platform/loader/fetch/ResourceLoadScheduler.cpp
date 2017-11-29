// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform/loader/fetch/ResourceLoadScheduler.h"

#include "base/metrics/field_trial_params.h"
#include "base/strings/string_number_conversions.h"
#include "platform/Histogram.h"
#include "platform/runtime_enabled_features.h"

namespace blink {

namespace {

// Field trial name.
const char kResourceLoadSchedulerTrial[] = "ResourceLoadScheduler";

// Field trial parameter names.
// Note: bg_limit is supported on m61+, but bg_sub_limit is only on m63+.
// If bg_sub_limit param is not found, we should use bg_limit to make the
// study result statistically correct.
const char kOutstandingLimitForBackgroundMainFrameName[] = "bg_limit";
const char kOutstandingLimitForBackgroundSubFrameName[] = "bg_sub_limit";

// Field trial default parameters.
constexpr size_t kOutstandingLimitForBackgroundFrameDefault = 16u;

// Maximum request count that request count metrics assume.
constexpr base::HistogramBase::Sample kMaximumReportSize10K = 10000;

// Maximum traffic bytes that traffic metrics assume.
constexpr base::HistogramBase::Sample kMaximumReportSize1G =
    1 * 1000 * 1000 * 1000;

// Bucket count for metrics.
constexpr int32_t kReportBucket = 25;

uint32_t GetFieldTrialUint32Param(const char* name, uint32_t default_param) {
  std::map<std::string, std::string> trial_params;
  bool result =
      base::GetFieldTrialParams(kResourceLoadSchedulerTrial, &trial_params);
  if (!result)
    return default_param;

  const auto& found = trial_params.find(name);
  if (found == trial_params.end())
    return default_param;

  uint32_t param;
  if (!base::StringToUint(found->second, &param))
    return default_param;

  return param;
}

uint32_t GetOutstandingThrottledLimit(FetchContext* context) {
  DCHECK(context);

  uint32_t main_frame_limit =
      GetFieldTrialUint32Param(kOutstandingLimitForBackgroundMainFrameName,
                               kOutstandingLimitForBackgroundFrameDefault);
  if (context->IsMainFrame())
    return main_frame_limit;

  // We do not have a fixed default limit for sub-frames, but use the limit for
  // the main frame so that it works as how previous versions that haven't
  // consider sub-frames' specific limit work.
  return GetFieldTrialUint32Param(kOutstandingLimitForBackgroundSubFrameName,
                                  main_frame_limit);
}

}  // namespace

class ResourceLoadScheduler::TrafficMonitor {
 public:
  TrafficMonitor(bool is_main_frame);
  ~TrafficMonitor();

  // Reports traffic metrics.
  void Flush(WebFrameScheduler::ThrottlingState);

  // Tracks resource request completion.
  void Track(int64_t decoded_body_length);

 private:
  const bool is_main_frame_;

  WebFrameScheduler::ThrottlingState last_state_ =
      WebFrameScheduler::ThrottlingState::kStopped;

  size_t request_count_ = 0;
  size_t traffic_bytes_ = 0;
  size_t total_throttled_request_count_ = 0;
  size_t total_throttled_traffic_bytes_ = 0;
  size_t total_not_throttled_request_count_ = 0;
  size_t total_not_throttled_traffic_bytes_ = 0;
};

ResourceLoadScheduler::TrafficMonitor::TrafficMonitor(bool is_main_frame)
    : is_main_frame_(is_main_frame) {}

ResourceLoadScheduler::TrafficMonitor::~TrafficMonitor() {
  Flush(WebFrameScheduler::ThrottlingState::kStopped);

  DEFINE_STATIC_LOCAL(
      CustomCountHistogram, main_frame_total_throttled_request_count,
      ("Blink.ResourceLoadScheduler.TotalRequestCount.MainframeThrottled", 0,
       kMaximumReportSize10K, kReportBucket));
  DEFINE_STATIC_LOCAL(
      CustomCountHistogram, main_frame_total_not_throttled_request_count,
      ("Blink.ResourceLoadScheduler.TotalRequestCount.MainframeNotThrottled", 0,
       kMaximumReportSize10K, kReportBucket));
  DEFINE_STATIC_LOCAL(
      CustomCountHistogram, sub_frame_total_throttled_request_count,
      ("Blink.ResourceLoadScheduler.TotalRequestCount.SubframeThrottled", 0,
       kMaximumReportSize10K, kReportBucket));
  DEFINE_STATIC_LOCAL(
      CustomCountHistogram, sub_frame_total_not_throttled_request_count,
      ("Blink.ResourceLoadScheduler.TotalRequestCount.SubframeNotThrottled", 0,
       kMaximumReportSize10K, kReportBucket));

  DEFINE_STATIC_LOCAL(
      CustomCountHistogram, main_frame_total_throttled_traffic_bytes,
      ("Blink.ResourceLoadScheduler.TotalTrafficBytes.MainframeThrottled", 0,
       kMaximumReportSize1G, kReportBucket));
  DEFINE_STATIC_LOCAL(
      CustomCountHistogram, main_frame_total_not_throttled_traffic_bytes,
      ("Blink.ResourceLoadScheduler.TotalTrafficBytes.MainframeNotThrottled", 0,
       kMaximumReportSize1G, kReportBucket));
  DEFINE_STATIC_LOCAL(
      CustomCountHistogram, sub_frame_total_throttled_traffic_bytes,
      ("Blink.ResourceLoadScheduler.TotalTrafficBytes.SubframeThrottled", 0,
       kMaximumReportSize1G, kReportBucket));
  DEFINE_STATIC_LOCAL(
      CustomCountHistogram, sub_frame_total_not_throttled_traffic_bytes,
      ("Blink.ResourceLoadScheduler.TotalTrafficBytes.SubframeNotThrottled", 0,
       kMaximumReportSize1G, kReportBucket));

  if (is_main_frame_) {
    main_frame_total_throttled_request_count.Count(
        total_throttled_request_count_);
    main_frame_total_not_throttled_request_count.Count(
        total_not_throttled_request_count_);
    main_frame_total_throttled_traffic_bytes.Count(
        total_throttled_traffic_bytes_);
    main_frame_total_not_throttled_traffic_bytes.Count(
        total_not_throttled_traffic_bytes_);
  } else {
    sub_frame_total_throttled_request_count.Count(
        total_throttled_request_count_);
    sub_frame_total_not_throttled_request_count.Count(
        total_not_throttled_request_count_);
    sub_frame_total_throttled_traffic_bytes.Count(
        total_throttled_traffic_bytes_);
    sub_frame_total_not_throttled_traffic_bytes.Count(
        total_not_throttled_traffic_bytes_);
  }
}

void ResourceLoadScheduler::TrafficMonitor::Flush(
    WebFrameScheduler::ThrottlingState state) {
  DEFINE_STATIC_LOCAL(
      CustomCountHistogram, main_frame_throttled_request_count,
      ("Blink.ResourceLoadScheduler.RequestCount.MainframeThrottled", 0,
       kMaximumReportSize10K, kReportBucket));
  DEFINE_STATIC_LOCAL(
      CustomCountHistogram, main_frame_not_throttled_request_count,
      ("Blink.ResourceLoadScheduler.RequestCount.MainframeNotThrottled", 0,
       kMaximumReportSize10K, kReportBucket));
  DEFINE_STATIC_LOCAL(
      CustomCountHistogram, sub_frame_throttled_request_count,
      ("Blink.ResourceLoadScheduler.RequestCount.SubframeThrottled", 0,
       kMaximumReportSize10K, kReportBucket));
  DEFINE_STATIC_LOCAL(
      CustomCountHistogram, sub_frame_not_throttled_request_count,
      ("Blink.ResourceLoadScheduler.RequestCount.SubframeNotThrottled", 0,
       kMaximumReportSize10K, kReportBucket));

  DEFINE_STATIC_LOCAL(
      CustomCountHistogram, main_frame_throttled_traffic_bytes,
      ("Blink.ResourceLoadScheduler.TrafficBytes.MainframeThrottled", 0,
       kMaximumReportSize1G, kReportBucket));
  DEFINE_STATIC_LOCAL(
      CustomCountHistogram, main_frame_not_throttled_traffic_bytes,
      ("Blink.ResourceLoadScheduler.TrafficBytes.MainframeNotThrottled", 0,
       kMaximumReportSize1G, kReportBucket));
  DEFINE_STATIC_LOCAL(
      CustomCountHistogram, sub_frame_throttled_traffic_bytes,
      ("Blink.ResourceLoadScheduler.TrafficBytes.SubframeThrottled", 0,
       kMaximumReportSize1G, kReportBucket));
  DEFINE_STATIC_LOCAL(
      CustomCountHistogram, sub_frame_not_throttled_traffic_bytes,
      ("Blink.ResourceLoadScheduler.TrafficBytes.SubframeNotThrottled", 0,
       kMaximumReportSize1G, kReportBucket));

  switch (last_state_) {
    case WebFrameScheduler::ThrottlingState::kThrottled:
      if (is_main_frame_) {
        main_frame_throttled_request_count.Count(request_count_);
        main_frame_throttled_traffic_bytes.Count(traffic_bytes_);
      } else {
        sub_frame_throttled_request_count.Count(request_count_);
        sub_frame_throttled_traffic_bytes.Count(traffic_bytes_);
      }
      break;
      total_throttled_request_count_ += request_count_;
      total_throttled_traffic_bytes_ += traffic_bytes_;
    case WebFrameScheduler::ThrottlingState::kNotThrottled:
      if (is_main_frame_) {
        main_frame_not_throttled_request_count.Count(request_count_);
        main_frame_not_throttled_traffic_bytes.Count(traffic_bytes_);
      } else {
        sub_frame_not_throttled_request_count.Count(request_count_);
        sub_frame_not_throttled_traffic_bytes.Count(traffic_bytes_);
      }
      total_not_throttled_request_count_ += request_count_;
      total_not_throttled_traffic_bytes_ += traffic_bytes_;
      break;
    case WebFrameScheduler::ThrottlingState::kStopped:
      break;
  }

  last_state_ = state;
  request_count_ = 0;
  traffic_bytes_ = 0;
}

void ResourceLoadScheduler::TrafficMonitor::Track(int64_t decoded_body_length) {
  if (decoded_body_length < 0)
    return;

  request_count_++;
  traffic_bytes_ += decoded_body_length;
}

constexpr ResourceLoadScheduler::ClientId
    ResourceLoadScheduler::kInvalidClientId;

ResourceLoadScheduler::ResourceLoadScheduler(FetchContext* context)
    : outstanding_throttled_limit_(GetOutstandingThrottledLimit(context)),
      context_(context) {
  traffic_monitor_ = std::make_unique<ResourceLoadScheduler::TrafficMonitor>(
      context_->IsMainFrame());

  if (!RuntimeEnabledFeatures::ResourceLoadSchedulerEnabled())
    return;

  auto* scheduler = context->GetFrameScheduler();
  if (!scheduler)
    return;

  is_enabled_ = true;
  scheduler->AddThrottlingObserver(WebFrameScheduler::ObserverType::kLoader,
                                   this);
}

ResourceLoadScheduler::~ResourceLoadScheduler() = default;

void ResourceLoadScheduler::Trace(blink::Visitor* visitor) {
  visitor->Trace(pending_request_map_);
  visitor->Trace(context_);
}

void ResourceLoadScheduler::Shutdown() {
  // Do nothing if the feature is not enabled, or Shutdown() was already called.
  if (is_shutdown_)
    return;
  is_shutdown_ = true;

  // Flush out all report here to avoid running dangerous code in GC phase.
  traffic_monitor_.reset();

  if (!is_enabled_)
    return;
  auto* scheduler = context_->GetFrameScheduler();
  DCHECK(scheduler);
  scheduler->RemoveThrottlingObserver(WebFrameScheduler::ObserverType::kLoader,
                                      this);
}

void ResourceLoadScheduler::Request(ResourceLoadSchedulerClient* client,
                                    ThrottleOption option,
                                    ResourceLoadScheduler::ClientId* id) {
  *id = GenerateClientId();
  if (is_shutdown_)
    return;

  if (!is_enabled_ || option == ThrottleOption::kCanNotBeThrottled) {
    Run(*id, client);
    return;
  }

  pending_request_map_.insert(*id, client);
  pending_requests_.insert(ClientIdWithPriority(*id));
  MaybeRun();
}

bool ResourceLoadScheduler::Release(ResourceLoadScheduler::ClientId id,
                                    ResourceLoadScheduler::ReleaseOption option,
                                    int64_t decoded_body_length) {
  // Check kInvalidClientId that can not be passed to the HashSet.
  if (id == kInvalidClientId)
    return false;

  if (running_requests_.find(id) != running_requests_.end()) {
    running_requests_.erase(id);
    traffic_monitor_->Track(decoded_body_length);
    if (option == ReleaseOption::kReleaseAndSchedule)
      MaybeRun();
    return true;
  }
  auto found = pending_request_map_.find(id);
  if (found != pending_request_map_.end()) {
    pending_request_map_.erase(found);
    // Intentionally does not remove it from |pending_requests_|.

    // Didn't release any running requests, but the outstanding limit might be
    // changed to allow another request.
    if (option == ReleaseOption::kReleaseAndSchedule)
      MaybeRun();
    return true;
  }
  return false;
}

void ResourceLoadScheduler::SetOutstandingLimitForTesting(size_t limit) {
  SetOutstandingLimitAndMaybeRun(limit);
}

void ResourceLoadScheduler::OnNetworkQuiet() {
  DCHECK(IsMainThread());
  if (maximum_running_requests_seen_ == 0)
    return;

  DEFINE_STATIC_LOCAL(
      CustomCountHistogram, main_frame_throttled,
      ("Blink.ResourceLoadScheduler.PeakRequests.MainframeThrottled", 0,
       kMaximumReportSize10K, kReportBucket));
  DEFINE_STATIC_LOCAL(
      CustomCountHistogram, main_frame_not_throttled,
      ("Blink.ResourceLoadScheduler.PeakRequests.MainframeNotThrottled", 0,
       kMaximumReportSize10K, kReportBucket));
  DEFINE_STATIC_LOCAL(
      CustomCountHistogram, main_frame_partially_throttled,
      ("Blink.ResourceLoadScheduler.PeakRequests.MainframePartiallyThrottled",
       0, kMaximumReportSize10K, kReportBucket));
  DEFINE_STATIC_LOCAL(
      CustomCountHistogram, sub_frame_throttled,
      ("Blink.ResourceLoadScheduler.PeakRequests.SubframeThrottled", 0,
       kMaximumReportSize10K, kReportBucket));
  DEFINE_STATIC_LOCAL(
      CustomCountHistogram, sub_frame_not_throttled,
      ("Blink.ResourceLoadScheduler.PeakRequests.SubframeNotThrottled", 0,
       kMaximumReportSize10K, kReportBucket));
  DEFINE_STATIC_LOCAL(
      CustomCountHistogram, sub_frame_partially_throttled,
      ("Blink.ResourceLoadScheduler.PeakRequests.SubframePartiallyThrottled", 0,
       kMaximumReportSize10K, kReportBucket));

  switch (throttling_history_) {
    case ThrottlingHistory::kInitial:
    case ThrottlingHistory::kNotThrottled:
      if (context_->IsMainFrame())
        main_frame_not_throttled.Count(maximum_running_requests_seen_);
      else
        sub_frame_not_throttled.Count(maximum_running_requests_seen_);
      break;
    case ThrottlingHistory::kThrottled:
      if (context_->IsMainFrame())
        main_frame_throttled.Count(maximum_running_requests_seen_);
      else
        sub_frame_throttled.Count(maximum_running_requests_seen_);
      break;
    case ThrottlingHistory::kPartiallyThrottled:
      if (context_->IsMainFrame())
        main_frame_partially_throttled.Count(maximum_running_requests_seen_);
      else
        sub_frame_partially_throttled.Count(maximum_running_requests_seen_);
      break;
    case ThrottlingHistory::kStopped:
      break;
  }
}

void ResourceLoadScheduler::OnThrottlingStateChanged(
    WebFrameScheduler::ThrottlingState state) {
  // Flush traffic reports before activating another scheduling below.
  traffic_monitor_->Flush(state);

  switch (state) {
    case WebFrameScheduler::ThrottlingState::kThrottled:
      if (throttling_history_ == ThrottlingHistory::kInitial)
        throttling_history_ = ThrottlingHistory::kThrottled;
      else if (throttling_history_ == ThrottlingHistory::kNotThrottled)
        throttling_history_ = ThrottlingHistory::kPartiallyThrottled;
      SetOutstandingLimitAndMaybeRun(outstanding_throttled_limit_);
      break;
    case WebFrameScheduler::ThrottlingState::kNotThrottled:
      if (throttling_history_ == ThrottlingHistory::kInitial)
        throttling_history_ = ThrottlingHistory::kNotThrottled;
      else if (throttling_history_ == ThrottlingHistory::kThrottled)
        throttling_history_ = ThrottlingHistory::kPartiallyThrottled;
      SetOutstandingLimitAndMaybeRun(kOutstandingUnlimited);
      break;
    case WebFrameScheduler::ThrottlingState::kStopped:
      throttling_history_ = ThrottlingHistory::kStopped;
      SetOutstandingLimitAndMaybeRun(0u);
      break;
  }
}

ResourceLoadScheduler::ClientId ResourceLoadScheduler::GenerateClientId() {
  ClientId id = ++current_id_;
  CHECK_NE(0u, id);
  return id;
}

void ResourceLoadScheduler::MaybeRun() {
  // Requests for keep-alive loaders could be remained in the pending queue,
  // but ignore them once Shutdown() is called.
  if (is_shutdown_)
    return;

  while (!pending_requests_.empty()) {
    if (running_requests_.size() >= outstanding_limit_)
      return;
    ClientId id = pending_requests_.begin()->client_id;
    pending_requests_.erase(pending_requests_.begin());
    auto found = pending_request_map_.find(id);
    if (found == pending_request_map_.end())
      continue;  // Already released.
    ResourceLoadSchedulerClient* client = found->value;
    pending_request_map_.erase(found);
    Run(id, client);
  }
}

void ResourceLoadScheduler::Run(ResourceLoadScheduler::ClientId id,
                                ResourceLoadSchedulerClient* client) {
  running_requests_.insert(id);
  if (running_requests_.size() > maximum_running_requests_seen_) {
    maximum_running_requests_seen_ = running_requests_.size();
  }
  client->Run();
}

void ResourceLoadScheduler::SetOutstandingLimitAndMaybeRun(size_t limit) {
  outstanding_limit_ = limit;
  MaybeRun();
}

}  // namespace blink
