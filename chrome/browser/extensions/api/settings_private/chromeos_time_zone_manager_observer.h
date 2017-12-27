// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_EXTENSIONS_API_SETTINGS_PRIVATE_CHROMEOS_TIME_ZONE_MANAGER_OBSERVER_H_
#define CHROME_BROWSER_EXTENSIONS_API_SETTINGS_PRIVATE_CHROMEOS_TIME_ZONE_MANAGER_OBSERVER_H_

#include "chrome/browser/extensions/api/settings_private/generated_prefs.h"

#include <string>

#include "base/macros.h"
#include "chrome/browser/chromeos/system/timezone_resolver_manager.h"

namespace extensions {

// Helper class to trigger preference observers on TimeZoneResolverManager
// update.
class GeneratedPrefTimeZoneManagerObserver
    : public GeneratedPrefImplBase,
      public chromeos::system::TimeZoneResolverManager::Observer {
 public:
  ~GeneratedPrefTimeZoneManagerObserver() override;

  // chromeos::system::TimeZoneResolverManager::Observer
  void OnTimeZoneResolverUpdated() override;

 protected:
  explicit GeneratedPrefTimeZoneManagerObserver(const std::string& pref_name);

  const std::string pref_name_;

  DISALLOW_COPY_AND_ASSIGN(GeneratedPrefTimeZoneManagerObserver);
};

}  // namespace extensions

#endif  // CHROME_BROWSER_EXTENSIONS_API_SETTINGS_PRIVATE_CHROMEOS_TIME_ZONE_MANAGER_OBSERVER_H_
