// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/common/manifest_handlers/kiosk_mode_info.h"

#include <memory>

#include "base/memory/ptr_util.h"
#include "base/strings/string16.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "base/values.h"
#include "base/version.h"
#include "extensions/common/api/extensions_manifest_types.h"
#include "extensions/common/features/behavior_feature.h"
#include "extensions/common/features/feature.h"
#include "extensions/common/features/feature_provider.h"
#include "extensions/common/manifest_constants.h"

namespace extensions {

namespace keys = manifest_keys;

using api::extensions_manifest_types::KioskSecondaryAppsType;

namespace {

// Whether "disable_on_startup" manifest property for the extension should be
// respected or not. If false, secondary apps that specify this property will
// be ignored.
bool CanDisableSecondaryKioskAppOnStartup(const Extension* extension) {
  if (!extension)
    return false;

  const Feature* feature = FeatureProvider::GetBehaviorFeatures()->GetFeature(
      behavior_feature::kAllowDisableSecondaryKioskAppOnStartup);
  if (!feature)
    return false;

  return feature->IsAvailableToExtension(extension).is_available();
}

}  // namespace

SecondaryKioskAppInfo::SecondaryKioskAppInfo(const extensions::ExtensionId& id,
                                             bool disable_on_startup)
    : id(id), disable_on_startup(disable_on_startup) {}

SecondaryKioskAppInfo::~SecondaryKioskAppInfo() = default;

KioskModeInfo::KioskModeInfo(
    KioskStatus kiosk_status,
    const std::vector<SecondaryKioskAppInfo>& secondary_apps,
    const std::string& required_platform_version,
    bool always_update)
    : kiosk_status(kiosk_status),
      secondary_apps(secondary_apps),
      required_platform_version(required_platform_version),
      always_update(always_update) {}

KioskModeInfo::~KioskModeInfo() {
}

// static
KioskModeInfo* KioskModeInfo::Get(const Extension* extension) {
  return static_cast<KioskModeInfo*>(
      extension->GetManifestData(keys::kKioskMode));
}

// static
bool KioskModeInfo::IsKioskEnabled(const Extension* extension) {
  KioskModeInfo* info = Get(extension);
  return info ? info->kiosk_status != NONE : false;
}

// static
bool KioskModeInfo::IsKioskOnly(const Extension* extension) {
  KioskModeInfo* info = Get(extension);
  return info ? info->kiosk_status == ONLY : false;
}

// static
bool KioskModeInfo::HasSecondaryApps(const Extension* extension) {
  KioskModeInfo* info = Get(extension);
  return info && !info->secondary_apps.empty();
}

// static
bool KioskModeInfo::IsValidPlatformVersion(const std::string& version_string) {
  const base::Version version(version_string);
  return version.IsValid() && version.components().size() <= 3u;
}

KioskModeHandler::KioskModeHandler() {
  supported_keys_.push_back(keys::kKiosk);
  supported_keys_.push_back(keys::kKioskEnabled);
  supported_keys_.push_back(keys::kKioskOnly);
  supported_keys_.push_back(keys::kKioskSecondaryApps);
}

KioskModeHandler::~KioskModeHandler() {
}

bool KioskModeHandler::Parse(Extension* extension, base::string16* error) {
  const Manifest* manifest = extension->manifest();
  DCHECK(manifest->HasKey(keys::kKioskEnabled) ||
         manifest->HasKey(keys::kKioskOnly));

  bool kiosk_enabled = false;
  if (manifest->HasKey(keys::kKioskEnabled) &&
      !manifest->GetBoolean(keys::kKioskEnabled, &kiosk_enabled)) {
    *error = base::ASCIIToUTF16(manifest_errors::kInvalidKioskEnabled);
    return false;
  }

  bool kiosk_only = false;
  if (manifest->HasKey(keys::kKioskOnly) &&
      !manifest->GetBoolean(keys::kKioskOnly, &kiosk_only)) {
    *error = base::ASCIIToUTF16(manifest_errors::kInvalidKioskOnly);
    return false;
  }

  if (kiosk_only && !kiosk_enabled) {
    *error = base::ASCIIToUTF16(
        manifest_errors::kInvalidKioskOnlyButNotEnabled);
    return false;
  }

  // All other use cases should be already filtered out by manifest feature
  // checks.
  DCHECK(extension->is_platform_app());

  KioskModeInfo::KioskStatus kiosk_status = KioskModeInfo::NONE;
  if (kiosk_enabled)
    kiosk_status = kiosk_only ? KioskModeInfo::ONLY : KioskModeInfo::ENABLED;

  // Kiosk secondary apps key is optional.
  std::map<std::string, SecondaryKioskAppInfo> secondary_apps;
  if (manifest->HasKey(keys::kKioskSecondaryApps)) {
    const base::Value* secondary_apps_value = nullptr;
    const base::ListValue* list = nullptr;
    if (!manifest->Get(keys::kKioskSecondaryApps, &secondary_apps_value) ||
        !secondary_apps_value->GetAsList(&list)) {
      *error = base::ASCIIToUTF16(manifest_errors::kInvalidKioskSecondaryApps);
      return false;
    }

    const bool allow_disable_on_startup =
        CanDisableSecondaryKioskAppOnStartup(extension);

    for (const auto& value : *list) {
      std::unique_ptr<KioskSecondaryAppsType> app =
          KioskSecondaryAppsType::FromValue(value, error);
      if (!app) {
        *error = base::ASCIIToUTF16(
            manifest_errors::kInvalidKioskSecondaryAppsBadAppId);
        return false;
      }

      if (secondary_apps.count(app->id))
        continue;

      if (app->disable_on_startup && !allow_disable_on_startup)
        continue;

      secondary_apps.emplace(
          app->id,
          SecondaryKioskAppInfo(
              app->id, app->disable_on_startup && *app->disable_on_startup));
    }
  }

  // Optional kiosk.required_platform_version key.
  std::string required_platform_version;
  if (manifest->HasPath(keys::kKioskRequiredPlatformVersion) &&
      (!manifest->GetString(keys::kKioskRequiredPlatformVersion,
                            &required_platform_version) ||
       !KioskModeInfo::IsValidPlatformVersion(required_platform_version))) {
    *error = base::ASCIIToUTF16(
        manifest_errors::kInvalidKioskRequiredPlatformVersion);
    return false;
  }

  // Optional kiosk.always_update key.
  bool always_update = false;
  if (manifest->HasPath(keys::kKioskAlwaysUpdate) &&
      !manifest->GetBoolean(keys::kKioskAlwaysUpdate, &always_update)) {
    *error = base::ASCIIToUTF16(manifest_errors::kInvalidKioskAlwaysUpdate);
    return false;
  }

  std::vector<SecondaryKioskAppInfo> secondary_apps_vector;
  for (const auto& app : secondary_apps)
    secondary_apps_vector.emplace_back(std::move(app.second));
  extension->SetManifestData(keys::kKioskMode,
                             std::make_unique<KioskModeInfo>(
                                 kiosk_status, secondary_apps_vector,
                                 required_platform_version, always_update));

  return true;
}

const std::vector<std::string> KioskModeHandler::Keys() const {
  return supported_keys_;
}

}  // namespace extensions
