// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UkmTimeAggregator_h
#define UkmTimeAggregator_h

#include "platform/PlatformExport.h"
#include "platform/wtf/Time.h"
#include "platform/wtf/Vector.h"
#include "platform/wtf/text/WTFString.h"

namespace ukm {
class UkmRecorder;
}

namespace blink {
class CustomCountHistogram;

// This class is a helper class for aggregating and recording time based UKM
// metrics
class PLATFORM_EXPORT UkmTimeAggregator {
 public:
  class PLATFORM_EXPORT ScopedUkmTimer {
   public:
    ScopedUkmTimer(ScopedUkmTimer&&);
    ~ScopedUkmTimer();

   private:
    friend class UkmTimeAggregator;

    ScopedUkmTimer(UkmTimeAggregator*,
                   size_t metric_index,
                   CustomCountHistogram* histogram_counter);

    UkmTimeAggregator* aggregator_;
    const size_t metric_index_;
    CustomCountHistogram* const histogram_counter_;
    const TimeTicks start_time_;

    DISALLOW_COPY_AND_ASSIGN(ScopedUkmTimer);
  };

  UkmTimeAggregator(String event_name,
                    int64_t source_id,
                    ukm::UkmRecorder*,
                    const Vector<String>& metric_names,
                    TimeDelta event_frequency);
  ~UkmTimeAggregator();

  ScopedUkmTimer GetScopedTimer(
      size_t metric_index,
      CustomCountHistogram* histogram_counter = nullptr);

 private:
  struct MetricRecord {
    String worst_case_metric_name;
    String average_metric_name;
    TimeDelta total_duration;
    TimeDelta worst_case_duration;
    size_t sample_count = 0u;

    void reset() {
      total_duration = TimeDelta();
      worst_case_duration = TimeDelta();
      sample_count = 0u;
    }
  };

  void RecordSample(size_t metric_index,
                    TimeTicks start,
                    TimeTicks end,
                    CustomCountHistogram* histogram_counter);
  void FlushIfNeeded(TimeTicks current_time);
  void Flush(TimeTicks current_time);

  const String event_name_;
  const int64_t source_id_;
  ukm::UkmRecorder* const recorder_;
  const TimeDelta event_frequency_;
  TimeTicks last_flushed_time_;
  Vector<MetricRecord> metric_records_;
  bool has_data_ = false;
};

}  // namespace blink

#endif  // UkmTimeAggregator_h
