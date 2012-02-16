// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_PLUGIN_PREFS_FACTORY_H_
#define CHROME_BROWSER_PLUGIN_PREFS_FACTORY_H_
#pragma once

#include "base/compiler_specific.h"
#include "base/memory/singleton.h"
#include "chrome/browser/profiles/refcounted_profile_keyed_service_factory.h"

class PluginPrefs;
class PrefService;
class Profile;
class ProfileKeyedService;

class PluginPrefsFactory : public RefcountedProfileKeyedServiceFactory {
 public:
  static PluginPrefsFactory* GetInstance();

  PluginPrefs* GetPrefsForProfile(Profile* profile);

  static ProfileKeyedBase* CreatePrefsForProfile(Profile* profile);

  // Some unit tests that deal with PluginPrefs don't run with a Profile. Let
  // them still register their preferences.
  void ForceRegisterPrefsForTest(PrefService* prefs);

 private:
  friend struct DefaultSingletonTraits<PluginPrefsFactory>;

  PluginPrefsFactory();
  virtual ~PluginPrefsFactory();

  // RefcountedProfileKeyedServiceFactory methods:
  virtual RefcountedProfileKeyedService* BuildServiceInstanceFor(
      Profile* profile) const OVERRIDE;

  // ProfileKeyedServiceFactory methods:
  virtual void RegisterUserPrefs(PrefService* prefs) OVERRIDE;
  virtual bool ServiceRedirectedInIncognito() OVERRIDE;
  virtual bool ServiceIsNULLWhileTesting() OVERRIDE;
  virtual bool ServiceIsCreatedWithProfile() OVERRIDE;
};

#endif  // CHROME_BROWSER_PLUGIN_PREFS_FACTORY_H_
