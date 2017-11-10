// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/autofill/core/common/autofill_constants.h"

#include "build/build_config.h"

namespace autofill {

// TODO(rogerm): Move autofill_experiments from browser to common then relocate
// this feature flags.
const base::Feature kAutofillEnforceMinRequiredFieldsForHeuristics{
    "AutofillEnforceMinRequiredFieldsForHeuristics",
    base::FEATURE_ENABLED_BY_DEFAULT};

const base::Feature kAutofillEnforceMinRequiredFieldsForQuery{
    "AutofillEnforceMinRequiredFieldsForQuery",
    base::FEATURE_ENABLED_BY_DEFAULT};

const base::Feature kAutofillEnforceMinRequiredFieldsForUpload{
    "AutofillEnforceMinRequiredFieldsForUpload",
    base::FEATURE_DISABLED_BY_DEFAULT};

const char kHelpURL[] =
#if defined(OS_CHROMEOS)
    "https://support.google.com/chromebook/?p=settings_autofill";
#else
    "https://support.google.com/chrome/?p=settings_autofill";
#endif

const char kSettingsOrigin[] = "Chrome settings";

size_t MinRequiredFieldsForHeuristics() {
  return base::FeatureList::IsEnabled(
             kAutofillEnforceMinRequiredFieldsForHeuristics)
             ? 3
             : 1;
}
size_t MinRequiredFieldsForQuery() {
  return base::FeatureList::IsEnabled(kAutofillEnforceMinRequiredFieldsForQuery)
             ? 3
             : 1;
}
size_t MinRequiredFieldsForUpload() {
  return base::FeatureList::IsEnabled(
             kAutofillEnforceMinRequiredFieldsForUpload)
             ? 3
             : 1;
}

}  // namespace autofill
