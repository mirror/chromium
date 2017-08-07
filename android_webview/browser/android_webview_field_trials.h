// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ANDROID_WEBVIEW_BROWSER_ANDROID_WEBVIEW_FIELD_TRIALS_H_
#define ANDROID_WEBVIEW_BROWSER_ANDROID_WEBVIEW_FIELD_TRIALS_H_

#include "base/macros.h"
#include "components/variations/platform_field_trials.h"

namespace variations {

// Infrastructure for setting up platform specific field trials. Chrome and
// WebView make use through their corresponding subclasses.
class AndroidWebViewFieldTrials : public variations::PlatformFieldTrials {
 public:
  AndroidWebViewFieldTrials(){};
  ~AndroidWebViewFieldTrials() override{};

  // Set up field trials for a specific platform.
  void SetupFieldTrials() override{};

  // Create field trials that will control feature list features. This should be
  // called during the same timing window as
  // FeatureList::AssociateReportingFieldTrial. |has_seed| indicates that the
  // variations service used a seed to create field trials. This can be used to
  // prevent associating a field trial with a feature that you expect to be
  // controlled by the variations seed.
  void SetupFeatureControllingFieldTrials(
      bool has_seed,
      base::FeatureList* feature_list) override {}

 private:
  DISALLOW_COPY_AND_ASSIGN(AndroidWebViewFieldTrials);
};

}  // namespace variations

#endif  // ANDROID_WEBVIEW_BROWSER_ANDROID_WEBVIEW_FIELD_TRIALS_H_
