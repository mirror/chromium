// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/page_load_metrics/observers/use_counter/ukm_features.h"

bool IsOptInUKMFeature(blink::mojom::WebFeature feature) {
  CR_DEFINE_STATIC_LOCAL(const std::set<blink::mojom::WebFeature>,
                         opt_in_features, {});
  return opt_in_features.count(feature);
}
