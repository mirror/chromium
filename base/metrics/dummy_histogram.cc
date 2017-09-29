// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/metrics/dummy_histogram.h"

#include <memory>

#include "base/lazy_instance.h"
#include "base/logging.h"
#include "base/metrics/histogram_samples.h"
#include "base/metrics/metrics_hashes.h"

namespace base {

namespace {

base::LazyInstance<base::DummyHistogram>::Leaky g_dummy_histogram_ =
    LAZY_INSTANCE_INITIALIZER;

// Helper classes for DummyHistogram.
class DummySampleCountIterator : public SampleCountIterator {
 public:
  DummySampleCountIterator() {}
  ~DummySampleCountIterator() override {}

  // SampleCountIterator:
  bool Done() const override { return true; }
  void Next() override {}
  void Get(HistogramBase::Sample* min,
           int64_t* max,
           HistogramBase::Count* count) const override {}

 private:
  DISALLOW_COPY_AND_ASSIGN(DummySampleCountIterator);
};

class DummyHistogramSamples : public HistogramSamples {
 public:
  DummyHistogramSamples() : HistogramSamples(0, nullptr) {}
  ~DummyHistogramSamples() override {}

  // HistogramSamples:
  HistogramBase::Count GetCount(HistogramBase::Sample value) const override;
  HistogramBase::Count TotalCount() const override;
  std::unique_ptr<SampleCountIterator> Iterator() const override;
  bool AddSubtractImpl(SampleCountIterator* iter, Operator op) override;

 private:
  DISALLOW_COPY_AND_ASSIGN(DummyHistogramSamples);
};

HistogramBase::Count DummyHistogramSamples::GetCount(
    HistogramBase::Sample value) const {
  return HistogramBase::Count();
}

HistogramBase::Count DummyHistogramSamples::TotalCount() const {
  return HistogramBase::Count();
}

std::unique_ptr<SampleCountIterator> DummyHistogramSamples::Iterator() const {
  return std::make_unique<DummySampleCountIterator>();
}

bool DummyHistogramSamples::AddSubtractImpl(SampleCountIterator* iter,
                                            Operator op) {
  return true;
}

}  // namespace

// static
DummyHistogram* DummyHistogram::GetInstance() {
  return g_dummy_histogram_.Pointer();
}

DummyHistogram::DummyHistogram() : HistogramBase("dummy_histogram") {}

DummyHistogram::~DummyHistogram() {}

void DummyHistogram::CheckName(const StringPiece& name) const {}

uint64_t DummyHistogram::name_hash() const {
  return HashMetricName(histogram_name());
}

HistogramType DummyHistogram::GetHistogramType() const {
  return DUMMY_HISTOGRAM;
}

bool DummyHistogram::HasConstructionArguments(
    Sample expected_minimum,
    Sample expected_maximum,
    uint32_t expected_bucket_count) const {
  return true;
}

void DummyHistogram::Add(Sample value) {}

void DummyHistogram::AddCount(Sample value, int count) {}

void DummyHistogram::AddSamples(const HistogramSamples& samples) {}

bool DummyHistogram::AddSamplesFromPickle(base::PickleIterator* iter) {
  return true;
}

std::unique_ptr<HistogramSamples> DummyHistogram::SnapshotSamples() const {
  return nullptr;
}

std::unique_ptr<HistogramSamples> DummyHistogram::SnapshotDelta() {
  return nullptr;
}

std::unique_ptr<HistogramSamples> DummyHistogram::SnapshotFinalDelta() const {
  return nullptr;
}

void DummyHistogram::WriteHTMLGraph(std::string* output) const {}

void DummyHistogram::WriteAscii(std::string* output) const {}

void DummyHistogram::SerializeInfoImpl(Pickle* pickle) const {}

void DummyHistogram::GetParameters(DictionaryValue* params) const {}

void DummyHistogram::GetCountAndBucketData(Count* count,
                                           int64_t* sum,
                                           ListValue* buckets) const {}

}  // namespace base
