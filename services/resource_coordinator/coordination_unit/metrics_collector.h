// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_RESOURCE_COORDINATOR_COORDINATION_UNIT_METRICS_COLLECTOR_H_
#define SERVICES_RESOURCE_COORDINATOR_COORDINATION_UNIT_METRICS_COLLECTOR_H_

#include "base/macros.h"
#include "base/metrics/histogram_macros.h"
#include "base/time/default_tick_clock.h"
#include "base/time/time.h"
#include "services/metrics/public/cpp/ukm_builders.h"
#include "services/resource_coordinator/coordination_unit/coordination_unit_graph_observer.h"

// Tabs can be kept in the background for a long time, metrics show 75th
// percentile of time spent in background is 2.5 hours, and the 95th is 24 hour.
// In order to guide the selection of an appropriate observation window we are
// proposing using a CUSTOM_TIMES histogram from 1s to 48h, with 100 buckets.
#define HEURISTICS_HISTOGRAM(name, sample)                                  \
  UMA_HISTOGRAM_CUSTOM_TIMES(name, sample, base::TimeDelta::FromSeconds(1), \
                             base::TimeDelta::FromHours(48), 100)

namespace ukm {

class MojoUkmRecorder;

}  // namespace ukm

namespace resource_coordinator {

class CoordinationUnitImpl;
class FrameCoordinationUnitImpl;
class WebContentsCoordinationUnitImpl;

extern const char kTabFromBackgroundedToFirstAlertFiredUMA[];
extern const char kTabFromBackgroundedToFirstAudioStartsUMA[];
extern const char kTabFromBackgroundedToFirstFaviconUpdatedUMA[];
extern const char kTabFromBackgroundedToFirstTitleUpdatedUMA[];
extern const char
    kTabFromBackgroundedToFirstNonPersistentNotificationCreatedUMA[];
extern const base::TimeDelta kMaxAudioSlientTimeout;
extern const base::TimeDelta kMetricsReportDelayTimeout;

// A MetricsCollector observes changes happened inside CoordinationUnit Graph,
// and reports UMA/UKM.
class MetricsCollector : public CoordinationUnitGraphObserver {
 public:
  MetricsCollector();
  ~MetricsCollector() override;

  // CoordinationUnitGraphObserver implementation.
  bool ShouldObserve(const CoordinationUnitImpl* coordination_unit) override;
  void OnCoordinationUnitCreated(
      const CoordinationUnitImpl* coordination_unit) override;
  void OnBeforeCoordinationUnitDestroyed(
      const CoordinationUnitImpl* coordination_unit) override;
  void OnFramePropertyChanged(const FrameCoordinationUnitImpl* frame_cu,
                              const mojom::PropertyType property_type,
                              int64_t value) override;
  void OnWebContentsPropertyChanged(
      const WebContentsCoordinationUnitImpl* web_contents_cu,
      const mojom::PropertyType property_type,
      int64_t value) override;
  void OnFrameEventReceived(const FrameCoordinationUnitImpl* frame_cu,
                            const mojom::Event event) override;
  void OnWebContentsEventReceived(
      const WebContentsCoordinationUnitImpl* web_contents_cu,
      const mojom::Event event) override;

 private:
  friend class MetricsCollectorTest;

  template <class UKMBuilderClass>
  class BackgroundMetricsReporter {
   public:
    BackgroundMetricsReporter(
        const char* metrics_name,
        const WebContentsCoordinationUnitImpl* web_contents_cu)
        : metrics_name_(metrics_name),
          web_contents_cu_(web_contents_cu),
          ukm_source_id_(-1),
          reported_(false),
          ukm_reported_(false) {}

    virtual void Reset() {
      reported_ = false;
      ukm_reported_ = false;
    }

    void SetUKMSourceID(int64_t ukm_source_id) {
      ukm_source_id_ = ukm_source_id;
    }

    void OnSignalReceived(bool is_main_frame,
                          base::TimeDelta duration,
                          ukm::MojoUkmRecorder* ukm_recorder) {
      if (!ShouldReport(is_main_frame))
        return;

      if (!reported_) {
        reported_ = true;
        HEURISTICS_HISTOGRAM(metrics_name_, duration);
      }
      ReportUKMIfNeeded(is_main_frame, duration, ukm_recorder);
    }

   protected:
    virtual bool ShouldReport(bool is_main_frame) {
      return !reported_ || !ukm_reported_;
    }

    virtual void ReportUKMIfNeeded(bool is_main_frame,
                                   base::TimeDelta duration,
                                   ukm::MojoUkmRecorder* ukm_recorder) {
      if (ukm_source_id_ == -1 || ukm_reported_)
        return;

      UKMBuilderClass ukm_builder(ukm_source_id_);
      ukm_builder.SetTimeFromBackgrounded(duration.InMilliseconds())
          .Record(ukm_recorder);
      ukm_reported_ = true;
    }

    const char* metrics_name_;
    const WebContentsCoordinationUnitImpl* web_contents_cu_;
    int64_t ukm_source_id_;
    bool reported_;

   private:
    bool ukm_reported_;
  };

  template <class UKMBuilderClass>
  class BackgroundMetricsReporterWithChildFrame
      : public BackgroundMetricsReporter<UKMBuilderClass> {
   public:
    BackgroundMetricsReporterWithChildFrame(
        const char* metrics_name,
        const WebContentsCoordinationUnitImpl* web_contents_cu)
        : BackgroundMetricsReporter<UKMBuilderClass>(metrics_name,
                                                     web_contents_cu),
          main_frame_ukm_reported_(false),
          child_frame_ukm_reported_(false) {}

    void Reset() override {
      this->reported_ = false;
      main_frame_ukm_reported_ = false;
      child_frame_ukm_reported_ = false;
    }

   private:
    bool ShouldReport(bool is_main_frame) override {
      return !this->reported_ || ShouldReportMainFrameUKM(is_main_frame) ||
             ShouldReportChildFrameUKM(is_main_frame);
    }

    void ReportUKMIfNeeded(bool is_main_frame,
                           base::TimeDelta duration,
                           ukm::MojoUkmRecorder* ukm_recorder) override {
      if (this->ukm_source_id_ == -1 ||
          (!ShouldReportMainFrameUKM(is_main_frame) &&
           !ShouldReportChildFrameUKM(is_main_frame))) {
        return;
      }

      UKMBuilderClass ukm_builder(this->ukm_source_id_);
      ukm_builder.SetIsMainFrame(is_main_frame);
      ukm_builder.SetTimeFromBackgrounded(duration.InMilliseconds())
          .Record(ukm_recorder);
      if (is_main_frame) {
        main_frame_ukm_reported_ = true;
      } else {
        child_frame_ukm_reported_ = true;
      }
    }

    bool ShouldReportMainFrameUKM(bool is_main_frame) const {
      return is_main_frame && !main_frame_ukm_reported_;
    }
    bool ShouldReportChildFrameUKM(bool is_main_frame) const {
      return !is_main_frame && !child_frame_ukm_reported_;
    }

    bool main_frame_ukm_reported_;
    bool child_frame_ukm_reported_;
  };

  struct MetricsReportRecord {
    explicit MetricsReportRecord(
        const WebContentsCoordinationUnitImpl* web_contents_cu);
    MetricsReportRecord(const MetricsReportRecord& other);
    void UpdateUKMSourceID(int64_t ukm_source_id);
    void Reset();
    BackgroundMetricsReporterWithChildFrame<
        ukm::builders::TabManager_Background_FirstAlertFired>
        first_alert_fired;
    BackgroundMetricsReporterWithChildFrame<
        ukm::builders::TabManager_Background_FirstAudioStarts>
        first_audible;
    BackgroundMetricsReporter<
        ukm::builders::TabManager_Background_FirstFaviconUpdated>
        first_favicon_updated;
    BackgroundMetricsReporterWithChildFrame<
        ukm::builders::
            TabManager_Background_FirstNonPersistentNotificationCreated>
        first_non_persistent_notification_created;
    BackgroundMetricsReporter<
        ukm::builders::TabManager_Background_FirstTitleUpdated>
        first_title_updated;
  };

  struct FrameData {
    base::TimeTicks last_audible_time;
  };

  struct WebContentsData {
    base::TimeTicks last_invisible_time;
    base::TimeTicks navigation_finished_time;
  };

  struct UkmCPUUsageCollectionState {
    size_t num_cpu_usage_measurements = 0u;
    // |ukm::UkmRecorder::GetNewSourceID| monotonically increases starting at 0,
    // so -1 implies that the current |ukm_source_id| is invalid.
    ukm::SourceId ukm_source_id = -1;
  };

  bool ShouldReportMetrics(
      const WebContentsCoordinationUnitImpl* web_contents_cu);
  bool IsCollectingCPUUsageForUkm(const CoordinationUnitID& web_contents_cu_id);
  void RecordCPUUsageForUkm(const CoordinationUnitID& web_contents_cu_id,
                            double cpu_usage,
                            size_t num_coresident_tabs);
  void UpdateUkmSourceIdForWebContents(
      const CoordinationUnitID& web_contents_cu_id,
      ukm::SourceId ukm_source_id);
  void UpdateWithFieldTrialParams();
  void ResetMetricsReportRecord(CoordinationUnitID cu_id);

  // Note: |clock_| is always |&default_tick_clock_|, except during unit
  // testing.
  base::DefaultTickClock default_tick_clock_;
  base::TickClock* const clock_;
  std::map<CoordinationUnitID, FrameData> frame_data_map_;
  std::map<CoordinationUnitID, WebContentsData> web_contents_data_map_;
  // The metrics_report_record_map_ is used to record whether a metric was
  // already reported to avoid reporting multiple metrics.
  std::map<CoordinationUnitID, MetricsReportRecord> metrics_report_record_map_;
  std::map<CoordinationUnitID, UkmCPUUsageCollectionState>
      ukm_cpu_usage_collection_state_map_;
  size_t max_ukm_cpu_usage_measurements_ = 0u;
  DISALLOW_COPY_AND_ASSIGN(MetricsCollector);
};

}  // namespace resource_coordinator

#endif  // SERVICES_RESOURCE_COORDINATOR_COORDINATION_UNIT_METRICS_COLLECTOR_H_
