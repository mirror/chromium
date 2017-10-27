// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_PAGE_LOAD_METRICS_OBSERVERS_USE_COUNTER_UKM_FEATURES_H_
#define CHROME_BROWSER_PAGE_LOAD_METRICS_OBSERVERS_USE_COUNTER_UKM_FEATURES_H_

#include "third_party/WebKit/public/platform/web_feature.mojom.h"

// This file defines a list of UseCounter WebFeature_s being measured in the
// UKM-based UseCounter. Features must all satisfy UKM privacy requirements
// (see go/ukm-logs). Features should only be added if it's shown (or highly
// likely be) rare, e.g. <1% of page views as measured by UMA.

// Returns true if a given feature is an opt-in ukm feature for use counter.
bool IsOptInUKMFeature(blink::mojom::WebFeature feature);

#endif  // CHROME_BROWSER_PAGE_LOAD_METRICS_OBSERVERS_USE_COUNTER_UKM_FEATURES_H_
