// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BASE_METRICS_DUMMY_HISTOGRAM_SAMPLES_H_
#define BASE_METRICS_DUMMY_HISTOGRAM_SAMPLES_H_

#include "base/base_export.h"
#include "base/metrics/histogram_samples.h"

namespace base {

class BASE_EXPORT DummyHistogramSamples : public HistogramSamples {
 public:
  HistogramBase::Count GetCount(HistogramBase::Sample value) const override;
  HistogramBase::Count TotalCount() const override;
  std::unique_ptr<SampleCountIterator> Iterator() const override;
  bool AddSubtractImpl(SampleCountIterator* iter, Operator op) override;
};

class BASE_EXPORT DummySampleCountIterator : public SampleCountIterator {
 public:
  bool Done() const override;
  void Next() override;
  void Get(HistogramBase::Sample* min,
           int64_t* max,
           HistogramBase::Count* count) const override;
};

}  // namespace base

#endif  // BASE_METRICS_DUMMY_HISTOGRAM_SAMPLES_H_
