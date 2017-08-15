// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/inspector/InspectorPerformanceAgent.h"

#include "core/frame/LocalDOMWindow.h"
#include "core/frame/LocalFrame.h"
#include "core/frame/PerformanceMonitor.h"
#include "core/inspector/InspectedFrames.h"
#include "core/timing/DOMWindowPerformance.h"
#include "core/timing/Performance.h"
#include "core/timing/PerformanceTiming.h"
#include "platform/InstanceCounters.h"
#include "platform/wtf/dtoa/utils.h"

namespace blink {

using protocol::Response;

#define DEFINE_PERF_METRIC_NAME(name) #name "Count",
const char* InspectorPerformanceAgent::page_metric_names_[] = {
    PERF_METRICS_LIST(DEFINE_PERF_METRIC_NAME)};

const char* InspectorPerformanceAgent::instance_metric_names_[] = {
    INSTANCE_COUNTERS_LIST(DEFINE_PERF_METRIC_NAME)};
#undef DEFINE_PERF_METRIC_NAME

InspectorPerformanceAgent::InspectorPerformanceAgent(
    InspectedFrames* inspected_frames)
    : inspected_frames_(inspected_frames) {}

InspectorPerformanceAgent::~InspectorPerformanceAgent() = default;

protocol::Response InspectorPerformanceAgent::enable() {
  enabled_ = true;
  instrumenting_agents_->addInspectorPerformanceAgent(this);
  return Response::OK();
}

protocol::Response InspectorPerformanceAgent::disable() {
  enabled_ = false;
  instrumenting_agents_->removeInspectorPerformanceAgent(this);
  return Response::OK();
}

namespace {
void AppendMetric(protocol::Array<protocol::Performance::Metric>* container,
                  const String& name,
                  double value) {
  container->addItem(protocol::Performance::Metric::create()
                         .setName(name)
                         .setValue(value)
                         .build());
}
}  // namespace

Response InspectorPerformanceAgent::getMetrics(
    std::unique_ptr<protocol::Array<protocol::Performance::Metric>>*
        out_result) {
  if (!enabled_) {
    *out_result = protocol::Array<protocol::Performance::Metric>::create();
    return Response::OK();
  }

  // Page performance metrics.
  PerformanceMonitor* performance_monitor =
      inspected_frames_->Root()->GetPerformanceMonitor();
  std::unique_ptr<protocol::Array<protocol::Performance::Metric>> result =
      protocol::Array<protocol::Performance::Metric>::create();
  for (size_t i = 0; i < ARRAY_SIZE(page_metric_names_); ++i) {
    double value = performance_monitor->PerfMetricValue(
        static_cast<PerformanceMonitor::MetricsType>(i));
    AppendMetric(result.get(), page_metric_names_[i], value);
  }

  // Renderer metrics.
  for (size_t i = 0; i < InstanceCounters::kCounterTypeLength; ++i) {
    int value = InstanceCounters::CounterValue(
        static_cast<InstanceCounters::CounterType>(i));
    AppendMetric(result.get(), instance_metric_names_[i], value);
  }

  // Performance timings.
  LocalDOMWindow* window = inspected_frames_->Root()->DomWindow();
  if (window) {
    Performance* performance = DOMWindowPerformance::performance(*window);
    PerformanceTiming* timing = performance->timing();
#define APPEND_PERFORMANCE_TIMING_VALUE(name) \
  AppendMetric(result.get(), #name, timing->name());
    PERFORMANCE_TIMING_LIST(APPEND_PERFORMANCE_TIMING_VALUE)
#undef APPEND_PERFORMANCE_TIMING_VALUE
  }

  *out_result = std::move(result);
  return Response::OK();
}

void InspectorPerformanceAgent::ConsoleTimeStamp(const String& title) {
  if (!enabled_)
    return;
  std::unique_ptr<protocol::Array<protocol::Performance::Metric>> metrics;
  getMetrics(&metrics);
  GetFrontend()->metrics(std::move(metrics), title);
}

DEFINE_TRACE(InspectorPerformanceAgent) {
  visitor->Trace(inspected_frames_);
  InspectorBaseAgent<protocol::Performance::Metainfo>::Trace(visitor);
}

}  // namespace blink
