// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_PREDICTORS_GLOWPLUG_STATS_COLLECTOR_H_
#define CHROME_BROWSER_PREDICTORS_GLOWPLUG_STATS_COLLECTOR_H_

#include <map>
#include <memory>

#include "base/macros.h"
#include "chrome/browser/predictors/resource_prefetch_predictor.h"
#include "chrome/browser/predictors/resource_prefetcher.h"
#include "url/gurl.h"

namespace predictors {

namespace internal {
constexpr char kResourcePrefetchPredictorPrecisionHistogram[] =
    "ResourcePrefetchPredictor.LearningPrecision";
constexpr char kResourcePrefetchPredictorRecallHistogram[] =
    "ResourcePrefetchPredictor.LearningRecall";
constexpr char kResourcePrefetchPredictorCountHistogram[] =
    "ResourcePrefetchPredictor.LearningCount";
constexpr char kResourcePrefetchPredictorPrefetchMissesCountCached[] =
    "ResourcePrefetchPredictor.PrefetchMissesCount.Cached";
constexpr char kResourcePrefetchPredictorPrefetchMissesCountNotCached[] =
    "ResourcePrefetchPredictor.PrefetchMissesCount.NotCached";
constexpr char kResourcePrefetchPredictorPrefetchHitsCountCached[] =
    "ResourcePrefetchPredictor.PrefetchHitsCount.Cached";
constexpr char kResourcePrefetchPredictorPrefetchHitsCountNotCached[] =
    "ResourcePrefetchPredictor.PrefetchHitsCount.NotCached";
constexpr char kResourcePrefetchPredictorPrefetchHitsSize[] =
    "ResourcePrefetchPredictor.PrefetchHitsSizeKB";
constexpr char kResourcePrefetchPredictorPrefetchMissesSize[] =
    "ResourcePrefetchPredictor.PrefetchMissesSizeKB";
constexpr char kResourcePrefetchPredictorRedirectStatusHistogram[] =
    "ResourcePrefetchPredictor.RedirectStatus";
}  // namespace internal

class GlowplugStatsCollector {
 public:
  GlowplugStatsCollector(ResourcePrefetchPredictor* predictor,
                         const LoadingPredictorConfig& config);
  ~GlowplugStatsCollector();
  void RecordPrefetcherStats(
      std::unique_ptr<ResourcePrefetcher::PrefetcherStats> stats);
  void RecordPageRequestSummary(
      const ResourcePrefetchPredictor::PageRequestSummary& summary);
  void CleanupAbandonedStats();

 private:
  ResourcePrefetchPredictor* predictor_;
  LoadingPredictorConfig config_;
  std::map<GURL, std::unique_ptr<ResourcePrefetcher::PrefetcherStats>>
      prefetcher_stats_;

  DISALLOW_COPY_AND_ASSIGN(GlowplugStatsCollector);
};

}  // namespace predictors

#endif  // CHROME_BROWSER_PREDICTORS_GLOWPLUG_STATS_COLLECTOR_H_
