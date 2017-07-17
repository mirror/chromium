// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/metrics/public/cpp/histogram_provider.h"

#include "base/command_line.h"
#include "base/metrics/statistics_recorder.h"

namespace metrics {

HistogramProvider::HistogramProvider() {}

HistogramProvider::~HistogramProvider() {}

void HistogramProvider::GetHistogram(const std::string& name,
                                     GetHistogramCallback callback) {
  base::HistogramBase* histogram =
      base::StatisticsRecorder::FindHistogram(name);

  std::string histogram_json = "{}";
  if (histogram)
    histogram->WriteJSON(&histogram_json);
  std::move(callback).Run(histogram_json);
}

}  // namespace metrics
