// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/signin/core/browser/scoped_account_consistency.h"

#include <map>
#include <string>
#include <utility>

#include "base/feature_list.h"
#include "base/logging.h"
#include "base/metrics/field_trial.h"
#include "base/metrics/field_trial_param_associator.h"
#include "base/metrics/field_trial_params.h"
#include "base/test/scoped_feature_list.h"
#include "components/signin/core/common/signin_features.h"

ScopedAccountConsistency::ScopedAccountConsistency(
    switches::AccountConsistencyMethod method,
    bool create_field_trial_list) {
#if !BUILDFLAG(ENABLE_DICE_SUPPORT)
  DCHECK_NE(switches::AccountConsistencyMethod::kDice, method);
  DCHECK_NE(switches::AccountConsistencyMethod::kDiceFix, method);
#endif

#if BUILDFLAG(ENABLE_MIRROR)
  DCHECK_EQ(switches::AccountConsistencyMethod::kMirror, method);
  return;
#endif

  if (method == switches::AccountConsistencyMethod::kDisabled) {
    scoped_feature_list_.InitAndDisableFeature(
        switches::kAccountConsistencyFeature);
    DCHECK_EQ(method, switches::GetAccountConsistencyMethod());
    return;
  }

  if (create_field_trial_list)
    field_trial_list_ = base::MakeUnique<base::FieldTrialList>(nullptr);

  // Set up the account consistency method through a field trial.
  std::string kTrialName = "name";
  std::string kTrialGroup = "group";
  base::FieldTrial* trial =
      base::FieldTrialList::CreateFieldTrial(kTrialName, kTrialGroup);
  std::string feature_param;
  switch (method) {
    case switches::AccountConsistencyMethod::kDisabled:
      NOTREACHED();
      break;
    case switches::AccountConsistencyMethod::kMirror:
      feature_param = switches::kAccountConsistencyFeatureMethodMirror;
      break;
    case switches::AccountConsistencyMethod::kDiceFix:
      feature_param = switches::kAccountConsistencyFeatureMethodDiceFix;
      break;
    case switches::AccountConsistencyMethod::kDice:
      feature_param = switches::kAccountConsistencyFeatureMethodDice;
      break;
  }

  std::map<std::string, std::string> field_trial_params;
  field_trial_params[switches::kAccountConsistencyFeatureMethodParameter] =
      feature_param;
  base::FieldTrialParamAssociator::GetInstance()->AssociateFieldTrialParams(
      kTrialName, kTrialGroup, field_trial_params);

  scoped_feature_list_.InitAndEnableFeatureWithFieldTrialOverride(
      switches::kAccountConsistencyFeature, trial);
  DCHECK_EQ(method, switches::GetAccountConsistencyMethod());
}

ScopedAccountConsistency::~ScopedAccountConsistency() {
  base::FieldTrialParamAssociator::GetInstance()->ClearAllParamsForTesting();
}
