// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CONTENT_SETTINGS_CONTENT_SETTINGS_POLICY_PROVIDER_H_
#define CHROME_BROWSER_CONTENT_SETTINGS_CONTENT_SETTINGS_POLICY_PROVIDER_H_
#pragma once

// A content settings provider that takes its settings out of policies.

#include <vector>

#include "base/basictypes.h"
#include "base/synchronization/lock.h"
#include "base/tuple.h"
#include "chrome/browser/content_settings/content_settings_origin_identifier_value_map.h"
#include "chrome/browser/content_settings/content_settings_provider.h"
#include "chrome/browser/prefs/pref_change_registrar.h"
#include "content/common/notification_observer.h"
#include "content/common/notification_registrar.h"

class ContentSettingsDetails;
class HostContentSettingsMap;
class PrefService;

namespace content_settings {

class PolicyDefaultProvider : public DefaultProviderInterface,
                              public NotificationObserver {
 public:
  PolicyDefaultProvider(HostContentSettingsMap* map, PrefService* prefs);
  virtual ~PolicyDefaultProvider();

  // DefaultContentSettingsProvider implementation.
  virtual ContentSetting ProvideDefaultSetting(
      ContentSettingsType content_type) const;
  virtual void UpdateDefaultSetting(ContentSettingsType content_type,
                                    ContentSetting setting);
  virtual bool DefaultSettingIsManaged(ContentSettingsType content_type) const;

  virtual void ShutdownOnUIThread();

  static void RegisterUserPrefs(PrefService* prefs);

  // NotificationObserver implementation.
  virtual void Observe(int type,
                       const NotificationSource& source,
                       const NotificationDetails& details);

 private:
  // Informs observers that content settings have changed. Make sure that
  // |lock_| is not held when calling this, as listeners will usually call one
  // of the GetSettings functions in response, which would then lead to a
  // mutex deadlock.
  void NotifyObservers(const ContentSettingsDetails& details);

  // Reads the policy managed default settings.
  void ReadManagedDefaultSettings();

  // Reads the policy controlled default settings for a specific content type.
  void UpdateManagedDefaultSetting(ContentSettingsType content_type);

  // Copies of the pref data, so that we can read it on the IO thread.
  ContentSettings managed_default_content_settings_;

  HostContentSettingsMap* host_content_settings_map_;
  PrefService* prefs_;

  // Used around accesses to the managed_default_content_settings_ object to
  // guarantee thread safety.
  mutable base::Lock lock_;

  PrefChangeRegistrar pref_change_registrar_;
  NotificationRegistrar notification_registrar_;

  DISALLOW_COPY_AND_ASSIGN(PolicyDefaultProvider);
};

// PolicyProvider that provider managed content-settings.
class PolicyProvider : public ProviderInterface,
                       public NotificationObserver {
 public:
  PolicyProvider(HostContentSettingsMap* map,
                 PrefService* prefs,
                 DefaultProviderInterface* default_provider);
  virtual ~PolicyProvider();
  static void RegisterUserPrefs(PrefService* prefs);

  // ProviderInterface Implementation
  virtual void SetContentSetting(
      const ContentSettingsPattern& primary_pattern,
      const ContentSettingsPattern& secondary_pattern,
      ContentSettingsType content_type,
      const ResourceIdentifier& resource_identifier,
      ContentSetting content_setting);

  virtual ContentSetting GetContentSetting(
      const GURL& primary_url,
      const GURL& secondary_url,
      ContentSettingsType content_type,
      const ResourceIdentifier& resource_identifier) const;

  virtual void GetAllContentSettingsRules(
      ContentSettingsType content_type,
      const ResourceIdentifier& resource_identifier,
      Rules* content_setting_rules) const;

  virtual void ClearAllContentSettingsRules(
      ContentSettingsType content_type);

  virtual void ShutdownOnUIThread();

  // NotificationObserver implementation.
  virtual void Observe(int type,
                       const NotificationSource& source,
                       const NotificationDetails& details);
 private:
  typedef Tuple5<
      ContentSettingsPattern,
      ContentSettingsPattern,
      ContentSettingsType,
      ResourceIdentifier,
      ContentSetting> ContentSettingsRule;

  typedef std::vector<ContentSettingsRule> ContentSettingsRules;

  void ReadManagedContentSettings(bool overwrite);

  void GetContentSettingsFromPreferences(ContentSettingsRules* rules);

  void ReadManagedContentSettingsTypes(ContentSettingsType content_type);

  void NotifyObservers(const ContentSettingsDetails& details);

  OriginIdentifierValueMap value_map_;

  HostContentSettingsMap* host_content_settings_map_;
  PrefService* prefs_;

  // Weak, owned by HostContentSettingsMap.
  DefaultProviderInterface* default_provider_;

  PrefChangeRegistrar pref_change_registrar_;

  // Used around accesses to the content_settings_ object to guarantee
  // thread safety.
  mutable base::Lock lock_;

  DISALLOW_COPY_AND_ASSIGN(PolicyProvider);
};

}  // namespace content_settings

#endif  // CHROME_BROWSER_CONTENT_SETTINGS_CONTENT_SETTINGS_POLICY_PROVIDER_H_
