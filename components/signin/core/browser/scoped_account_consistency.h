// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_SIGNIN_CORE_BROWSER_SCOPED_ACCOUNT_CONSISTENCY_H_
#define COMPONENTS_SIGNIN_CORE_BROWSER_SCOPED_ACCOUNT_CONSISTENCY_H_

#include <memory>

#include "base/macros.h"
#include "base/test/scoped_feature_list.h"
#include "components/signin/core/common/profile_management_switches.h"

namespace base {
class FieldTrialList;
}

namespace signin {

// Changes the account consistency method while it is in scope. Useful for
// tests.
class ScopedAccountConsistency {
 public:
  // |create_field_trial_list| creates the field trial list ; it is typically
  // needed in unit tests but not in browser tests.
  ScopedAccountConsistency(AccountConsistencyMethod method,
                           bool create_field_trial_list = true);

  ~ScopedAccountConsistency();

 private:
  base::test::ScopedFeatureList scoped_feature_list_;
  std::unique_ptr<base::FieldTrialList> field_trial_list_;

  DISALLOW_COPY_AND_ASSIGN(ScopedAccountConsistency);
};

class ScopedAccountConsistencyMirror {
 public:
  ScopedAccountConsistencyMirror(bool create_field_trial_list = true);

 private:
  ScopedAccountConsistency scoped_mirror_;

  DISALLOW_COPY_AND_ASSIGN(ScopedAccountConsistencyMirror);
};

class ScopedAccountConsistencyDice {
 public:
  ScopedAccountConsistencyDice(bool create_field_trial_list = true);

 private:
  ScopedAccountConsistency scoped_dice_;

  DISALLOW_COPY_AND_ASSIGN(ScopedAccountConsistencyDice);
};

}  // namespace signin

#endif  // COMPONENTS_SIGNIN_CORE_BROWSER_SCOPED_ACCOUNT_CONSISTENCY_H_
