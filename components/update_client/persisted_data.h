// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_UPDATE_CLIENT_PERSISTED_DATA_H_
#define COMPONENTS_UPDATE_CLIENT_PERSISTED_DATA_H_

#include <string>
#include <vector>

#include "base/gtest_prod_util.h"
#include "base/macros.h"
#include "base/threading/thread_checker.h"
#include "base/values.h"
#include "extensions/features/features.h"

class PrefRegistrySimple;
class PrefService;

namespace update_client {

// A PersistedData is a wrapper layer around a PrefService, designed to maintain
// update data that outlives the browser process and isn't exposed outside of
// update_client.
//
// The public methods of this class should be called only on the thread that
// initializes it - which also has to match the thread the PrefService has been
// initialized on.
class PersistedData {
 public:
  // Constructs a provider that uses the specified |pref_service|.
  // The associated preferences are assumed to already be registered.
  // The |pref_service| must outlive the entire update_client.
  explicit PersistedData(PrefService* pref_service);

  ~PersistedData();

  // Returns the DateLastRollCall (the server-localized calendar date number the
  // |id| was last checked by this client on) for the specified |id|.
  // -2 indicates that there is no recorded date number for the |id|.
  int GetDateLastRollCall(const std::string& id) const;

#if BUILDFLAG(ENABLE_EXTENSIONS)
  // Return the number of days since last roll call of the specified |id|.
  // If there is no recorded days since last rollcall for the |id|, the function
  // will return -1.
  // This function is provided for backward-compatibility only, it will
  // eventually be removed.
  int GetDaysSinceLastRollCall(const std::string& id) const;

  // Returns the DateLastActive (the server-localized calendar date number the
  // |id| was last checked by this client on) for the specified |id|.
  // -2 indicates that there is no recorded date number for the |id|.
  int GetDateLastActive(const std::string& id) const;

  // Return the number of days since last active of the specified |id|.
  // If there is no recorded days since last active for the |id|, the function
  // will return -1.
  // This function is provided for backward-compatibility only, it will
  // eventually be removed.
  int GetDaysSinceLastActive(const std::string& id) const;

  // Records the DateLastActive for the specified |ids|. |datenum| must be a
  // non-negative integer: calls with a negative |datenum| are simply ignored.
  // Calls to SetDateLastActive that occur prior to the persisted data store
  // has been fully initialized are ignored.
  void SetDateLastActive(const std::vector<std::string>& ids, int datenum);

  // Get the active bit of the specified |id|.
  bool GetActiveBit(const std::string& id) const;

  // Clear the active bit of the provided |ids|.
  void ClearActiveBit(const std::vector<std::string>& ids);
#endif  // BUILDFLAG(ENABLE_EXTENSIONS)

  // Returns the PingFreshness (a random token that is written into the profile
  // data whenever the DateLastRollCall it is modified) for the specified |id|.
  // "" indicates that there is no recorded freshness value for the |id|.
  std::string GetPingFreshness(const std::string& id) const;

  // Records the DateLastRollCall for the specified |ids|. |datenum| must be a
  // non-negative integer: calls with a negative |datenum| are simply ignored.
  // Calls to SetDateLastRollCall that occur prior to the persisted data store
  // has been fully initialized are ignored. Also sets the PingFreshness.
  void SetDateLastRollCall(const std::vector<std::string>& ids, int datenum);

  // This is called only via update_client's RegisterUpdateClientPreferences.
  static void RegisterPrefs(PrefRegistrySimple* registry);

  // These functions return cohort data for the specified |id|. "Cohort"
  // indicates the membership of the client in any release channels components
  // have set up in a machine-readable format, while "CohortName" does so in a
  // human-readable form. "CohortHint" indicates the client's channel selection
  // preference.
  std::string GetCohort(const std::string& id) const;
  std::string GetCohortHint(const std::string& id) const;
  std::string GetCohortName(const std::string& id) const;

  // These functions set cohort data for the specified |id|.
  void SetCohort(const std::string& id, const std::string& cohort);
  void SetCohortHint(const std::string& id, const std::string& cohort_hint);
  void SetCohortName(const std::string& id, const std::string& cohort_name);

 private:
#if BUILDFLAG(ENABLE_EXTENSIONS)
  FRIEND_TEST_ALL_PREFIXES(PersistedDataTest, Simple);
  FRIEND_TEST_ALL_PREFIXES(UpdateClientUtilsTest, BuildUpdateCheckPingElement);
  FRIEND_TEST_ALL_PREFIXES(UpdateCheckerTest, UpdateCheckLastRollCall);
  FRIEND_TEST_ALL_PREFIXES(UpdateCheckerTest, UpdateCheckLastActive);
#endif

  int GetInt(const std::string& id, const std::string& key, int fallback) const;
  std::string GetString(const std::string& id, const std::string& key) const;
  void SetString(const std::string& id,
                 const std::string& key,
                 const std::string& value);

#if BUILDFLAG(ENABLE_EXTENSIONS)
  // Sets the active bit of the specified |id|.
  // This function is used for unit tests ONLY.
  void SetActiveBit(const std::string& id);

  // Sets the time of the last roll call of the specified |id|.
  // This function is used for unit tests ONLY.
  void SetLastRollCallTime(const std::string& id, const base::Time& time);

  // Sets the time of the last active of the specified |id|.
  // This function is used for unit tests ONLY.
  void SetLastActiveTime(const std::string& id, const base::Time& time);
#endif  // BUILDFLAG(ENABLE_EXTENSIONS)

  base::ThreadChecker thread_checker_;
  PrefService* pref_service_;

  DISALLOW_COPY_AND_ASSIGN(PersistedData);
};

}  // namespace update_client

#endif  // COMPONENTS_UPDATE_CLIENT_PERSISTED_DATA_H_
