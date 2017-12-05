// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_ANDROID_OOM_INTERVENTION_OOM_INTERVENTION_DECIDER_H_
#define CHROME_BROWSER_ANDROID_OOM_INTERVENTION_OOM_INTERVENTION_DECIDER_H_

#include "base/gtest_prod_util.h"
#include "base/supports_user_data.h"

namespace content {
class BrowserContext;
}

namespace user_prefs {
class PrefRegistrySyncable;
}

class OomInterventionDeciderTest;
class PrefService;

// This class contains triggering and opting-out logic for OOM intervention.
// Opt-out logic:
// - If user declined intervention, don't trigger it until OOM is observed on
//   the same site.
// - If user declined intervention again even after OOM is observed on the site,
//   never trigger intervention on the site.
// - If len(blacklist) > kMaxBlacklistSize, the user is permanently opted out.
//
// An instance of this class is associated with BrowserContext. You can obtain
// it via GetForBrowserContext().
class OomInterventionDecider : public base::SupportsUserData::Data {
 public:
  static void RegisterProfilePrefs(user_prefs::PrefRegistrySyncable* registry);

  static OomInterventionDecider* GetForBrowserContext(
      content::BrowserContext* context);

  explicit OomInterventionDecider(PrefService* prefs);
  ~OomInterventionDecider() override;

  bool CanTriggerIntervention(const std::string& host) const;

  void OnInterventionDeclined(const std::string& host);
  void OnOomDetected(const std::string& host);

  void ClearData();

 private:
  FRIEND_TEST_ALL_PREFIXES(OomInterventionDeciderTest, OptOutSingleHost);
  FRIEND_TEST_ALL_PREFIXES(OomInterventionDeciderTest, ParmanentlyOptOut);

  // These constants are declined here for testing.
  static const size_t kMaxListSize;
  static const size_t kMaxBlacklistSize;

  bool IsOptedOut(const std::string& host) const;

  bool IsInList(const char* list_name, const std::string& host) const;
  void AddToList(const char* list_name, const std::string& host);

  PrefService* prefs_;

  DISALLOW_COPY_AND_ASSIGN(OomInterventionDecider);
};

#endif  // CHROME_BROWSER_ANDROID_OOM_INTERVENTION_OOM_INTERVENTION_DECIDER_H_
