// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/metrics/dummy_histogram.h"

#include <memory>

#include "base/logging.h"
#include "base/memory/singleton.h"
#include "base/metrics/dummy_histogram_samples.h"
#include "base/metrics/metrics_hashes.h"

namespace base {

// static
DummyHistogram* DummyHistogram::GetInstance() {
  return Singleton<DummyHistogram, LeakySingletonTraits<DummyHistogram>>::get();
}

DummyHistogram::DummyHistogram() : HistogramBase("dummy_histogram") {}

DummyHistogram::~DummyHistogram() {}

void DummyHistogram::CheckName(const StringPiece& name) const {}
uint64_t DummyHistogram::name_hash() const {
  return HashMetricName(histogram_name_);
}

HistogramType DummyHistogram::GetHistogramType() const {
  return DUMMY_HISTOGRAM;
}

bool DummyHistogram::HasConstructionArguments(
    Sample expected_minimum,
    Sample expected_maximum,
    uint32_t expected_bucket_count) const {
  return false;
}

void DummyHistogram::Add(Sample value) {}
void DummyHistogram::AddCount(Sample value, int count) {}

void DummyHistogram::AddSamples(const HistogramSamples& samples) {}
bool DummyHistogram::AddSamplesFromPickle(base::PickleIterator* iter) {
  return true;
}

std::unique_ptr<HistogramSamples> DummyHistogram::SnapshotSamples() const {
  return std::unique_ptr<DummyHistogramSamples>(nullptr);
}
std::unique_ptr<HistogramSamples> DummyHistogram::SnapshotDelta() {
  return std::unique_ptr<DummyHistogramSamples>(nullptr);
}
std::unique_ptr<HistogramSamples> DummyHistogram::SnapshotFinalDelta() const {
  return std::unique_ptr<DummyHistogramSamples>(nullptr);
}

void DummyHistogram::WriteHTMLGraph(std::string* output) const {}
void DummyHistogram::WriteAscii(std::string* output) const {}

bool DummyHistogram::SerializeInfoImpl(Pickle* pickle) const {
  return false;
}
void DummyHistogram::GetParameters(DictionaryValue* params) const {}
void DummyHistogram::GetCountAndBucketData(Count* count,
                                           int64_t* sum,
                                           ListValue* buckets) const {}

}  // namespace base
