// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/metrics/dummy_histogram_samples.h"

#include <memory>

namespace base {

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

bool DummySampleCountIterator::Done() const {
  return true;
}
void DummySampleCountIterator::Next() {}
void DummySampleCountIterator::Get(HistogramBase::Sample* min,
                                   int64_t* max,
                                   HistogramBase::Count* count) const {}

}  // namespace base
