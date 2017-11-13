// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_SIGNIN_CORE_BROWSER_ACCOUNT_RECONCILOR_DELEGATE_H_
#define COMPONENTS_SIGNIN_CORE_BROWSER_ACCOUNT_RECONCILOR_DELEGATE_H_

#include <string>
#include <vector>

#include "google_apis/gaia/gaia_auth_util.h"

namespace signin {

class AccountReconcilorDelegate {
 public:
  virtual ~AccountReconcilorDelegate(){};

  virtual bool IsEnabled() const;

  // Returns true if account consistency is enforced (Mirror or Dice).
  // If this is false, reconcile is done, but its results are discarded and no
  // changes to the accounts are made.
  virtual bool IsAccountConsistencyEnforced() const;

  // Returns the first account to add in the Gaia cookie.
  // If this returns an empty string, the user must be logged out of all
  // accounts.
  virtual std::string GetFirstGaiaAccountForReconcile(
      const std::vector<std::string>& chrome_accounts,
      const std::vector<gaia::ListedAccount>& gaia_accounts,
      const std::string& primary_account,
      bool first_execution) const;

  // Called when reconcile is finished.
  virtual void OnReconcileFinished(const std::string& first_account,
                                   bool reconcile_is_noop) {}
};

}  // namespace signin

#endif  // COMPONENTS_SIGNIN_CORE_BROWSER_ACCOUNT_RECONCILOR_DELEGATE_H_
