// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_METRICS_METRICS_SERVICE_ACCESSOR_H_
#define COMPONENTS_METRICS_METRICS_SERVICE_ACCESSOR_H_

#include <stdint.h>
#include <vector>

#include "base/macros.h"
#include "base/strings/string_piece.h"

class PrefService;

namespace metrics {

class MetricsService;

// This class limits and documents access to metrics service helper methods.
// These methods are protected so each user has to inherit own program-specific
// specialization and enable access there by declaring friends.
class MetricsServiceAccessor {
 protected:
  // Constructor declared as protected to enable inheritance. Descendants should
  // disallow instantiation.
  MetricsServiceAccessor() {}

  // Returns whether metrics reporting is enabled, using the value of the
  // kMetricsReportingEnabled pref in |pref_service| to determine whether user
  // has enabled reporting.
  static bool IsMetricsReportingEnabled(PrefService* pref_service);


  // Registers a field trial name and group with |metrics_service| (if not
  // null), to be used to annotate a UMA report with a particular configuration
  // state. Returns true on success.
  // See the comment on MetricsService::RegisterSyntheticFieldTrial() for
  // details.
  static bool RegisterSyntheticFieldTrial(MetricsService* metrics_service,
                                          base::StringPiece trial_name,
                                          base::StringPiece group_name);

 private:
  DISALLOW_COPY_AND_ASSIGN(MetricsServiceAccessor);
};

}  // namespace metrics

#endif  // COMPONENTS_METRICS_METRICS_SERVICE_ACCESSOR_H_
