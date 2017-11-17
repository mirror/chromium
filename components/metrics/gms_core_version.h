// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_METRICS_GMS_CORE_VERSION_H_
#define COMPONENTS_METRICS_GMS_CORE_VERSION_H_

#include "base/macros.h"

class PrefService;

namespace metrics {

// Detect whether the gms core version changes from last launch. If changes,
// store the new gms core version.
class GmsCoreVersion {
 public:
  GmsCoreVersion(PrefService* local_state);

  ~GmsCoreVersion();

  // Return whether the gms core version changes since last launch. Store the
  // new version number if changes.
  bool check_and_update();

 private:
  PrefService* const local_state_;

  DISALLOW_COPY_AND_ASSIGN(GmsCoreVersion);
};

}  // namespace metrics

#endif  // COMPONENTS_METRICS_GMS_CORE_VERSION_H_
