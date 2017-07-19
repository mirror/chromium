// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_SIGNIN_CORE_BROWSER_SCOPED_ACCOUNT_CONSISTENCY_H_
#define COMPONENTS_SIGNIN_CORE_BROWSER_SCOPED_ACCOUNT_CONSISTENCY_H_

#include <memory>

#include "base/test/scoped_feature_list.h"
#include "components/signin/core/common/profile_management_switches.h"

namespace base {
class FieldTrialList;
}

class ScopedAccountConsistency {
 public:
  // |create_field_trial_list| creates the field trial list ; it is typically
  // needed in unit tests but not in browser tests.
  ScopedAccountConsistency(switches::AccountConsistencyMethod method,
                           bool create_field_trial_list = true);

  ~ScopedAccountConsistency();

 private:
  base::test::ScopedFeatureList scoped_feature_list_;
  std::unique_ptr<base::FieldTrialList> field_trial_list_;
};

#endif  // COMPONENTS_SIGNIN_CORE_BROWSER_SCOPED_ACCOUNT_CONSISTENCY_H_
