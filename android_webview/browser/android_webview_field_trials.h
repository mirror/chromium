// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ANDROID_WEBVIEW_BROWSER_ANDROID_WEBVIEW_FIELD_TRIALS_H_
#define ANDROID_WEBVIEW_BROWSER_ANDROID_WEBVIEW_FIELD_TRIALS_H_

#include "base/macros.h"
#include "components/variations/platform_field_trials.h"

namespace variations {

class AndroidWebViewFieldTrials : public variations::PlatformFieldTrials {
 public:
  AndroidWebViewFieldTrials() {}
  ~AndroidWebViewFieldTrials() override {}

  // variations::PlatformFieldTrials:
  void SetupFieldTrials() override{};
  void SetupFeatureControllingFieldTrials(
      bool has_seed,
      base::FeatureList* feature_list) override {}

 private:
  DISALLOW_COPY_AND_ASSIGN(AndroidWebViewFieldTrials);
};

}  // namespace variations

#endif  // ANDROID_WEBVIEW_BROWSER_ANDROID_WEBVIEW_FIELD_TRIALS_H_
