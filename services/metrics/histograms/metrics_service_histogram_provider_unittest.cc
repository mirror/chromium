// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/metrics/histograms/metrics_service_histogram_provider.h"

#include <memory>
#include <string>

#include "base/memory/ptr_util.h"
#include "base/metrics/histogram.h"
#include "base/metrics/histogram_flattener.h"
#include "base/metrics/histogram_snapshot_manager.h"
#include "base/metrics/persistent_histogram_allocator.h"
#include "base/metrics/persistent_memory_allocator.h"
#include "base/metrics/statistics_recorder.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace metrics {
namespace {

const uint32_t TEST_MEMORY_SIZE = 64 << 10;  // 64 KiB

class HistogramFlattenerDeltaRecorder : public base::HistogramFlattener {
 public:
  HistogramFlattenerDeltaRecorder() {}

  void RecordDelta(const base::HistogramBase& histogram,
                   const base::HistogramSamples& snapshot) override {
    recorded_delta_histogram_names_.push_back(histogram.histogram_name());
  }

  void InconsistencyDetected(
      base::HistogramBase::Inconsistency problem) override {
    ASSERT_TRUE(false);
  }

  void UniqueInconsistencyDetected(
      base::HistogramBase::Inconsistency problem) override {
    ASSERT_TRUE(false);
  }

  void InconsistencyDetectedInLoggedCount(int amount) override {
    ASSERT_TRUE(false);
  }

  std::vector<std::string> GetRecordedDeltaHistogramNames() {
    return recorded_delta_histogram_names_;
  }

 private:
  std::vector<std::string> recorded_delta_histogram_names_;

  DISALLOW_COPY_AND_ASSIGN(HistogramFlattenerDeltaRecorder);
};

}  // namespace

class MetricsServiceHistogramProviderTest : public testing::Test {
 protected:
  MetricsServiceHistogramProviderTest() {
    // Get this first so it isn't created inside a persistent allocator.
    base::PersistentHistogramAllocator::GetCreateHistogramResultHistogram();

    // MergeHistogramDeltas() uses a histogram macro which caches a pointer
    // to a histogram. If not called before setting a persistent global
    // allocator, then it would point into memory that will go away.
    provider_.MergeHistogramDeltasForTest();

    // Create a dedicated StatisticsRecorder for this test.
    test_recorder_ = base::StatisticsRecorder::CreateTemporaryForTesting();

    // Create a global allocator using a block of memory from the heap.
    base::GlobalHistogramAllocator::CreateWithLocalMemory(TEST_MEMORY_SIZE, 0,
                                                          "");
  }

  ~MetricsServiceHistogramProviderTest() override {
    base::GlobalHistogramAllocator::ReleaseForTesting();
  }

  MetricsServiceHistogramProvider* provider() { return &provider_; }

  std::unique_ptr<base::PersistentHistogramAllocator> CreateDuplicateAllocator(
      base::PersistentHistogramAllocator* allocator) {
    // Just wrap around the data segment in-use by the passed allocator.
    return base::MakeUnique<base::PersistentHistogramAllocator>(
        base::MakeUnique<base::PersistentMemoryAllocator>(
            const_cast<void*>(allocator->data()), allocator->length(), 0, 0,
            std::string(), false));
  }

  size_t GetSnapshotHistogramCount() {
    // Merge the data from the allocator into the StatisticsRecorder.
    provider_.MergeHistogramDeltasForTest();

    // Flatten what is known to see what has changed since the last time.
    HistogramFlattenerDeltaRecorder flattener;
    base::HistogramSnapshotManager snapshot_manager(&flattener);
    // "true" to the begin() includes histograms held in persistent storage.
    snapshot_manager.PrepareDeltas(
        base::StatisticsRecorder::begin(true), base::StatisticsRecorder::end(),
        base::Histogram::kNoFlags, base::Histogram::kNoFlags);
    return flattener.GetRecordedDeltaHistogramNames().size();
  }

  void RegisterAllocator(
      int id,
      std::unique_ptr<base::PersistentHistogramAllocator> allocator) {
    provider_.RegisterAllocator(id, std::move(allocator));
  }

  void DeregisterAllocator(int id) { provider_.DeregisterAllocator(id); }

 private:
  MetricsServiceHistogramProvider provider_;
  std::unique_ptr<base::StatisticsRecorder> test_recorder_;

  DISALLOW_COPY_AND_ASSIGN(MetricsServiceHistogramProviderTest);
};

TEST_F(MetricsServiceHistogramProviderTest, SnapshotMetrics) {
  base::HistogramBase* foo = base::Histogram::FactoryGet("foo", 1, 100, 10, 0);
  base::HistogramBase* bar = base::Histogram::FactoryGet("bar", 1, 100, 10, 0);
  base::HistogramBase* baz = base::Histogram::FactoryGet("baz", 1, 100, 10, 0);
  foo->Add(42);
  bar->Add(84);

  // Detach the global allocator but keep it around until this method exits
  // so that the memory holding histogram data doesn't get released. Register
  // a new allocator that duplicates the global one.
  std::unique_ptr<base::GlobalHistogramAllocator> global_allocator(
      base::GlobalHistogramAllocator::ReleaseForTesting());
  RegisterAllocator(123, CreateDuplicateAllocator(global_allocator.get()));

  // Recording should find the two histograms created in persistent memory.
  EXPECT_EQ(2U, GetSnapshotHistogramCount());

  // A second run should have nothing to produce.
  EXPECT_EQ(0U, GetSnapshotHistogramCount());

  // Create a new histogram and update existing ones. Should now report 3 items.
  baz->Add(1969);
  foo->Add(10);
  bar->Add(20);
  EXPECT_EQ(3U, GetSnapshotHistogramCount());

  // Ensure that deregistering does a final merge of the data.
  foo->Add(10);
  bar->Add(20);
  DeregisterAllocator(123);
  EXPECT_EQ(2U, GetSnapshotHistogramCount());

  // Further snapshots should be empty even if things have changed.
  foo->Add(10);
  bar->Add(20);
  EXPECT_EQ(0U, GetSnapshotHistogramCount());
}

}  // namespace metrics