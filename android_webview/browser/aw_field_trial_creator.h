// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ANDROID_WEBVIEW_BROWSER_AW_FIELD_TRIAL_CREATOR_H_
#define ANDROID_WEBVIEW_BROWSER_AW_FIELD_TRIAL_CREATOR_H_

#include "android_webview/browser/android_webview_field_trials.h"
#include "base/metrics/field_trial.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/pref_service_factory.h"
#include "components/variations/service/variations_field_trial_creator.h"

namespace android_webview {

class AwFieldTrialCreator {
 public:
  AwFieldTrialCreator();
  ~AwFieldTrialCreator();

  std::unique_ptr<PrefService> CreateLocalState();
  void SetUpFieldTrials();

 private:
  // A/B testing infrastructure for the entire application. empty until
  // |SetupFieldTrials()| is called.
  std::unique_ptr<base::FieldTrialList> field_trial_list_;

  // Platform specific field trial infrastructure.
  AndroidWebViewFieldTrials webview_field_trials_;

  // Responsible for creating a feature list from the seed.
  std::unique_ptr<variations::VariationsFieldTrialCreator>
      variations_field_trial_creator_;

  // Used to create a PrefService for variations prefs.
  PrefServiceFactory pref_service_factory_;

  DISALLOW_COPY_AND_ASSIGN(AwFieldTrialCreator);
};

}  // namespace android_webview

#endif  // ANDROID_WEBVIEW_BROWSER_AW_FIELD_TRIAL_CREATOR_H_
