// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/sync/base/data_type_histogram.h"

#include "base/metrics/histogram_functions.h"
#include "base/metrics/sparse_histogram.h"
#include "base/rand_util.h"

const char kModelTypeMemoryHistogramPrefix[] = "Sync.ModelTypeMemoryKB.";

void SyncRecordKbDatatypeBin(const std::string& name, int sample, int value) {
  // Convert raw value to KiB and probabilistically round up/down if the
  // remainder is more than a random number [0, 1KiB). This gives a more
  // accurate count when there are a large number of records. RandInt is
  // "inclusive", hence the -1 for the max value.
  int64_t value_kb = value >> 10;
  if (value - (value_kb << 10) > base::RandInt(0, (1 << 10) - 1))
    value_kb += 1;
  if (value_kb == 0)
    return;

  base::HistogramBase* histogram = base::SparseHistogram::FactoryGet(
      name, base::HistogramBase::kUmaTargetedHistogramFlag);

  histogram->AddCount(sample, value_kb);
}

void SyncRecordMemoryKbHistogram(const std::string& histogram_name_prefix,
                                 syncer::ModelType model_type,
                                 size_t value) {
  std::string type_string;
  if (RealModelTypeToNotificationType(model_type, &type_string)) {
    std::string full_histogram_name = histogram_name_prefix + type_string;
    base::UmaHistogramCounts1M(full_histogram_name, value / 1024);
  }
}
