// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/content_settings/content_settings_pref_provider.h"

#include <list>
#include <string>
#include <utility>
#include <vector>

#include "base/auto_reset.h"
#include "base/command_line.h"
#include "base/metrics/histogram.h"
#include "chrome/browser/content_settings/content_settings_details.h"
#include "chrome/browser/content_settings/content_settings_pattern.h"
#include "chrome/browser/content_settings/content_settings_utils.h"
#include "chrome/browser/prefs/pref_service.h"
#include "chrome/browser/prefs/scoped_user_pref_update.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/common/chrome_notification_types.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/common/content_settings.h"
#include "chrome/common/pref_names.h"
#include "content/browser/browser_thread.h"
#include "content/browser/user_metrics.h"
#include "content/common/notification_details.h"
#include "content/common/notification_service.h"
#include "content/common/notification_source.h"
#include "googleurl/src/gurl.h"
#include "net/base/net_util.h"

namespace {

// The preference keys where resource identifiers are stored for
// ContentSettingsType values that support resource identifiers.
const char* kResourceTypeNames[] = {
  NULL,
  NULL,
  NULL,
  "per_plugin",
  NULL,
  NULL,  // Not used for Geolocation
  NULL,  // Not used for Notifications
};
COMPILE_ASSERT(arraysize(kResourceTypeNames) == CONTENT_SETTINGS_NUM_TYPES,
               resource_type_names_incorrect_size);

// The default setting for each content type.
const ContentSetting kDefaultSettings[] = {
  CONTENT_SETTING_ALLOW,  // CONTENT_SETTINGS_TYPE_COOKIES
  CONTENT_SETTING_ALLOW,  // CONTENT_SETTINGS_TYPE_IMAGES
  CONTENT_SETTING_ALLOW,  // CONTENT_SETTINGS_TYPE_JAVASCRIPT
  CONTENT_SETTING_ALLOW,  // CONTENT_SETTINGS_TYPE_PLUGINS
  CONTENT_SETTING_BLOCK,  // CONTENT_SETTINGS_TYPE_POPUPS
  CONTENT_SETTING_ASK,    // Not used for Geolocation
  CONTENT_SETTING_ASK,    // CONTENT_SETTINGS_TYPE_NOTIFICATIONS
};
COMPILE_ASSERT(arraysize(kDefaultSettings) == CONTENT_SETTINGS_NUM_TYPES,
               default_settings_incorrect_size);

// The names of the ContentSettingsType values, for use with dictionary prefs.
const char* kTypeNames[] = {
  "cookies",
  "images",
  "javascript",
  "plugins",
  "popups",
  NULL,  // Not used for Geolocation
  // TODO(markusheintz): Refactoring in progress. Content settings exceptions
  // for notifications will be added next.
  "notifications",  // Only used for default Notifications settings.
};
COMPILE_ASSERT(arraysize(kTypeNames) == CONTENT_SETTINGS_NUM_TYPES,
               type_names_incorrect_size);

void SetDefaultContentSettings(DictionaryValue* default_settings) {
  default_settings->Clear();

  for (int i = 0; i < CONTENT_SETTINGS_NUM_TYPES; ++i) {
    if (kTypeNames[i] != NULL) {
      default_settings->SetInteger(kTypeNames[i],
                                   kDefaultSettings[i]);
    }
  }
}

const char* kPatternSeparator = ",";

std::string CreatePatternString(
    const ContentSettingsPattern& primary_pattern,
    const ContentSettingsPattern& secondary_pattern) {
  return primary_pattern.ToString() +
         std::string(kPatternSeparator) +
         secondary_pattern.ToString();
}

std::pair<ContentSettingsPattern, ContentSettingsPattern>
    ParsePatternString(const std::string& pattern_str) {
  DCHECK(!pattern_str.empty());
  size_t pos = pattern_str.find(kPatternSeparator);

  std::pair<ContentSettingsPattern, ContentSettingsPattern> pattern_pair;
  if (pos == std::string::npos) {
    pattern_pair.first = ContentSettingsPattern::FromString(pattern_str);
    DCHECK(pattern_pair.first.IsValid());
    pattern_pair.second = ContentSettingsPattern();
  } else {
    pattern_pair.first = ContentSettingsPattern::FromString(
        pattern_str.substr(0, pos));
    DCHECK(pattern_pair.first.IsValid());
    pattern_pair.second = ContentSettingsPattern::FromString(
        pattern_str.substr(pos+1, pattern_str.size() - pos - 1));
    DCHECK(pattern_pair.second.IsValid());
  }
  return pattern_pair;
}

ContentSetting ValueToContentSetting(Value* value) {
  int int_value;
  value->GetAsInteger(&int_value);
  return IntToContentSetting(int_value);
}

ContentSettingsType StringToContentSettingsType(
    const std::string& content_type_str) {
  for (size_t type = 0; type < arraysize(kTypeNames); ++type) {
    if ((kTypeNames[type] != NULL) && (kTypeNames[type] == content_type_str))
      return ContentSettingsType(type);
  }
  for (size_t type = 0; type < arraysize(kResourceTypeNames); ++type) {
    if ((kResourceTypeNames[type] != NULL) &&
        (kResourceTypeNames[type] == content_type_str)) {
      return ContentSettingsType(type);
    }
  }
  return CONTENT_SETTINGS_TYPE_DEFAULT;
}

ContentSetting FixObsoleteCookiePromptMode(ContentSettingsType content_type,
                                           ContentSetting setting) {
  if (content_type == CONTENT_SETTINGS_TYPE_COOKIES &&
      setting == CONTENT_SETTING_ASK) {
    return CONTENT_SETTING_BLOCK;
  }
  return setting;
}

}  // namespace

namespace content_settings {

PrefDefaultProvider::PrefDefaultProvider(HostContentSettingsMap* map,
                                         PrefService* prefs,
                                         bool incognito)
    : host_content_settings_map_(map),
      prefs_(prefs),
      is_incognito_(incognito),
      updating_preferences_(false) {
  DCHECK(prefs_);
  MigrateObsoleteNotificationPref();

  // Read global defaults.
  DCHECK_EQ(arraysize(kTypeNames),
            static_cast<size_t>(CONTENT_SETTINGS_NUM_TYPES));
  ReadDefaultSettings(true);
  if (default_content_settings_.settings[CONTENT_SETTINGS_TYPE_COOKIES] ==
      CONTENT_SETTING_BLOCK) {
    UserMetrics::RecordAction(
        UserMetricsAction("CookieBlockingEnabledPerDefault"));
  } else {
    UserMetrics::RecordAction(
        UserMetricsAction("CookieBlockingDisabledPerDefault"));
  }

  pref_change_registrar_.Init(prefs_);
  pref_change_registrar_.Add(prefs::kDefaultContentSettings, this);
}

PrefDefaultProvider::~PrefDefaultProvider() {
  DCHECK(!prefs_);
}

ContentSetting PrefDefaultProvider::ProvideDefaultSetting(
    ContentSettingsType content_type) const {
  base::AutoLock lock(lock_);
  return default_content_settings_.settings[content_type];
}

void PrefDefaultProvider::UpdateDefaultSetting(
    ContentSettingsType content_type,
    ContentSetting setting) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  DCHECK(prefs_);
  DCHECK(kTypeNames[content_type] != NULL);  // Don't call this for Geolocation.

  // The default settings may not be directly modified for OTR sessions.
  // Instead, they are synced to the main profile's setting.
  if (is_incognito_)
    return;

  std::string dictionary_path(kTypeNames[content_type]);
  {
    AutoReset<bool> auto_reset(&updating_preferences_, true);
    base::AutoLock lock(lock_);
    DictionaryPrefUpdate update(prefs_, prefs::kDefaultContentSettings);
    DictionaryValue* default_settings_dictionary = update.Get();
    if ((setting == CONTENT_SETTING_DEFAULT) ||
        (setting == kDefaultSettings[content_type])) {
      default_content_settings_.settings[content_type] =
          kDefaultSettings[content_type];
      default_settings_dictionary->RemoveWithoutPathExpansion(dictionary_path,
                                                              NULL);
    } else {
      default_content_settings_.settings[content_type] = setting;
      default_settings_dictionary->SetWithoutPathExpansion(
          dictionary_path, Value::CreateIntegerValue(setting));
    }
  }

  ContentSettingsDetails details(
      ContentSettingsPattern(),
      ContentSettingsPattern(),
      content_type,
      std::string());
  NotifyObservers(details);
}

bool PrefDefaultProvider::DefaultSettingIsManaged(
    ContentSettingsType content_type) const {
  return false;
}

void PrefDefaultProvider::Observe(int type,
                                  const NotificationSource& source,
                                  const NotificationDetails& details) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));

  if (type == chrome::NOTIFICATION_PREF_CHANGED) {
    DCHECK_EQ(prefs_, Source<PrefService>(source).ptr());
    if (updating_preferences_)
      return;

    std::string* name = Details<std::string>(details).ptr();
    if (*name == prefs::kDefaultContentSettings) {
      ReadDefaultSettings(true);
    } else {
      NOTREACHED() << "Unexpected preference observed";
      return;
    }

    ContentSettingsDetails details(ContentSettingsPattern(),
                                   ContentSettingsPattern(),
                                   CONTENT_SETTINGS_TYPE_DEFAULT,
                                   std::string());
    NotifyObservers(details);
  } else {
    NOTREACHED() << "Unexpected notification";
  }
}

void PrefDefaultProvider::ShutdownOnUIThread() {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  DCHECK(prefs_);
  pref_change_registrar_.RemoveAll();
  prefs_ = NULL;
  host_content_settings_map_ = NULL;
}

void PrefDefaultProvider::ReadDefaultSettings(bool overwrite) {
  const DictionaryValue* default_settings_dictionary =
      prefs_->GetDictionary(prefs::kDefaultContentSettings);

  base::AutoLock lock(lock_);

  if (overwrite)
    default_content_settings_ = ContentSettings();

  // Careful: The returned value could be NULL if the pref has never been set.
  if (default_settings_dictionary != NULL) {
    GetSettingsFromDictionary(default_settings_dictionary,
                              &default_content_settings_);
  }
  ForceDefaultsToBeExplicit();
}

void PrefDefaultProvider::ForceDefaultsToBeExplicit() {
  DCHECK_EQ(arraysize(kDefaultSettings),
            static_cast<size_t>(CONTENT_SETTINGS_NUM_TYPES));

  for (int i = 0; i < CONTENT_SETTINGS_NUM_TYPES; ++i) {
    if (default_content_settings_.settings[i] == CONTENT_SETTING_DEFAULT)
      default_content_settings_.settings[i] = kDefaultSettings[i];
  }
}

void PrefDefaultProvider::GetSettingsFromDictionary(
    const DictionaryValue* dictionary,
    ContentSettings* settings) {
  for (DictionaryValue::key_iterator i(dictionary->begin_keys());
       i != dictionary->end_keys(); ++i) {
    const std::string& content_type(*i);
    for (size_t type = 0; type < arraysize(kTypeNames); ++type) {
      if ((kTypeNames[type] != NULL) && (kTypeNames[type] == content_type)) {
        int setting = CONTENT_SETTING_DEFAULT;
        bool found = dictionary->GetIntegerWithoutPathExpansion(content_type,
                                                                &setting);
        DCHECK(found);
        settings->settings[type] = IntToContentSetting(setting);
        break;
      }
    }
  }
  // Migrate obsolete cookie prompt mode.
  if (settings->settings[CONTENT_SETTINGS_TYPE_COOKIES] ==
      CONTENT_SETTING_ASK)
    settings->settings[CONTENT_SETTINGS_TYPE_COOKIES] = CONTENT_SETTING_BLOCK;

  settings->settings[CONTENT_SETTINGS_TYPE_PLUGINS] =
      ClickToPlayFixup(CONTENT_SETTINGS_TYPE_PLUGINS,
                       settings->settings[CONTENT_SETTINGS_TYPE_PLUGINS]);
}

void PrefDefaultProvider::NotifyObservers(
    const ContentSettingsDetails& details) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  if (host_content_settings_map_ == NULL)
    return;
  NotificationService::current()->Notify(
      chrome::NOTIFICATION_CONTENT_SETTINGS_CHANGED,
      Source<HostContentSettingsMap>(host_content_settings_map_),
      Details<const ContentSettingsDetails>(&details));
}

void PrefDefaultProvider::MigrateObsoleteNotificationPref() {
  if (prefs_->HasPrefPath(prefs::kDesktopNotificationDefaultContentSetting)) {
    ContentSetting setting = IntToContentSetting(
        prefs_->GetInteger(prefs::kDesktopNotificationDefaultContentSetting));
    UpdateDefaultSetting(CONTENT_SETTINGS_TYPE_NOTIFICATIONS, setting);
    prefs_->ClearPref(prefs::kDesktopNotificationDefaultContentSetting);
  }
}

// static
void PrefDefaultProvider::RegisterUserPrefs(PrefService* prefs) {
  // The registration of the preference prefs::kDefaultContentSettings should
  // also include the default values for default content settings. This allows
  // functional tests to get default content settings by reading the preference
  // prefs::kDefaultContentSettings via pyauto.
  // TODO(markusheintz): Write pyauto hooks for the content settings map as
  // content settings should be read from the host content settings map.
  DictionaryValue* default_content_settings = new DictionaryValue();
  SetDefaultContentSettings(default_content_settings);
  prefs->RegisterDictionaryPref(prefs::kDefaultContentSettings,
                                default_content_settings,
                                PrefService::SYNCABLE_PREF);

  // Obsolete prefs, for migrations:
  prefs->RegisterIntegerPref(
      prefs::kDesktopNotificationDefaultContentSetting,
      kDefaultSettings[CONTENT_SETTINGS_TYPE_NOTIFICATIONS],
      PrefService::SYNCABLE_PREF);
}

// ////////////////////////////////////////////////////////////////////////////
// PrefProvider:
//

// static
void PrefProvider::RegisterUserPrefs(PrefService* prefs) {
  prefs->RegisterIntegerPref(
      prefs::kContentSettingsVersion,
      ContentSettingsPattern::kContentSettingsPatternVersion,
      PrefService::UNSYNCABLE_PREF);
  prefs->RegisterDictionaryPref(prefs::kContentSettingsPatternPairs,
                                PrefService::SYNCABLE_PREF);

  // Obsolete prefs, for migration:
  prefs->RegisterDictionaryPref(prefs::kContentSettingsPatterns,
                                PrefService::SYNCABLE_PREF);
  prefs->RegisterListPref(prefs::kPopupWhitelistedHosts,
                          PrefService::UNSYNCABLE_PREF);
  prefs->RegisterDictionaryPref(prefs::kPerHostContentSettings,
                                PrefService::UNSYNCABLE_PREF);
}

PrefProvider::PrefProvider(HostContentSettingsMap* map,
                           PrefService* prefs,
                           bool incognito)
  : prefs_(prefs),
    host_content_settings_map_(map),
    is_incognito_(incognito),
    updating_preferences_(false) {
  DCHECK(prefs_);
  if (!is_incognito_) {
    // Migrate obsolete preferences.
    MigrateObsoletePerhostPref();
    MigrateObsoletePopupsPref();
    MigrateObsoleteContentSettingsPatternPref();
  }

  // Verify preferences version.
  if (!prefs_->HasPrefPath(prefs::kContentSettingsVersion)) {
    prefs_->SetInteger(prefs::kContentSettingsVersion,
                      ContentSettingsPattern::kContentSettingsPatternVersion);
  }
  if (prefs_->GetInteger(prefs::kContentSettingsVersion) >
      ContentSettingsPattern::kContentSettingsPatternVersion) {
    LOG(ERROR) << "Unknown content settings version in preferences.";
    return;
  }

  // Read content settings exceptions.
  ReadContentSettingsFromPref(false);

  if (!is_incognito_) {
    UMA_HISTOGRAM_COUNTS("ContentSettings.NumberOfExceptions",
                         value_map_.size());
  }

  pref_change_registrar_.Init(prefs_);
  pref_change_registrar_.Add(prefs::kContentSettingsPatterns, this);
  pref_change_registrar_.Add(prefs::kContentSettingsPatternPairs, this);
}

ContentSetting PrefProvider::GetContentSetting(
    const GURL& primary_url,
    const GURL& secondary_url,
    ContentSettingsType content_type,
    const ResourceIdentifier& resource_identifier) const {
  // For a |PrefProvider| used in a |HostContentSettingsMap| of a non incognito
  // profile, this will always return NULL.
  // TODO(markusheintz): I don't like this. I'd like to have an
  // IncognitoProviderWrapper that wrapps the pref provider for a host content
  // settings map of an incognito profile.
  base::AutoLock auto_lock(lock_);
  Value* incognito_value = incognito_value_map_.GetValue(
      primary_url,
      secondary_url,
      content_type,
      resource_identifier);
  if (incognito_value)
    return ValueToContentSetting(incognito_value);

  Value* value = value_map_.GetValue(
      primary_url,
      secondary_url,
      content_type,
      resource_identifier);
  if (value)
    return ValueToContentSetting(value);

  return CONTENT_SETTING_DEFAULT;
}

void PrefProvider::GetAllContentSettingsRules(
    ContentSettingsType content_type,
    const ResourceIdentifier& resource_identifier,
    Rules* content_setting_rules) const {
  DCHECK(content_setting_rules);
  content_setting_rules->clear();

  const OriginIdentifierValueMap* map_to_return =
      is_incognito_ ? &incognito_value_map_ : &value_map_;

  base::AutoLock auto_lock(lock_);
  for (OriginIdentifierValueMap::const_iterator entry = map_to_return->begin();
       entry != map_to_return->end();
       ++entry) {
    if (entry->content_type == content_type &&
        entry->identifier == resource_identifier) {
      ContentSetting setting = ValueToContentSetting(entry->value.get());
      DCHECK(setting != CONTENT_SETTING_DEFAULT);
      Rule new_rule(entry->primary_pattern,
                    entry->secondary_pattern,
                    setting);
      content_setting_rules->push_back(new_rule);
    }
  }
}

void PrefProvider::SetContentSetting(
    const ContentSettingsPattern& primary_pattern,
    const ContentSettingsPattern& secondary_pattern,
    ContentSettingsType content_type,
    const ResourceIdentifier& resource_identifier,
    ContentSetting setting) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  DCHECK(prefs_);
  DCHECK(kTypeNames[content_type] != NULL);  // Don't call this for Geolocation.

  // Update in memory value map.
  OriginIdentifierValueMap* map_to_modify = &incognito_value_map_;
  if (!is_incognito_)
    map_to_modify = &value_map_;

  {
    base::AutoLock auto_lock(lock_);
    if (setting == CONTENT_SETTING_DEFAULT) {
      map_to_modify->DeleteValue(
          primary_pattern,
          secondary_pattern,
          content_type,
          resource_identifier);
    } else {
      map_to_modify->SetValue(
          primary_pattern,
          secondary_pattern,
          content_type,
          resource_identifier,
          Value::CreateIntegerValue(setting));
    }
  }

  // Update the content settings preference.
  if (!is_incognito_) {
    UpdatePref(primary_pattern,
               secondary_pattern,
               content_type,
               resource_identifier,
               setting);
  }

  ContentSettingsDetails details(
      primary_pattern, secondary_pattern, content_type, resource_identifier);
  NotifyObservers(details);
}

void PrefProvider::ClearAllContentSettingsRules(
    ContentSettingsType content_type) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  DCHECK(kTypeNames[content_type] != NULL);  // Don't call this for Geolocation.
  DCHECK(prefs_);

  OriginIdentifierValueMap* map_to_modify = &incognito_value_map_;
  if (!is_incognito_)
    map_to_modify = &value_map_;

  {
    base::AutoLock auto_lock(lock_);
    OriginIdentifierValueMap::iterator entry(map_to_modify->begin());
    while (entry != map_to_modify->end()) {
      if (entry->content_type == content_type) {
        UpdatePref(
            entry->primary_pattern,
            entry->secondary_pattern,
            entry->content_type,
            entry->identifier,
            CONTENT_SETTING_DEFAULT);
        // Delete current |entry| and set |entry| to the next value map entry.
        entry = map_to_modify->erase(entry);
      } else {
        ++entry;
      }
    }
  }

  ContentSettingsDetails details(
      ContentSettingsPattern(),
      ContentSettingsPattern(),
      content_type,
      std::string());
  NotifyObservers(details);
}

void PrefProvider::Observe(
    int type,
    const NotificationSource& source,
    const NotificationDetails& details) {
    DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));

  if (type == chrome::NOTIFICATION_PREF_CHANGED) {
    DCHECK_EQ(prefs_, Source<PrefService>(source).ptr());
    if (updating_preferences_)
      return;

    std::string* name = Details<std::string>(details).ptr();
    if (*name == prefs::kContentSettingsPatternPairs) {
      SyncObsoletePref();
      ReadContentSettingsFromPref(true);
    } else if (*name == prefs::kContentSettingsPatterns) {
      AutoReset<bool> auto_reset(&updating_preferences_, true);
      MigrateObsoleteContentSettingsPatternPref();
      ReadContentSettingsFromPref(true);
    } else {
      NOTREACHED() << "Unexpected preference observed";
      return;
    }

    ContentSettingsDetails details(ContentSettingsPattern(),
                                   ContentSettingsPattern(),
                                   CONTENT_SETTINGS_TYPE_DEFAULT,
                                   std::string());
    NotifyObservers(details);
  } else {
    NOTREACHED() << "Unexpected notification";
  }
}

PrefProvider::~PrefProvider() {
  DCHECK(!prefs_);
}

// ////////////////////////////////////////////////////////////////////////////
// Private

void PrefProvider::UpdatePref(
    const ContentSettingsPattern& primary_pattern,
    const ContentSettingsPattern& secondary_pattern,
    ContentSettingsType content_type,
    const ResourceIdentifier& resource_identifier,
    ContentSetting setting) {
  AutoReset<bool> auto_reset(&updating_preferences_, true);
  UpdatePatternPairsPref(primary_pattern,
                         secondary_pattern,
                         content_type,
                         resource_identifier,
                         setting);
  UpdatePatternsPref(primary_pattern,
                     secondary_pattern,
                     content_type,
                     resource_identifier,
                     setting);
}

void PrefProvider::ReadContentSettingsFromPref(bool overwrite) {
  base::AutoLock auto_lock(lock_);

  const DictionaryValue* all_settings_dictionary =
      prefs_->GetDictionary(prefs::kContentSettingsPatternPairs);

  if (overwrite)
    value_map_.clear();

  AutoReset<bool> auto_reset(&updating_preferences_, true);
  // Careful: The returned value could be NULL if the pref has never been set.
  if (all_settings_dictionary != NULL) {
    DictionaryPrefUpdate update(prefs_, prefs::kContentSettingsPatternPairs);
    DictionaryValue* mutable_settings;
    scoped_ptr<DictionaryValue> mutable_settings_scope;

    if (!is_incognito_) {
      mutable_settings = update.Get();
    } else {
      // Create copy as we do not want to persist anything in OTR prefs.
      mutable_settings = all_settings_dictionary->DeepCopy();
      mutable_settings_scope.reset(mutable_settings);
    }
    // Convert all Unicode patterns into punycode form, then read.
    CanonicalizeContentSettingsExceptions(mutable_settings);

    for (DictionaryValue::key_iterator i(mutable_settings->begin_keys());
         i != mutable_settings->end_keys(); ++i) {
      const std::string& pattern_str(*i);
      std::pair<ContentSettingsPattern, ContentSettingsPattern> pattern_pair =
         ParsePatternString(pattern_str);
      if (!pattern_pair.first.IsValid() ||
          !pattern_pair.second.IsValid()) {
        LOG(DFATAL) << "Invalid pattern strings: " << pattern_str;
        continue;
      }

      // Get settings dictionary for the current pattern string, and read
      // settings from the dictionary.
      DictionaryValue* settings_dictionary = NULL;
      bool found = mutable_settings->GetDictionaryWithoutPathExpansion(
          pattern_str, &settings_dictionary);
      DCHECK(found);

      for (size_t i = 0; i < CONTENT_SETTINGS_NUM_TYPES; ++i) {
        ContentSettingsType content_type = static_cast<ContentSettingsType>(i);

        if (RequiresResourceIdentifier(content_type)) {
          const std::string content_type_str = kResourceTypeNames[i];
          DictionaryValue* resource_dictionary = NULL;
          if (settings_dictionary->GetDictionary(
                  content_type_str, &resource_dictionary)) {
            for (DictionaryValue::key_iterator j(
                     resource_dictionary->begin_keys());
                 j != resource_dictionary->end_keys();
                 ++j) {
              const std::string& resource_identifier(*j);
              int setting = CONTENT_SETTING_DEFAULT;
              found = resource_dictionary->GetIntegerWithoutPathExpansion(
                  resource_identifier, &setting);
              DCHECK_NE(CONTENT_SETTING_DEFAULT, setting);
              setting = ClickToPlayFixup(content_type,
                                         ContentSetting(setting));
              value_map_.SetValue(pattern_pair.first,
                                  pattern_pair.second,
                                  content_type,
                                  resource_identifier,
                                  Value::CreateIntegerValue(setting));
            }
          }
        } else if (kTypeNames[i]) {
          const std::string content_type_str(kTypeNames[i]);
          int setting = CONTENT_SETTING_DEFAULT;
          if (settings_dictionary->GetIntegerWithoutPathExpansion(
                  content_type_str, &setting)) {
            DCHECK_NE(CONTENT_SETTING_DEFAULT, setting);
            setting = FixObsoleteCookiePromptMode(content_type,
                                                  ContentSetting(setting));
            setting = ClickToPlayFixup(content_type,
                                       ContentSetting(setting));
            value_map_.SetValue(pattern_pair.first,
                                pattern_pair.second,
                                content_type,
                                ResourceIdentifier(""),
                                Value::CreateIntegerValue(setting));
          }
        }
      }
    }
  }
}

void PrefProvider::UpdatePatternsPref(
      const ContentSettingsPattern& primary_pattern,
      const ContentSettingsPattern& secondary_pattern,
      ContentSettingsType content_type,
      const ResourceIdentifier& resource_identifier,
      ContentSetting setting) {
  DictionaryPrefUpdate update(prefs_,
                              prefs::kContentSettingsPatterns);
  DictionaryValue* all_settings_dictionary = update.Get();

  // Get settings dictionary for |primary_pattern|.
  std::string pattern_str(primary_pattern.ToString());
  DictionaryValue* settings_dictionary = NULL;
  bool found = all_settings_dictionary->GetDictionaryWithoutPathExpansion(
      pattern_str, &settings_dictionary);

  if (!found && (setting != CONTENT_SETTING_DEFAULT)) {
    settings_dictionary = new DictionaryValue;
    all_settings_dictionary->SetWithoutPathExpansion(
        pattern_str, settings_dictionary);
  }

  if (settings_dictionary) {
    if (RequiresResourceIdentifier(content_type)) {
      // Get resource dictionary.
      std::string res_dictionary_path(kResourceTypeNames[content_type]);
      DictionaryValue* resource_dictionary = NULL;
      found = settings_dictionary->GetDictionary(
          res_dictionary_path, &resource_dictionary);
      if (!found) {
        if (setting == CONTENT_SETTING_DEFAULT)
          return;  // Nothing to remove. Exit early.
        resource_dictionary = new DictionaryValue;
        settings_dictionary->Set(res_dictionary_path, resource_dictionary);
      }
      // Update resource dictionary.
      if (setting == CONTENT_SETTING_DEFAULT) {
        resource_dictionary->RemoveWithoutPathExpansion(resource_identifier,
                                                        NULL);
        if (resource_dictionary->empty()) {
          settings_dictionary->RemoveWithoutPathExpansion(
              res_dictionary_path, NULL);
        }
      } else {
        resource_dictionary->SetWithoutPathExpansion(
            resource_identifier, Value::CreateIntegerValue(setting));
      }
    } else {
      // Update settings dictionary.
      std::string setting_path(kTypeNames[content_type]);
      if (setting == CONTENT_SETTING_DEFAULT) {
        settings_dictionary->RemoveWithoutPathExpansion(setting_path,
                                                        NULL);
      } else {
        settings_dictionary->SetWithoutPathExpansion(
            setting_path, Value::CreateIntegerValue(setting));
      }
    }
    // Remove the settings dictionary if it is empty.
    if (settings_dictionary->empty()) {
      all_settings_dictionary->RemoveWithoutPathExpansion(
          pattern_str, NULL);
    }
  }
}

void PrefProvider::UpdatePatternPairsPref(
      const ContentSettingsPattern& primary_pattern,
      const ContentSettingsPattern& secondary_pattern,
      ContentSettingsType content_type,
      const ResourceIdentifier& resource_identifier,
      ContentSetting setting) {
  DictionaryPrefUpdate update(prefs_,
                              prefs::kContentSettingsPatternPairs);
  DictionaryValue* all_settings_dictionary = update.Get();

  // Get settings dictionary for the given patterns.
  std::string pattern_str(CreatePatternString(primary_pattern,
                                              secondary_pattern));
  DictionaryValue* settings_dictionary = NULL;
  bool found = all_settings_dictionary->GetDictionaryWithoutPathExpansion(
      pattern_str, &settings_dictionary);

  if (!found && (setting != CONTENT_SETTING_DEFAULT)) {
    settings_dictionary = new DictionaryValue;
    all_settings_dictionary->SetWithoutPathExpansion(
        pattern_str, settings_dictionary);
  }

  if (settings_dictionary) {
    if (RequiresResourceIdentifier(content_type)) {
      // Get resource dictionary.
      std::string res_dictionary_path(kResourceTypeNames[content_type]);
      DictionaryValue* resource_dictionary = NULL;
      found = settings_dictionary->GetDictionary(
          res_dictionary_path, &resource_dictionary);
      if (!found) {
        if (setting == CONTENT_SETTING_DEFAULT)
          return;  // Nothing to remove. Exit early.
        resource_dictionary = new DictionaryValue;
        settings_dictionary->Set(res_dictionary_path, resource_dictionary);
      }
      // Update resource dictionary.
      if (setting == CONTENT_SETTING_DEFAULT) {
        resource_dictionary->RemoveWithoutPathExpansion(resource_identifier,
                                                        NULL);
        if (resource_dictionary->empty()) {
          settings_dictionary->RemoveWithoutPathExpansion(
              res_dictionary_path, NULL);
        }
      } else {
        resource_dictionary->SetWithoutPathExpansion(
            resource_identifier, Value::CreateIntegerValue(setting));
      }
    } else {
      // Update settings dictionary.
      std::string setting_path(kTypeNames[content_type]);
      if (setting == CONTENT_SETTING_DEFAULT) {
        settings_dictionary->RemoveWithoutPathExpansion(setting_path,
                                                        NULL);
      } else {
        settings_dictionary->SetWithoutPathExpansion(
            setting_path, Value::CreateIntegerValue(setting));
      }
    }
    // Remove the settings dictionary if it is empty.
    if (settings_dictionary->empty()) {
      all_settings_dictionary->RemoveWithoutPathExpansion(
          pattern_str, NULL);
    }
  }
}

// static
void PrefProvider::CanonicalizeContentSettingsExceptions(
    DictionaryValue* all_settings_dictionary) {
  DCHECK(all_settings_dictionary);

  std::vector<std::string> remove_items;
  std::vector<std::pair<std::string, std::string> > move_items;
  for (DictionaryValue::key_iterator i(all_settings_dictionary->begin_keys());
       i != all_settings_dictionary->end_keys(); ++i) {
    const std::string& pattern_str(*i);
    std::pair<ContentSettingsPattern, ContentSettingsPattern> pattern_pair =
         ParsePatternString(pattern_str);

    const std::string canonicalized_pattern_str = CreatePatternString(
        pattern_pair.first, pattern_pair.second);

    if (canonicalized_pattern_str.empty() ||
        canonicalized_pattern_str == pattern_str)
      continue;

    // Clear old pattern if prefs already have canonicalized pattern.
    DictionaryValue* new_pattern_settings_dictionary = NULL;
    if (all_settings_dictionary->GetDictionaryWithoutPathExpansion(
            canonicalized_pattern_str, &new_pattern_settings_dictionary)) {
      remove_items.push_back(pattern_str);
      continue;
    }

    // Move old pattern to canonicalized pattern.
    DictionaryValue* old_pattern_settings_dictionary = NULL;
    if (all_settings_dictionary->GetDictionaryWithoutPathExpansion(
            pattern_str, &old_pattern_settings_dictionary)) {
      move_items.push_back(
          std::make_pair(pattern_str, canonicalized_pattern_str));
    }
  }

  for (size_t i = 0; i < remove_items.size(); ++i) {
    all_settings_dictionary->RemoveWithoutPathExpansion(remove_items[i], NULL);
  }

  for (size_t i = 0; i < move_items.size(); ++i) {
    Value* pattern_settings_dictionary = NULL;
    all_settings_dictionary->RemoveWithoutPathExpansion(
        move_items[i].first, &pattern_settings_dictionary);
    all_settings_dictionary->SetWithoutPathExpansion(
        move_items[i].second, pattern_settings_dictionary);
  }
}

void PrefProvider::NotifyObservers(
    const ContentSettingsDetails& details) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  DCHECK(host_content_settings_map_);
  NotificationService::current()->Notify(
      chrome::NOTIFICATION_CONTENT_SETTINGS_CHANGED,
      Source<HostContentSettingsMap>(host_content_settings_map_),
      Details<const ContentSettingsDetails>(&details));
}

void PrefProvider::ShutdownOnUIThread() {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  DCHECK(prefs_);
  pref_change_registrar_.RemoveAll();
  prefs_ = NULL;
  host_content_settings_map_ = NULL;
}

void PrefProvider::MigrateObsoletePerhostPref() {
  if (prefs_->HasPrefPath(prefs::kPerHostContentSettings)) {
    const DictionaryValue* all_settings_dictionary =
        prefs_->GetDictionary(prefs::kPerHostContentSettings);
    DCHECK(all_settings_dictionary);
    for (DictionaryValue::key_iterator
         i(all_settings_dictionary->begin_keys());
         i != all_settings_dictionary->end_keys(); ++i) {
      const std::string& host(*i);
      ContentSettingsPattern pattern =
          ContentSettingsPattern::FromString(
              std::string(ContentSettingsPattern::kDomainWildcard) + host);
      DictionaryValue* host_settings_dictionary = NULL;
      bool found = all_settings_dictionary->GetDictionaryWithoutPathExpansion(
          host, &host_settings_dictionary);
      DCHECK(found);

      for (DictionaryValue::key_iterator i(
               host_settings_dictionary->begin_keys());
           i != host_settings_dictionary->end_keys();
           ++i) {
        const std::string& content_type_str(*i);
        ContentSettingsType content_type =
            StringToContentSettingsType(content_type_str);

        if (content_type == CONTENT_SETTINGS_TYPE_DEFAULT) {
          NOTREACHED();
          LOG(DFATAL) << "Skip settings for invalid content settings type '"
                      << content_type_str << "'";
          continue;
        }

        if (!RequiresResourceIdentifier(content_type)) {
          int setting_int_value = CONTENT_SETTING_DEFAULT;
          bool found =
              host_settings_dictionary->GetIntegerWithoutPathExpansion(
                  content_type_str, &setting_int_value);
          DCHECK(found);
          ContentSetting setting = IntToContentSetting(setting_int_value);

          setting = FixObsoleteCookiePromptMode(content_type, setting);
          setting = ClickToPlayFixup(content_type, setting);

          // TODO(markusheintz): Maybe this check can be removed.
          if (setting != CONTENT_SETTING_DEFAULT) {
            SetContentSetting(
                pattern,
                pattern,
                content_type,
                "",
                setting);
          }
        }
      }
    }
    prefs_->ClearPref(prefs::kPerHostContentSettings);
  }
}

void PrefProvider::MigrateObsoletePopupsPref() {
  if (prefs_->HasPrefPath(prefs::kPopupWhitelistedHosts)) {
    const ListValue* whitelist_pref =
        prefs_->GetList(prefs::kPopupWhitelistedHosts);
    for (ListValue::const_iterator i(whitelist_pref->begin());
         i != whitelist_pref->end(); ++i) {
      std::string host;
      (*i)->GetAsString(&host);
      SetContentSetting(ContentSettingsPattern::FromString(host),
                        ContentSettingsPattern::FromString(host),
                        CONTENT_SETTINGS_TYPE_POPUPS,
                        "",
                        CONTENT_SETTING_ALLOW);
    }
    prefs_->ClearPref(prefs::kPopupWhitelistedHosts);
  }
}

void PrefProvider::MigrateObsoleteContentSettingsPatternPref() {
  if (prefs_->HasPrefPath(prefs::kContentSettingsPatterns) && !is_incognito_) {
    const DictionaryValue* all_settings_dictionary =
      prefs_->GetDictionary(prefs::kContentSettingsPatterns);

    DictionaryPrefUpdate update(prefs_, prefs::kContentSettingsPatternPairs);
    DictionaryValue* exceptions_dictionary;
    exceptions_dictionary = update.Get();
    for (DictionaryValue::key_iterator i(all_settings_dictionary->begin_keys());
         i != all_settings_dictionary->end_keys();
         ++i) {
      const std::string& key(*i);

      // Validate pattern string and skip it if it is invalid.
      std::pair<ContentSettingsPattern, ContentSettingsPattern> pattern_pair =
          ParsePatternString(key);
      ContentSettingsPattern primary_pattern = pattern_pair.first;
      if (!primary_pattern.IsValid())
        continue;

      // Copy dictionary value.
      // Get old settings.
      DictionaryValue* dictionary = NULL;
      bool found = all_settings_dictionary->GetDictionaryWithoutPathExpansion(
          key, &dictionary);
      DCHECK(found);

      // Create new dictionary key.
      std::string new_pattern_str = CreatePatternString(
          primary_pattern, ContentSettingsPattern::Wildcard());

      // Existing values are overwritten.
      exceptions_dictionary->SetWithoutPathExpansion(
          new_pattern_str, dictionary->DeepCopy());
    }
  }
}

void PrefProvider::SyncObsoletePref() {
  AutoReset<bool> auto_reset(&updating_preferences_, true);
  if (prefs_->HasPrefPath(prefs::kContentSettingsPatternPairs) &&
      !is_incognito_) {
    const DictionaryValue* pattern_pairs_dictionary =
      prefs_->GetDictionary(prefs::kContentSettingsPatternPairs);

    DictionaryPrefUpdate update(prefs_, prefs::kContentSettingsPatterns);
    DictionaryValue* obsolete_settings_dictionary = update.Get();

    for (DictionaryValue::key_iterator i =
             pattern_pairs_dictionary->begin_keys();
         i != pattern_pairs_dictionary->end_keys();
         ++i) {
      const std::string& key(*i);
      // Validate pattern string and skip it if it is invalid.
      std::pair<ContentSettingsPattern, ContentSettingsPattern> pattern_pair =
          ParsePatternString(key);
      if (!pattern_pair.first.IsValid() || !pattern_pair.second.IsValid())
        continue;

      // Copy dictionary
      DictionaryValue* dictionary = NULL;
      bool found = pattern_pairs_dictionary->GetDictionaryWithoutPathExpansion(
          key, &dictionary);
      DCHECK(found);
      std::string new_key = pattern_pair.first.ToString();
      // Existing values are overwritten.
      obsolete_settings_dictionary->SetWithoutPathExpansion(
          new_key, dictionary->DeepCopy());
    }
  }
}

}  // namespace content_settings
