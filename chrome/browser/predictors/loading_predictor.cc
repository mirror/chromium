// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/predictors/loading_predictor.h"

#include "base/memory/ptr_util.h"
#include "base/metrics/histogram_macros.h"
#include "chrome/browser/predictors/loading_data_collector.h"
#include "chrome/browser/predictors/loading_stats_collector.h"
#include "chrome/browser/predictors/resource_prefetch_common.h"
#include "chrome/browser/predictors/resource_prefetch_predictor.h"

namespace predictors {

using URLRequestSummary = ResourcePrefetchPredictor::URLRequestSummary;

LoadingPredictor::LoadingPredictor(const LoadingPredictorConfig& config,
                                   Profile* profile)
    : config_(config),
      profile_(profile),
      resource_prefetch_predictor_(
          base::MakeUnique<ResourcePrefetchPredictor>(config, profile)),
      stats_collector_(base::MakeUnique<LoadingStatsCollector>(
          resource_prefetch_predictor_.get(),
          config)),
      loading_data_collector_(base::MakeUnique<LoadingDataCollector>(
          resource_prefetch_predictor())),
      weak_factory_(this) {
  resource_prefetch_predictor_->SetStatsCollector(stats_collector_.get());
}

LoadingPredictor::~LoadingPredictor() = default;

void LoadingPredictor::PrepareForPageLoad(const GURL& url, HintOrigin origin) {
  if (active_hints_.find(url) != active_hints_.end())
    return;

  std::vector<HintType> hint_types;

  bool prefetching = MaybeAddPrefetch(url, origin, &hint_types);
  if (!prefetching)
    MaybeAddPreconnectAndPreresolve(url, origin, &hint_types);

  if (!hint_types.empty()) {
    // To report hint durations.
    active_hints_.emplace(url,
                          std::make_pair(hint_types, base::TimeTicks::Now()));
  }
}

void LoadingPredictor::CancelPageLoadHint(const GURL& url) {
  CancelActiveHint(active_hints_.find(url));
}

void LoadingPredictor::StartInitialization() {
  resource_prefetch_predictor_->StartInitialization();
}

LoadingDataCollector* LoadingPredictor::loading_data_collector() {
  return loading_data_collector_.get();
}

ResourcePrefetchPredictor* LoadingPredictor::resource_prefetch_predictor() {
  return resource_prefetch_predictor_.get();
}

void LoadingPredictor::Shutdown() {
  resource_prefetch_predictor_->Shutdown();
}

void LoadingPredictor::OnMainFrameRequest(const URLRequestSummary& summary) {
  DCHECK(summary.resource_type == content::RESOURCE_TYPE_MAIN_FRAME);

  const NavigationID& navigation_id = summary.navigation_id;
  CleanupAbandonedHintsAndNavigations(navigation_id);
  active_navigations_.emplace(navigation_id, navigation_id.main_frame_url);
  PrepareForPageLoad(navigation_id.main_frame_url, HintOrigin::NAVIGATION);
}

void LoadingPredictor::OnMainFrameRedirect(const URLRequestSummary& summary) {
  DCHECK(summary.resource_type == content::RESOURCE_TYPE_MAIN_FRAME);

  auto it = active_navigations_.find(summary.navigation_id);
  if (it != active_navigations_.end()) {
    if (summary.navigation_id.main_frame_url == summary.redirect_url)
      return;
    NavigationID navigation_id = summary.navigation_id;
    navigation_id.main_frame_url = summary.redirect_url;
    active_navigations_.emplace(navigation_id, it->second);
    active_navigations_.erase(it);
  }
}

void LoadingPredictor::OnMainFrameResponse(const URLRequestSummary& summary) {
  DCHECK(summary.resource_type == content::RESOURCE_TYPE_MAIN_FRAME);

  const NavigationID& navigation_id = summary.navigation_id;
  auto it = active_navigations_.find(navigation_id);
  if (it != active_navigations_.end()) {
    const GURL& initial_url = it->second;
    CancelPageLoadHint(initial_url);
    active_navigations_.erase(it);
  } else {
    CancelPageLoadHint(navigation_id.main_frame_url);
  }
}

bool LoadingPredictor::MaybeAddPrefetch(const GURL& url,
                                        HintOrigin origin,
                                        std::vector<HintType>* hint_types) {
  ResourcePrefetchPredictor::Prediction prediction;
  if (!resource_prefetch_predictor_->GetPrefetchData(url, &prediction))
    return false;

  if (config_.IsPrefetchingEnabledForOrigin(profile_, origin))
    resource_prefetch_predictor_->StartPrefetching(url, prediction);

  hint_types->push_back(HintType::PREFETCH);
  return true;
}

bool LoadingPredictor::MaybeAddPreconnectAndPreresolve(
    const GURL& url,
    HintOrigin origin,
    std::vector<HintType>* hint_types) {
  PreconnectPrediction prediction;
  if (!resource_prefetch_predictor_->PredictPreconnectOrigins(url, &prediction))
    return false;

  if (config_.IsPreconnectEnabledForOrigin(profile_, origin)) {
    // Not implemented.
  }

  if (!prediction.preconnect_origins.empty())
    hint_types->push_back(HintType::PRECONNECT);
  if (!prediction.preresolve_hosts.empty())
    hint_types->push_back(HintType::PRERESOLVE);
  return true;
}

LoadingPredictor::HintMap::iterator LoadingPredictor::CancelActiveHint(
    LoadingPredictor::HintMap::iterator hint_it) {
  if (hint_it == active_hints_.end())
    return hint_it;

  const GURL& url = hint_it->first;
  for (HintType hint_type : hint_it->second.first) {
    switch (hint_type) {
      case HintType::PREFETCH:
        resource_prefetch_predictor_->StopPrefetching(url);
        UMA_HISTOGRAM_TIMES(
            internal::kResourcePrefetchPredictorPrefetchingDurationHistogram,
            base::TimeTicks::Now() - hint_it->second.second);
        break;
      case HintType::PRECONNECT:
      case HintType::PRERESOLVE:
        // Not implemented.
        break;
    }
  }

  return active_hints_.erase(hint_it);
}

void LoadingPredictor::CleanupAbandonedHintsAndNavigations(
    const NavigationID& navigation_id) {
  base::TimeTicks time_now = base::TimeTicks::Now();
  const base::TimeDelta max_navigation_age =
      base::TimeDelta::FromSeconds(config_.max_navigation_lifetime_seconds);

  // Hints.
  for (auto it = active_hints_.begin(); it != active_hints_.end();) {
    base::TimeDelta prefetch_age = time_now - it->second.second;
    if (prefetch_age > max_navigation_age) {
      // Will go to the last bucket in the duration reported in
      // CancelActiveHint() meaning that the duration was unlimited.
      it = CancelActiveHint(it);
    } else {
      ++it;
    }
  }

  // Navigations.
  for (auto it = active_navigations_.begin();
       it != active_navigations_.end();) {
    if ((it->first.tab_id == navigation_id.tab_id) ||
        (time_now - it->first.creation_time > max_navigation_age)) {
      const GURL& initial_url = it->second;
      CancelActiveHint(active_hints_.find(initial_url));
      it = active_navigations_.erase(it);
    } else {
      ++it;
    }
  }
}

}  // namespace predictors
