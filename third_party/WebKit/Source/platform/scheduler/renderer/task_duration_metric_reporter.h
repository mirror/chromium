// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_WEBKIT_SOURCE_PLATFORM_SCHEDULER_RENDERER_TASK_DURATION_METRIC_REPORTER_H_
#define THIRD_PARTY_WEBKIT_SOURCE_PLATFORM_SCHEDULER_RENDERER_TASK_DURATION_METRIC_REPORTER_H_

#include <array>

#include "base/gtest_prod_util.h"
#include "base/macros.h"
#include "base/metrics/histogram.h"
#include "base/time/time.h"
#include "platform/PlatformExport.h"
#include "platform/scheduler/renderer/main_thread_task_queue.h"

namespace base {
class HistogramBase;
}

namespace blink {
namespace scheduler {

// A helper class to report task duration split by queue type.
// Aggregates small tasks internally and reports only whole milliseconds.
//
// |TaskClass| is an enum which should have COUNT field.
// All values reported to RecordTask should have lower values.
template <class TaskClass>
class PLATFORM_EXPORT TaskDurationMetricReporter {
 public:
  explicit TaskDurationMetricReporter(const char* metric_name)
      : metric_name_(metric_name) {}

  ~TaskDurationMetricReporter() {}

  void RecordTask(TaskClass task_class, base::TimeDelta duration) {
    RecordTaskWithPostfix(task_class, "", duration);
  }

  void RecordTaskWithPostfix(TaskClass task_class,
                             const char* postfix,
                             base::TimeDelta duration) {
    DCHECK_LT(static_cast<int>(task_class),
              static_cast<int>(TaskClass::kCount));
    // Report only whole milliseconds to avoid overflow.
    base::TimeDelta& unreported_duration =
        unreported_task_duration_[static_cast<int>(task_class)];
    unreported_duration += duration;
    int64_t milliseconds = unreported_duration.InMilliseconds();
    if (milliseconds > 0) {
      unreported_duration -= base::TimeDelta::FromMilliseconds(milliseconds);
      GetHistogram(postfix)->AddCount(static_cast<int>(task_class),
                                      static_cast<int>(milliseconds));
    }
  }

 private:
  FRIEND_TEST_ALL_PREFIXES(TaskDurationMetricReporterTest, Test);

  base::HistogramBase* GetHistogram(const char* postfix) {
    auto pair = histograms_.find(postfix);
    if (pair != histograms_.end())
      return pair->second;

    base::HistogramBase* histogram = base::Histogram::FactoryGet(
        metric_name_ + postfix, 1, static_cast<int>(TaskClass::kCount),
        static_cast<int>(TaskClass::kCount) + 1,
        base::HistogramBase::kUmaTargetedHistogramFlag);
    histograms_[postfix] = histogram;
    return histogram;
  }

  void SetHistogramForTesting(const char* postfix,
                              base::HistogramBase* histogram) {
    histograms_[postfix] = histogram;
  }

  std::array<base::TimeDelta, static_cast<size_t>(TaskClass::kCount)>
      unreported_task_duration_;

  std::string metric_name_;
  std::map<std::string, base::HistogramBase*> histograms_;

  DISALLOW_COPY_AND_ASSIGN(TaskDurationMetricReporter);
};

}  // namespace scheduler
}  // namespace blink

#endif  // THIRD_PARTY_WEBKIT_SOURCE_PLATFORM_SCHEDULER_RENDERER_TASK_DURATION_METRIC_REPORTER_H_
