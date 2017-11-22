// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_SIGNIN_ACCOUNT_CONSISTENCY_MANAGER_H_
#define CHROME_BROWSER_SIGNIN_ACCOUNT_CONSISTENCY_MANAGER_H_

#include "base/gtest_prod_util.h"
#include "base/macros.h"
#include "components/signin/core/browser/profile_management_switches.h"

namespace user_prefs {
class PrefRegistrySyncable;
}

class Profile;

class AccountConsistencyManager {
 public:
  explicit AccountConsistencyManager(Profile* profile);
  ~AccountConsistencyManager();

  static void RegisterProfilePrefs(user_prefs::PrefRegistrySyncable* registry);

  void SetReadyForDiceMigration(bool is_ready);

  static void SetDiceMigrationOnStartup(PrefService* prefs, bool migrate);

 private:
  FRIEND_TEST_ALL_PREFIXES(AccountConsistencyManagerTest, MigrateAtCreation);

  bool IsReadyForDiceMigration();

  Profile* profile_;

  DISALLOW_COPY_AND_ASSIGN(AccountConsistencyManager);
};

#endif  // CHROME_BROWSER_SIGNIN_ACCOUNT_CONSISTENCY_MANAGER_H_
