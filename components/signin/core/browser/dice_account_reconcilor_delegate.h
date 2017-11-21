// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_SIGNIN_CORE_BROWSER_DICE_ACCOUNT_RECONCILOR_DELEGATE_H_
#define COMPONENTS_SIGNIN_CORE_BROWSER_DICE_ACCOUNT_RECONCILOR_DELEGATE_H_

#include <string>

#include "base/macros.h"
#include "components/signin/core/browser/account_reconcilor_delegate.h"

namespace user_prefs {
class PrefRegistrySyncable;
}

class PrefService;
class SigninClient;

namespace signin {

// AccountReconcilorDelegate specialized for Dice.
class DiceAccountReconcilorDelegate : public AccountReconcilorDelegate {
 public:
  DiceAccountReconcilorDelegate(SigninClient* signin_client,
                                bool is_new_profile);
  ~DiceAccountReconcilorDelegate() override {}

  static void RegisterProfilePrefs(user_prefs::PrefRegistrySyncable* registry);

  // Dice migration methods, public for testing:
  // Schedules migration to happen at next startup.
  static void SetDiceMigrationOnStartup(PrefService* user_prefs, bool migrate);
  // Returns true if migration can happen on the next startup.
  bool IsReadyForDiceMigration(bool is_new_profile);

  // AccountReconcilorDelegate:
  bool IsReconcileEnabled() const override;
  bool IsAccountConsistencyEnforced() const override;
  std::string GetFirstGaiaAccountForReconcile(
      const std::vector<std::string>& chrome_accounts,
      const std::vector<gaia::ListedAccount>& gaia_accounts,
      const std::string& primary_account,
      bool first_execution) const override;
  void OnReconcileFinished(const std::string& first_account,
                           bool reconcile_is_noop) override;

 private:
  // Last known "first account". Used when cookies are lost as a best guess.
  std::string last_known_first_account_;

  SigninClient* signin_client_;

  DISALLOW_COPY_AND_ASSIGN(DiceAccountReconcilorDelegate);
};

}  // namespace signin

#endif  // COMPONENTS_SIGNIN_CORE_BROWSER_DICE_ACCOUNT_RECONCILOR_DELEGATE_H_
