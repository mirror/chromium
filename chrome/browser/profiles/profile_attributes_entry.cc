// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <utility>

#include "base/strings/utf_string_conversions.h"
#include "build/build_config.h"
#include "chrome/browser/profiles/profile_attributes_entry.h"
#include "chrome/browser/profiles/profile_avatar_icon_util.h"
#include "chrome/browser/profiles/profile_info_cache.h"
#include "chrome/browser/signin/signin_util.h"
#include "chrome/common/pref_names.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/scoped_user_pref_update.h"
#include "ui/base/resource/resource_bundle.h"

namespace {

const char kShortcutNameKey[] = "shortcut_name";
const char kGAIANameKey[] = "gaia_name";
const char kGAIAGivenNameKey[] = "gaia_given_name";
const char kGAIAIdKey[] = "gaia_id";
const char kActiveTimeKey[] = "active_time";
const char kUserNameKey[] = "user_name";
const char kIsUsingDefaultAvatarKey[] = "is_using_default_avatar";
const char kAvatarIconKey[] = "avatar_icon";
const char kAuthCredentialsKey[] = "local_auth_credentials";
const char kPasswordTokenKey[] = "gaia_password_token";
const char kUseGAIAPictureKey[] = "use_gaia_picture";
const char kBackgroundAppsKey[] = "background_apps";
const char kGAIAPictureFileNameKey[] = "gaia_picture_file_name";
const char kProfileIsEphemeral[] = "is_ephemeral";
const char kIsAuthErrorKey[] = "is_auth_error";

}  // namespace

ProfileAttributesEntry::ProfileAttributesEntry()
    : profile_info_cache_(nullptr),
      prefs_(nullptr),
      profile_path_(base::FilePath()) {}

void ProfileAttributesEntry::Initialize(ProfileInfoCache* cache,
                                        const base::FilePath& path,
                                        PrefService* prefs) {
  DCHECK(!profile_info_cache_);
  DCHECK(cache);
  profile_info_cache_ = cache;

  DCHECK(profile_path_.empty());
  DCHECK(!path.empty());
  profile_path_ = path;

  DCHECK(!prefs_);
  DCHECK(prefs);
  prefs_ = prefs;

  DCHECK(profile_info_cache_->GetUserDataDir() == profile_path_.DirName());
  storage_key_ = profile_path_.BaseName().MaybeAsASCII();

  is_force_signin_enabled_ = signin_util::IsForceSigninEnabled();
  if (!IsAuthenticated() && is_force_signin_enabled_)
    is_force_signin_profile_locked_ = true;
}

base::string16 ProfileAttributesEntry::GetName() const {
  return profile_info_cache_->GetNameOfProfileAtIndex(profile_index());
}

base::string16 ProfileAttributesEntry::GetShortcutName() const {
  return GetString16(kShortcutNameKey);
}

base::FilePath ProfileAttributesEntry::GetPath() const {
  return profile_path_;
}

base::Time ProfileAttributesEntry::GetActiveTime() const {
  if (IsDouble(kActiveTimeKey)) {
    return base::Time::FromDoubleT(GetDouble(kActiveTimeKey));
  } else {
    return base::Time();
  }
}

base::string16 ProfileAttributesEntry::GetUserName() const {
  return GetString16(kUserNameKey);
}

const gfx::Image& ProfileAttributesEntry::GetAvatarIcon() const {
  if (IsUsingGAIAPicture()) {
    const gfx::Image* image = GetGAIAPicture();
    if (image)
      return *image;
  }

#if !defined(OS_CHROMEOS) && !defined(OS_ANDROID)
  // Use the high resolution version of the avatar if it exists. Mobile and
  // ChromeOS don't need the high resolution version so no need to fetch it.
  const gfx::Image* image = GetHighResAvatar();
  if (image)
    return *image;
#endif

  int resource_id =
      profiles::GetDefaultAvatarIconResourceIDAtIndex(GetAvatarIconIndex());
  return ResourceBundle::GetSharedInstance().GetNativeImageNamed(resource_id);
}

std::string ProfileAttributesEntry::GetLocalAuthCredentials() const {
  return GetString(kAuthCredentialsKey);
}

std::string ProfileAttributesEntry::GetPasswordChangeDetectionToken() const {
  return GetString(kPasswordTokenKey);
}

bool ProfileAttributesEntry::GetBackgroundStatus() const {
  return GetBool(kBackgroundAppsKey);
}

base::string16 ProfileAttributesEntry::GetGAIAName() const {
  return GetString16(kGAIANameKey);
}

base::string16 ProfileAttributesEntry::GetGAIAGivenName() const {
  return GetString16(kGAIAGivenNameKey);
}

std::string ProfileAttributesEntry::GetGAIAId() const {
  return GetString(kGAIAIdKey);
}

const gfx::Image* ProfileAttributesEntry::GetGAIAPicture() const {
  std::string file_name = GetString(kGAIAPictureFileNameKey);

  // If the picture is not on disk then return null.
  if (file_name.empty())
    return nullptr;

  base::FilePath image_path = profile_path_.AppendASCII(file_name);
  return profile_info_cache_->LoadAvatarPictureFromPath(
      profile_path_, storage_key_, image_path);
}

bool ProfileAttributesEntry::IsUsingGAIAPicture() const {
  return GetBool(kUseGAIAPictureKey) ||
         (IsUsingDefaultAvatar() && GetGAIAPicture());
}

bool ProfileAttributesEntry::IsGAIAPictureLoaded() const {
  return profile_info_cache_->HasCachedAvatarImage(storage_key_);
}

bool ProfileAttributesEntry::IsSupervised() const {
  return profile_info_cache_->ProfileIsSupervisedAtIndex(profile_index());
}

bool ProfileAttributesEntry::IsChild() const {
  return profile_info_cache_->ProfileIsChildAtIndex(profile_index());
}

bool ProfileAttributesEntry::IsLegacySupervised() const {
  return profile_info_cache_->ProfileIsLegacySupervisedAtIndex(profile_index());
}

bool ProfileAttributesEntry::IsOmitted() const {
  return profile_info_cache_->IsOmittedProfileAtIndex(profile_index());
}

bool ProfileAttributesEntry::IsSigninRequired() const {
  return profile_info_cache_->ProfileIsSigninRequiredAtIndex(profile_index()) ||
         is_force_signin_profile_locked_;
}

std::string ProfileAttributesEntry::GetSupervisedUserId() const {
  return profile_info_cache_->GetSupervisedUserIdOfProfileAtIndex(
      profile_index());
}

bool ProfileAttributesEntry::IsEphemeral() const {
  return GetBool(kProfileIsEphemeral);
}

bool ProfileAttributesEntry::IsUsingDefaultName() const {
  return profile_info_cache_->ProfileIsUsingDefaultNameAtIndex(profile_index());
}

bool ProfileAttributesEntry::IsAuthenticated() const {
  // The profile is authenticated if the gaia_id of the info is not empty.
  // If it is empty, also check if the user name is not empty.  This latter
  // check is needed in case the profile has not been loaded yet and the
  // gaia_id property has not yet been written.
  return !GetGAIAId().empty() || !GetUserName().empty();
}

bool ProfileAttributesEntry::IsUsingDefaultAvatar() const {
  return profile_info_cache_->ProfileIsUsingDefaultAvatarAtIndex(
      profile_index());
}

bool ProfileAttributesEntry::IsAuthError() const {
  return GetBool(kIsAuthErrorKey);
}

size_t ProfileAttributesEntry::GetAvatarIconIndex() const {
  std::string icon_url = GetString(kAvatarIconKey);
  size_t icon_index = 0;
  if (!profiles::IsDefaultAvatarIconUrl(icon_url, &icon_index))
    DLOG(WARNING) << "Unknown avatar icon: " << icon_url;

  return icon_index;
}

void ProfileAttributesEntry::SetName(const base::string16& name) {
  profile_info_cache_->SetNameOfProfileAtIndex(profile_index(), name);
}

void ProfileAttributesEntry::SetShortcutName(const base::string16& name) {
  SetString16(kShortcutNameKey, name);
}

void ProfileAttributesEntry::SetActiveTimeToNow() {
  if (IsDouble(kActiveTimeKey) &&
      base::Time::Now() - GetActiveTime() < base::TimeDelta::FromHours(1)) {
    return;
  }
  SetDouble(kActiveTimeKey, base::Time::Now().ToDoubleT());
}

void ProfileAttributesEntry::SetIsOmitted(bool is_omitted) {
  profile_info_cache_->SetIsOmittedProfileAtIndex(profile_index(), is_omitted);
}

void ProfileAttributesEntry::SetSupervisedUserId(const std::string& id) {
  profile_info_cache_->SetSupervisedUserIdOfProfileAtIndex(profile_index(), id);
}

void ProfileAttributesEntry::SetLocalAuthCredentials(const std::string& auth) {
  SetString(kAuthCredentialsKey, auth);
}

void ProfileAttributesEntry::SetPasswordChangeDetectionToken(
    const std::string& token) {
  SetString(kPasswordTokenKey, token);
}

void ProfileAttributesEntry::SetBackgroundStatus(bool running_background_apps) {
  SetBool(kBackgroundAppsKey, running_background_apps);
}

void ProfileAttributesEntry::SetGAIAName(const base::string16& name) {
  if (name == GetGAIAName())
    return;

  base::string16 old_display_name = GetName();
  SetString16(kGAIANameKey, name);
  base::string16 new_display_name = GetName();

  base::FilePath profile_path = GetPath();
  profile_info_cache_->UpdateSortForProfileIndex(profile_index());

  if (old_display_name != new_display_name) {
    profile_info_cache_->NotifyOnProfileNameChanged(profile_path,
                                                    old_display_name);
  }
}

void ProfileAttributesEntry::SetGAIAGivenName(const base::string16& name) {
  if (name == GetGAIAGivenName())
    return;

  base::string16 old_display_name = GetName();
  SetString16(kGAIAGivenNameKey, name);
  base::string16 new_display_name = GetName();

  base::FilePath profile_path = GetPath();
  profile_info_cache_->UpdateSortForProfileIndex(profile_index());

  if (old_display_name != new_display_name) {
    profile_info_cache_->NotifyOnProfileNameChanged(profile_path,
                                                    old_display_name);
  }
}

void ProfileAttributesEntry::SetGAIAPicture(const gfx::Image* image) {
  base::FilePath profile_path = GetPath();
  std::string new_file_name =
      profile_info_cache_->SaveGAIAImageAndGetNewAvatarFileName(
          profile_path, image, storage_key_,
          GetString(kGAIAPictureFileNameKey));
  SetString(kGAIAPictureFileNameKey, new_file_name);
  profile_info_cache_->NotifyOnProfileAvatarChanged(profile_path);
}

void ProfileAttributesEntry::SetIsUsingGAIAPicture(bool value) {
  SetBool(kUseGAIAPictureKey, value);
  profile_info_cache_->NotifyOnProfileAvatarChanged(GetPath());
}

void ProfileAttributesEntry::SetIsSigninRequired(bool value) {
  profile_info_cache_->SetProfileSigninRequiredAtIndex(profile_index(), value);
  if (is_force_signin_enabled_)
    LockForceSigninProfile(value);
}

void ProfileAttributesEntry::LockForceSigninProfile(bool is_lock) {
  DCHECK(is_force_signin_enabled_);
  if (is_force_signin_profile_locked_ == is_lock)
    return;
  is_force_signin_profile_locked_ = is_lock;
  profile_info_cache_->NotifyIsSigninRequiredChanged(profile_path_);
}

void ProfileAttributesEntry::SetIsEphemeral(bool value) {
  SetBool(kProfileIsEphemeral, value);
}

void ProfileAttributesEntry::SetIsUsingDefaultName(bool value) {
  profile_info_cache_->SetProfileIsUsingDefaultNameAtIndex(
      profile_index(), value);
}

void ProfileAttributesEntry::SetIsUsingDefaultAvatar(bool value) {
  if (value == IsUsingDefaultAvatar())
    return;
  SetBool(kIsUsingDefaultAvatarKey, value);
}

void ProfileAttributesEntry::SetIsAuthError(bool value) {
  SetBool(kIsAuthErrorKey, value);
}

void ProfileAttributesEntry::SetAvatarIconIndex(size_t icon_index) {
  if (!profiles::IsDefaultAvatarIconIndex(icon_index)) {
    DLOG(WARNING) << "Unknown avatar icon index: " << icon_index;
    // switch to generic avatar
    icon_index = 0;
  }
  SetString(kAvatarIconKey, profiles::GetDefaultAvatarIconUrl(icon_index));

  if (!profile_info_cache_->GetDisableAvatarDownloadForTesting())
    profile_info_cache_->DownloadHighResAvatarIfNeeded(icon_index,
                                                       profile_path_);

  profile_info_cache_->NotifyOnProfileAvatarChanged(profile_path_);
}

void ProfileAttributesEntry::SetAuthInfo(
    const std::string& gaia_id, const base::string16& user_name) {
  profile_info_cache_->SetAuthInfoOfProfileAtIndex(
      profile_index(), gaia_id, user_name);
}

size_t ProfileAttributesEntry::profile_index() const {
  size_t index = profile_info_cache_->GetIndexOfProfileWithPath(profile_path_);
  DCHECK(index < profile_info_cache_->GetNumberOfProfiles());
  return index;
}

const gfx::Image* ProfileAttributesEntry::GetHighResAvatar() const {
  const size_t avatar_index = GetAvatarIconIndex();

  // If this is the placeholder avatar, it is already included in the
  // resources, so it doesn't need to be downloaded.
  if (avatar_index == profiles::GetPlaceholderAvatarIndex()) {
    return &ui::ResourceBundle::GetSharedInstance().GetImageNamed(
        profiles::GetPlaceholderAvatarIconResourceID());
  }

  const std::string key =
      profiles::GetDefaultAvatarIconFileNameAtIndex(avatar_index);
  const base::FilePath image_path =
      profiles::GetPathOfHighResAvatarAtIndex(avatar_index);
  return profile_info_cache_->LoadAvatarPictureFromPath(profile_path_, key,
                                                        image_path);
}

const base::Value* ProfileAttributesEntry::GetEntryData() const {
  const base::DictionaryValue* cache =
      prefs_->GetDictionary(prefs::kProfileInfoCache);
  return cache->FindKeyOfType(storage_key_, base::Value::Type::DICTIONARY);
}

void ProfileAttributesEntry::SetEntryData(base::Value data) {
  DCHECK(data.is_dict());

  DictionaryPrefUpdate update(prefs_, prefs::kProfileInfoCache);
  base::DictionaryValue* cache = update.Get();
  cache->SetKey(storage_key_, std::move(data));
}

const base::Value* ProfileAttributesEntry::GetValue(const char* key) const {
  const base::Value* entry_data = GetEntryData();
  return entry_data ? entry_data->FindKey(key) : nullptr;
}

std::string ProfileAttributesEntry::GetString(const char* key) const {
  const base::Value* value = GetValue(key);
  if (!value || !value->is_string())
    return std::string();
  return value->GetString();
}

base::string16 ProfileAttributesEntry::GetString16(const char* key) const {
  const base::Value* value = GetValue(key);
  if (!value || !value->is_string())
    return base::string16();
  return base::UTF8ToUTF16(value->GetString());
}

double ProfileAttributesEntry::GetDouble(const char* key) const {
  const base::Value* value = GetValue(key);
  if (!value || !value->is_double())
    return 0.0;
  return value->GetDouble();
}

bool ProfileAttributesEntry::GetBool(const char* key) const {
  const base::Value* value = GetValue(key);
  return value && value->is_bool() && value->GetBool();
}

// Type checking. Only IsDouble is implemented because others do not have
// callsites.
bool ProfileAttributesEntry::IsDouble(const char* key) const {
  const base::Value* value = GetValue(key);
  return value && value->is_double();
}

// Internal setters using keys;
bool ProfileAttributesEntry::SetString(const char* key, std::string value) {
  const base::Value* old_data = GetEntryData();
  if (old_data) {
    const base::Value* old_value = old_data->FindKey(key);
    if (old_value && old_value->is_string() && old_value->GetString() == value)
      return false;
  }

  base::Value new_data = old_data ? GetEntryData()->Clone()
                                  : base::Value(base::Value::Type::DICTIONARY);
  new_data.SetKey(key, base::Value(value));
  SetEntryData(std::move(new_data));
  return true;
}

bool ProfileAttributesEntry::SetString16(const char* key,
                                         base::string16 value) {
  const base::Value* old_data = GetEntryData();
  if (old_data) {
    const base::Value* old_value = old_data->FindKey(key);
    if (old_value && old_value->is_string() &&
        base::UTF8ToUTF16(old_value->GetString()) == value)
      return false;
  }

  base::Value new_data = old_data ? GetEntryData()->Clone()
                                  : base::Value(base::Value::Type::DICTIONARY);
  new_data.SetKey(key, base::Value(value));
  SetEntryData(std::move(new_data));
  return true;
}

bool ProfileAttributesEntry::SetDouble(const char* key, double value) {
  const base::Value* old_data = GetEntryData();
  if (old_data) {
    const base::Value* old_value = old_data->FindKey(key);
    if (old_value && old_value->is_double() && old_value->GetDouble() == value)
      return false;
  }

  base::Value new_data = old_data ? GetEntryData()->Clone()
                                  : base::Value(base::Value::Type::DICTIONARY);
  new_data.SetKey(key, base::Value(value));
  SetEntryData(std::move(new_data));
  return true;
}

bool ProfileAttributesEntry::SetBool(const char* key, bool value) {
  const base::Value* old_data = GetEntryData();
  if (old_data) {
    const base::Value* old_value = old_data->FindKey(key);
    if (old_value && old_value->is_bool() && old_value->GetBool() == value)
      return false;
  }

  base::Value new_data = old_data ? GetEntryData()->Clone()
                                  : base::Value(base::Value::Type::DICTIONARY);
  new_data.SetKey(key, base::Value(value));
  SetEntryData(std::move(new_data));
  return true;
}
