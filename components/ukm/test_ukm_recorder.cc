// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/ukm/test_ukm_recorder.h"

#include <algorithm>
#include <iterator>

#include "base/logging.h"
#include "base/metrics/metrics_hashes.h"
#include "base/task_scheduler/post_task.h"
#include "base/threading/sequenced_task_runner_handle.h"
#include "components/ukm/ukm_source.h"
#include "services/metrics/public/cpp/delegating_ukm_recorder.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace ukm {

namespace {

// Merge the data from entry1 to entry2.
void MergeEntry(mojom::UkmEntry* entry1, const mojom::UkmEntry* entry2) {
  if (entry1->event_hash) {
    EXPECT_EQ(entry1->source_id, entry2->source_id);
    EXPECT_EQ(entry1->event_hash, entry2->event_hash);
  } else {
    entry1->event_hash = entry2->event_hash;
    entry1->source_id = entry2->source_id;
  }
  for (const auto& metric : entry2->metrics) {
    entry1->metrics.emplace_back(metric->Clone());
  }
}

// Provides a single merged ukm::mojom::UkmEntry proto that contains all metrics
// from the given |entries|. |entries| must be non-empty, and all |entries| must
// have the same |source_id| and |event_hash|.
mojom::UkmEntryPtr GetMergedEntry(
    const std::vector<const mojom::UkmEntry*>& entries) {
  EXPECT_FALSE(entries.empty());
  mojom::UkmEntryPtr merged_entry = mojom::UkmEntry::New();
  for (const auto* entry : entries) {
    MergeEntry(merged_entry.get(), entry);
  }
  return merged_entry;
}

std::vector<int64_t> GetValuesForMetric(const mojom::UkmEntry* entry,
                                        const char* metric_name) {
  std::vector<int64_t> values;
  const uint64_t metric_hash = base::HashMetricName(metric_name);
  for (const auto& metric : entry->metrics) {
    if (metric->metric_hash == metric_hash)
      values.push_back(metric->value);
  }
  return values;
}

}  // namespace

TestUkmRecorder::TestUkmRecorder() {
  EnableRecording();
}

TestUkmRecorder::~TestUkmRecorder() {
};

const UkmSource* TestUkmRecorder::GetSourceForSourceId(
    SourceId source_id) const {
  const UkmSource* source = nullptr;
  for (const auto& kv : sources()) {
    if (kv.second->id() == source_id) {
      DCHECK_EQ(nullptr, source);
      source = kv.second.get();
    }
  }
  return source;
}

const mojom::UkmEntry* TestUkmRecorder::GetEntry(size_t entry_num) const {
  DCHECK_LT(entry_num, entries().size());
  return entries()[entry_num].get();
}

std::vector<const mojom::UkmEntry*> TestUkmRecorder::GetEntriesByName(
    base::StringPiece entry_name) const {
  uint64_t hash = base::HashMetricName(entry_name);
  std::vector<const mojom::UkmEntry*> result;
  for (const auto& it : entries()) {
    if (it->event_hash == hash)
      result.push_back(it.get());
  }
  return result;
}

std::map<ukm::SourceId, mojom::UkmEntryPtr>
TestUkmRecorder::GetMergedEntriesByName(base::StringPiece entry_name) const {
  uint64_t hash = base::HashMetricName(entry_name);
  std::map<ukm::SourceId, mojom::UkmEntryPtr> result;
  for (const auto& it : entries()) {
    if (it->event_hash != hash)
      continue;
    mojom::UkmEntryPtr& entry_ptr = result[it->source_id];
    if (!entry_ptr)
      entry_ptr = mojom::UkmEntry::New();
    MergeEntry(entry_ptr.get(), it.get());
  }
  return result;
}

// static
bool TestUkmRecorder::EntryHasMetric(const mojom::UkmEntry* entry,
                                     base::StringPiece metric_name) {
  return FindMetric(entry, metric_name) != nullptr;
}

void TestUkmRecorder::ExpectEntrySourceHasUrl(const mojom::UkmEntry* entry,
                                              const GURL& url) const {
  const UkmSource* src = GetSourceForSourceId(entry->source_id);
  if (src == nullptr) {
    FAIL() << "Entry source id has no associated Source.";
    return;
  }
  EXPECT_EQ(src->url(), url);
}

// static
const int64_t* TestUkmRecorder::GetEntryMetric(const mojom::UkmEntry* entry,
                                               base::StringPiece metric_name) {
  const mojom::UkmMetric* metric = FindMetric(entry, metric_name);
  return metric ? &metric->value : nullptr;
}

// static
void TestUkmRecorder::ExpectEntryMetric(const mojom::UkmEntry* entry,
                                        base::StringPiece metric_name,
                                        int64_t expected_value) {
  const mojom::UkmMetric* metric = FindMetric(entry, metric_name);
  if (metric == nullptr) {
    FAIL() << "Failed to find metric for event: " << metric_name;
    return;
  }
  EXPECT_EQ(metric->value, expected_value) << " for metric:" << metric_name;
}

// static
const mojom::UkmMetric* TestUkmRecorder::FindMetric(
    const mojom::UkmEntry* entry,
    base::StringPiece metric_name) {
  uint64_t hash = base::HashMetricName(metric_name);
  for (const auto& metric : entry->metrics) {
    if (metric->metric_hash == hash)
      return metric.get();
  }
  return nullptr;
}

std::vector<const mojom::UkmEntry*> TestUkmRecorder::GetEntriesForSourceID(
    SourceId source_id,
    const char* event_name) const {
  const uint64_t event_hash = base::HashMetricName(event_name);
  std::vector<const mojom::UkmEntry*> entries;
  for (size_t i = 0; i < entries_count(); ++i) {
    const mojom::UkmEntry* entry = GetEntry(i);
    if (entry->source_id == source_id && entry->event_hash == event_hash) {
      entries.push_back(entry);
    }
  }
  return entries;
}

mojom::UkmEntryPtr TestUkmRecorder::GetMergedEntryForSourceID(
    SourceId source_id,
    const char* event_name) const {
  mojom::UkmEntryPtr entry =
      GetMergedEntry(GetEntriesForSourceID(source_id, event_name));
  EXPECT_EQ(source_id, entry->source_id);
  EXPECT_EQ(base::HashMetricName(event_name), entry->event_hash);
  return entry;
}

bool TestUkmRecorder::HasEntry(const UkmSource& source,
                               const char* event_name) const {
  return CountEntries(source, event_name) > 0;
}

int TestUkmRecorder::CountEntries(const UkmSource& source,
                                  const char* event_name) const {
  return GetEntriesForSourceID(source.id(), event_name).size();
}

std::vector<int64_t> TestUkmRecorder::GetMetricValues(
    ukm::SourceId source_id,
    const char* event_name,
    const char* metric_name) const {
  mojom::UkmEntryPtr entry = GetMergedEntryForSourceID(source_id, event_name);
  return GetValuesForMetric(entry.get(), metric_name);
}

std::vector<int64_t> TestUkmRecorder::GetMetrics(
    const UkmSource& source,
    const char* event_name,
    const char* metric_name) const {
  return GetMetricValues(source.id(), event_name, metric_name);
}

TestAutoSetUkmRecorder::TestAutoSetUkmRecorder() : self_ptr_factory_(this) {
  DelegatingUkmRecorder::Get()->AddDelegate(self_ptr_factory_.GetWeakPtr());
}

TestAutoSetUkmRecorder::~TestAutoSetUkmRecorder() {
  DelegatingUkmRecorder::Get()->RemoveDelegate(this);
};

}  // namespace ukm
