// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/dom_distiller/core/feedback_reporter.h"

#include "base/metrics/histogram_macros.h"

namespace dom_distiller {

FeedbackReporter::FeedbackReporter() = default;

FeedbackReporter::~FeedbackReporter() = default;

// static
void FeedbackReporter::ReportQuality(bool good) {
  UMA_HISTOGRAM_BOOLEAN("DomDistiller.DistillationQuality", good);
}

}  // namespace dom_distiller
