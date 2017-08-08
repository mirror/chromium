// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/inspector/InspectorPerformanceAgent.h"

#include "core/frame/LocalFrame.h"
#include "core/inspector/InspectedFrames.h"
#include "platform/wtf/dtoa/utils.h"

namespace blink {

using protocol::Response;

const char* InspectorPerformanceAgent::metric_names_[] = {
#define DEFINE_PERF_METRIC_NAME(name) #name,
    PERF_METRICS_LIST(DEFINE_PERF_METRIC_NAME)
#undef DEFINE_PERF_METRIC_NAME
};

InspectorPerformanceAgent::InspectorPerformanceAgent(
    InspectedFrames* inspected_frames)
    : performance_monitor_(inspected_frames->Root()->GetPerformanceMonitor()) {
  for (size_t i = 0; i < ARRAY_SIZE(metric_names_); ++i) {
    metric_names_map_.insert(std::make_pair(
        metric_names_[i], static_cast<PerformanceMonitor::MetricsType>(i)));
  }
}

InspectorPerformanceAgent::~InspectorPerformanceAgent() = default;

Response InspectorPerformanceAgent::enableMetrics(
    std::unique_ptr<protocol::Array<protocol::String>> metrics) {
  enabled_metrics_.clear();
  for (size_t i = 0, c = metrics->length(); i < c; ++i) {
    auto it = metric_names_map_.find(metrics->get(i));
    if (it != metric_names_map_.end())
      enabled_metrics_.insert(it->second);
  }
  return Response::OK();
}

Response InspectorPerformanceAgent::getMetrics(
    std::unique_ptr<protocol::Array<protocol::Performance::Metric>>*
        out_result) {
  std::unique_ptr<protocol::Array<protocol::Performance::Metric>> result =
      protocol::Array<protocol::Performance::Metric>::create();
  for (auto it : enabled_metrics_) {
    result->addItem(protocol::Performance::Metric::create()
                        .setName(metric_names_[it])
                        .setValue(performance_monitor_->PerfMetricValue(it))
                        .build());
  }
  *out_result = std::move(result);
  return Response::OK();
}

DEFINE_TRACE(InspectorPerformanceAgent) {
  visitor->Trace(performance_monitor_);
  InspectorBaseAgent<protocol::Performance::Metainfo>::Trace(visitor);
}

}  // namespace blink
