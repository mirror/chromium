// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_ANDROID_SEARCH_PERMISSIONS_SEARCH_PERMISSIONS_SERVICE_H_
#define CHROME_BROWSER_ANDROID_SEARCH_PERMISSIONS_SEARCH_PERMISSIONS_SERVICE_H_

#include "base/callback_forward.h"
#include "base/memory/singleton.h"
#include "base/strings/string16.h"
#include "components/content_settings/core/common/content_settings.h"
#include "components/keyed_service/content/browser_context_keyed_service_factory.h"
#include "components/keyed_service/core/keyed_service.h"
#include "url/origin.h"

namespace content {
class BrowserContext;
}

namespace user_prefs {
class PrefRegistrySyncable;
}

class HostContentSettingsMap;
class PrefService;
class Profile;

// Helper class to manage DSE permissions. It keeps the setting valid by
// watching change to the CCTLD and DSE, and also provides logic for whether the
// setting should be used and it's current value.
// Glossary:
//     DSE: Default Search Engine
//     CCTLD: Country Code Top Level Domain (e.g. google.com.au)
class SearchPermissionsService : public KeyedService {
 public:
  // Delegate for search engine related functionality. Can be overridden for
  // testing.
  class SearchEngineDelegate {
   public:
    virtual ~SearchEngineDelegate() {}

    // Returns the name of the current DSE.
    virtual base::string16 GetDSEName() = 0;

    // Returns the origin of the DSE. If the current DSE is Google this will
    // return the current CCTLD.
    virtual url::Origin GetDSEOrigin() = 0;

    // Set a callback that will be called if the DSE or CCTLD changes for any
    // reason.
    virtual void SetDSEChangedCallback(const base::Closure& callback) = 0;
  };

  // Factory implementation will not create a service in incognito.
  class Factory : public BrowserContextKeyedServiceFactory {
   public:
    static SearchPermissionsService* GetForBrowserContext(
        content::BrowserContext* context);

    static Factory* GetInstance();

   private:
    friend struct base::DefaultSingletonTraits<Factory>;

    Factory();
    ~Factory() override;

    // BrowserContextKeyedServiceFactory
    bool ServiceIsCreatedWithBrowserContext() const override;
    KeyedService* BuildServiceInstanceFor(
        content::BrowserContext* profile) const override;
    void RegisterProfilePrefs(
        user_prefs::PrefRegistrySyncable* registry) override;
  };

  explicit SearchPermissionsService(Profile* profile);

  // Returns whether the DSE geolocation setting is applicable for geolocation
  // requests for the given top level origin.
  bool ArePermissionsControlledByDSE(const url::Origin& requesting_origin);

  // Returns the DSE's Origin if geolocation enabled, else an unique Origin.
  url::Origin GetDSEOriginIfEnabled();

  // KeyedService:
  void Shutdown() override;

 private:
  friend class SearchPermissionsServiceTest;
  FRIEND_TEST_ALL_PREFIXES(GeolocationPermissionContextTests,
                           SearchGeolocationInIncognito);
  struct PrefValue;

  ~SearchPermissionsService() override;

  // When the DSE CCTLD changes (either by changing their DSE or by changing
  // their CCTLD, and their DSE supports geolocation:
  // * If the DSE CCTLD origin permission is BLOCK, but the DSE geolocation
  //   setting is on, change the DSE geolocation setting to off
  // * If the DSE CCTLD origin permission is ALLOW, but the DSE geolocation
  //   setting is off, reset the DSE CCTLD origin permission to ASK.
  // Also, if the previous DSE did not support geolocation, and the new one
  // does, and the geolocation setting is on, reset whether the DSE geolocation
  // disclosure has been shown.
  void OnDSEChanged();

  ContentSetting UpdatePermission(ContentSettingsType type,
                                  const GURL& old_dse_origin,
                                  const GURL& new_dse_origin,
                                  ContentSetting old_setting,
                                  bool dse_name_changed);

  // Initialize the DSE geolocation setting if it hasn't already been
  // initialized. Also, if it hasn't been initialized, reset whether the DSE
  // geolocation disclosure has been shown to ensure user who may have seen it
  // on earlier versions (due to Finch experiments) see it again.
  void InitializeSettingsIfNeeded();

  PrefValue GetDSEPref();
  void SetDSEPref(const PrefValue& pref);

  // Retrieve the geolocation content setting for the current DSE CCTLD.
  ContentSetting GetContentSetting(const GURL& origin,
                                   ContentSettingsType type);
  void SetContentSetting(const GURL& origin,
                         ContentSettingsType type,
                         ContentSetting setting);

  void SetSearchEngineDelegateForTest(
      std::unique_ptr<SearchEngineDelegate> delegate);

  Profile* profile_;
  PrefService* pref_service_;
  HostContentSettingsMap* host_content_settings_map_;
  std::unique_ptr<SearchEngineDelegate> delegate_;
};

#endif  // CHROME_BROWSER_ANDROID_SEARCH_PERMISSIONS_SEARCH_PERMISSIONS_SERVICE_H_
