// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_UKM_UKM_RECORDER_IMPL_H_
#define COMPONENTS_UKM_UKM_RECORDER_IMPL_H_

#include <map>
#include <set>
#include <vector>

#include "base/sequence_checker.h"
#include "services/metrics/public/cpp/ukm_recorder.h"
#include "services/metrics/public/interfaces/ukm_interface.mojom.h"

namespace metrics {
class UkmBrowserTest;
}

namespace ukm {

class UkmSource;
class Report;

namespace debug {
class DebugPage;
}

class UkmRecorderImpl : public UkmRecorder {
 public:
  UkmRecorderImpl();
  ~UkmRecorderImpl() override;

  // Enables/disables recording control if data is allowed to be collected.
  void EnableRecording();
  void DisableRecording();

  // Deletes stored recordings.
  void Purge();

 protected:
  // Cache the list of whitelisted entries from the field trial parameter.
  void StoreWhitelistedEntries();

  // Writes recordings into a report proto, and clears recordings.
  void StoreRecordingsInReport(Report* report);

  const std::map<ukm::SourceId, std::unique_ptr<UkmSource>>& sources() const {
    return sources_;
  }

  const std::vector<mojom::UkmEntryPtr>& entries() const { return entries_; }

 private:
  friend ::metrics::UkmBrowserTest;
  friend ::ukm::debug::DebugPage;

  // This class provides keys that uniquely identify a metric's source & event.
  class SourceEventKey {
   public:
    explicit SourceEventKey(int64_t source_id)
        : source_id_(source_id), event_hash_(0) {}
    SourceEventKey(int64_t source_id, uint64_t event_hash)
        : source_id_(source_id), event_hash_(event_hash) {}

    bool operator<(const SourceEventKey& rhs) const {
      if (source_id_ != rhs.source_id_)
        return source_id_ < rhs.source_id_;
      return event_hash_ < rhs.event_hash_;
    }

    bool operator==(const SourceEventKey& rhs) const {
      return (source_id_ == rhs.source_id_ && event_hash_ == rhs.event_hash_);
    }

    int64_t source_id() const { return source_id_; }
    uint64_t event_hash() const { return event_hash_; }

   private:
    const int64_t source_id_;
    const uint64_t event_hash_;
  };

  struct MetricAggregate {
    double value_sum = 0;
    double value_square_sum = 0.0;
    uint64_t total_count = 0;
    uint64_t dropped_due_to_limits = 0;
    uint64_t dropped_due_to_sampling = 0;
  };

  using MetricAggregateMap = std::map<uint64_t, MetricAggregate>;

  // UkmRecorder:
  void UpdateSourceURL(SourceId source_id, const GURL& url) override;
  void AddEntry(mojom::UkmEntryPtr entry) override;

  // Whether recording new data is currently allowed.
  bool recording_enabled_;

  // Contains newly added sources and entries of UKM metrics which periodically
  // get serialized and cleared by BuildAndStoreLog().
  std::map<ukm::SourceId, std::unique_ptr<UkmSource>> sources_;
  std::vector<mojom::UkmEntryPtr> entries_;

  // Whitelisted Entry hashes, only the ones in this set will be recorded.
  std::set<uint64_t> whitelisted_entry_hashes_;

  // Aggregate information for collected metrics.
  std::map<SourceEventKey, MetricAggregateMap> metric_aggregations_;

  SEQUENCE_CHECKER(sequence_checker_);
};

}  // namespace ukm

#endif  // COMPONENTS_UKM_UKM_RECORDER_IMPL_H_
