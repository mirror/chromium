// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/variations/platform_field_trials.h"

#include <stdint.h>

#include "base/base64.h"
#include "base/macros.h"
#include "base/metrics/histogram_macros.h"
#include "base/numerics/safe_math.h"
#include "base/sha1.h"
#include "base/strings/string_number_conversions.h"
#include "build/build_config.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#include "components/variations/pref_names.h"
#include "components/variations/proto/variations_seed.pb.h"
#include "crypto/signature_verifier.h"
#include "third_party/protobuf/src/google/protobuf/io/coded_stream.h"
#include "third_party/zlib/google/compression_utils.h"

namespace variations {

PlatformFieldTrials::PlatformFieldTrials() {}

PlatformFieldTrials::~PlatformFieldTrials() {}

void PlatformFieldTrials::SetupFieldTrials() {
  // TODO(kmilka): implement functionality for WebView
  return;
}

// Create field trials that will control feature list features. This should be
// called during the same timing window as
// FeatureList::AssociateReportingFieldTrial. |has_seed| indicates that the
// variations service used a seed to create field trials. This can be used to
// prevent associating a field trial with a feature that you expect to be
// controlled by the variations seed.
void PlatformFieldTrials::SetupFeatureControllingFieldTrials(
    bool has_seed,
    base::FeatureList* feature_list) {
  // TODO(kmilka): implement functionality for WebView
  return;
}

}  // namespace variations
