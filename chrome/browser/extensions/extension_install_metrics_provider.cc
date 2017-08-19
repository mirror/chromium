// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/extensions/extension_install_metrics_provider.h"

#include "chrome/browser/browser_process.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "components/metrics/proto/chrome_user_metrics_extension.pb.h"
#include "extensions/browser/extension_prefs.h"
#include "extensions/browser/extension_registry.h"
#include "extensions/common/extension.h"
#include "extensions/common/manifest.h"
#include "extensions/common/manifest_constants.h"
#include "extensions/common/manifest_handlers/background_info.h"
#include "extensions/common/manifest_url_handlers.h"

using metrics::ExtensionInstallProto;

namespace extensions {

namespace {

ExtensionInstallProto::Type GetType(Manifest::Type type) {
  switch (type) {
    case Manifest::TYPE_UNKNOWN:
      return ExtensionInstallProto::UNKNOWN_TYPE;
    case Manifest::TYPE_EXTENSION:
      return ExtensionInstallProto::EXTENSION;
    case Manifest::TYPE_THEME:
      return ExtensionInstallProto::THEME;
    case Manifest::TYPE_USER_SCRIPT:
      return ExtensionInstallProto::USER_SCRIPT;
    case Manifest::TYPE_HOSTED_APP:
      return ExtensionInstallProto::HOSTED_APP;
    case Manifest::TYPE_LEGACY_PACKAGED_APP:
      return ExtensionInstallProto::LEGACY_PACKAGED_APP;
    case Manifest::TYPE_PLATFORM_APP:
      return ExtensionInstallProto::PLATFORM_APP;
    case Manifest::TYPE_SHARED_MODULE:
      return ExtensionInstallProto::SHARED_MODULE;
    case Manifest::NUM_LOAD_TYPES:
      NOTREACHED();
      // Fall through.
  }
  return ExtensionInstallProto::UNKNOWN_TYPE;
}

ExtensionInstallProto::InstallLocation GetInstallLocation(
    Manifest::Location location) {
  switch (location) {
    case Manifest::INVALID_LOCATION:
      return ExtensionInstallProto::UNKNOWN_LOCATION;
    case Manifest::INTERNAL:
      return ExtensionInstallProto::INTERNAL;
    case Manifest::EXTERNAL_PREF:
      return ExtensionInstallProto::EXTERNAL_PREF;
    case Manifest::EXTERNAL_REGISTRY:
      return ExtensionInstallProto::EXTERNAL_REGISTRY;
    case Manifest::UNPACKED:
      return ExtensionInstallProto::UNPACKED;
    case Manifest::COMPONENT:
      return ExtensionInstallProto::COMPONENT;
    case Manifest::EXTERNAL_PREF_DOWNLOAD:
      return ExtensionInstallProto::EXTERNAL_PREF_DOWNLOAD;
    case Manifest::EXTERNAL_POLICY_DOWNLOAD:
      return ExtensionInstallProto::EXTERNAL_POLICY_DOWNLOAD;
    case Manifest::COMMAND_LINE:
      return ExtensionInstallProto::COMMAND_LINE;
    case Manifest::EXTERNAL_POLICY:
      return ExtensionInstallProto::EXTERNAL_POLICY;
    case Manifest::EXTERNAL_COMPONENT:
      return ExtensionInstallProto::EXTERNAL_COMPONENT;
    case Manifest::NUM_LOCATIONS:
      NOTREACHED();
      // Fall through.
  }
  return ExtensionInstallProto::UNKNOWN_LOCATION;
}

ExtensionInstallProto::ActionType GetActionType(const Manifest& manifest) {
  if (manifest.HasKey(manifest_keys::kBrowserAction))
    return ExtensionInstallProto::BROWSER_ACTION;
  if (manifest.HasKey(manifest_keys::kPageAction))
    return ExtensionInstallProto::PAGE_ACTION;
  if (manifest.HasKey(manifest_keys::kSystemIndicator))
    return ExtensionInstallProto::SYSTEM_INDICATOR;
  return ExtensionInstallProto::NO_ACTION;
}

ExtensionInstallProto::BackgroundScriptType GetBackgroundScriptType(
    const Extension& extension) {
  if (BackgroundInfo::HasPersistentBackgroundPage(&extension))
    return ExtensionInstallProto::PERSISTENT_BACKGROUND_PAGE;
  if (BackgroundInfo::HasLazyBackgroundPage(&extension))
    return ExtensionInstallProto::EVENT_PAGE;
  DCHECK(!BackgroundInfo::HasBackgroundPage(&extension));
  return ExtensionInstallProto::NO_BACKGROUND_SCRIPT;
}

std::vector<ExtensionInstallProto::DisableReason> GetDisableReasons(
    const ExtensionId& id,
    ExtensionPrefs* prefs) {
  int disable_reasons = prefs->GetDisableReasons(id);
  std::vector<ExtensionInstallProto::DisableReason> reasons;
  if ((disable_reasons & Extension::DISABLE_USER_ACTION) != 0)
    reasons.push_back(ExtensionInstallProto::USER_ACTION);
  if ((disable_reasons & Extension::DISABLE_PERMISSIONS_INCREASE) != 0)
    reasons.push_back(ExtensionInstallProto::PERMISSIONS_INCREASE);
  if ((disable_reasons & Extension::DISABLE_RELOAD) != 0)
    reasons.push_back(ExtensionInstallProto::RELOAD);
  if ((disable_reasons & Extension::DISABLE_UNSUPPORTED_REQUIREMENT) != 0)
    reasons.push_back(ExtensionInstallProto::UNSUPPORTED_REQUIREMENT);
  if ((disable_reasons & Extension::DISABLE_SIDELOAD_WIPEOUT) != 0)
    reasons.push_back(ExtensionInstallProto::SIDELOAD_WIPEOUT);
  if ((disable_reasons & Extension::DEPRECATED_DISABLE_UNKNOWN_FROM_SYNC) != 0)
    reasons.push_back(ExtensionInstallProto::UNKNOWN_FROM_SYNC);
  if ((disable_reasons & Extension::DISABLE_NOT_VERIFIED) != 0)
    reasons.push_back(ExtensionInstallProto::NOT_VERIFIED);
  if ((disable_reasons & Extension::DISABLE_GREYLIST) != 0)
    reasons.push_back(ExtensionInstallProto::GREYLIST);
  if ((disable_reasons & Extension::DISABLE_CORRUPTED) != 0)
    reasons.push_back(ExtensionInstallProto::CORRUPTED);
  if ((disable_reasons & Extension::DISABLE_REMOTE_INSTALL) != 0)
    reasons.push_back(ExtensionInstallProto::REMOTE_INSTALL);
  if ((disable_reasons & Extension::DISABLE_EXTERNAL_EXTENSION) != 0)
    reasons.push_back(ExtensionInstallProto::EXTERNAL_EXTENSION);
  if ((disable_reasons & Extension::DISABLE_UPDATE_REQUIRED_BY_POLICY) != 0)
    reasons.push_back(ExtensionInstallProto::UPDATE_REQUIRED_BY_POLICY);
  if ((disable_reasons & Extension::DISABLE_CUSTODIAN_APPROVAL_REQUIRED) != 0)
    reasons.push_back(ExtensionInstallProto::CUSTODIAN_APPROVAL_REQUIRED);

  return reasons;
}

ExtensionInstallProto::BlacklistState GetBlacklistState(const ExtensionId& id,
                                                        ExtensionPrefs* prefs) {
  BlacklistState state = prefs->GetExtensionBlacklistState(id);
  switch (state) {
    case NOT_BLACKLISTED:
      return ExtensionInstallProto::NOT_BLACKLISTED;
    case BLACKLISTED_MALWARE:
      return ExtensionInstallProto::BLACKLISTED_MALWARE;
    case BLACKLISTED_SECURITY_VULNERABILITY:
      return ExtensionInstallProto::BLACKLISTED_SECURITY_VULNERABILITY;
    case BLACKLISTED_CWS_POLICY_VIOLATION:
      return ExtensionInstallProto::BLACKLISTED_CWS_POLICY_VIOLATION;
    case BLACKLISTED_POTENTIALLY_UNWANTED:
      return ExtensionInstallProto::BLACKLISTED_POTENTIALLY_UNWANTED;
    case BLACKLISTED_UNKNOWN:
      return ExtensionInstallProto::BLACKLISTED_UNKNOWN;
  }
  NOTREACHED();
  return ExtensionInstallProto::BLACKLISTED_UNKNOWN;
}

}  // namespace

ExtensionInstallMetricsProvider::ExtensionInstallMetricsProvider() {}
ExtensionInstallMetricsProvider::~ExtensionInstallMetricsProvider() {}

void ExtensionInstallMetricsProvider::ProvideCurrentSessionData(
    metrics::ChromeUserMetricsExtension* uma_proto) {
  std::vector<Profile*> profiles =
      g_browser_process->profile_manager()->GetLoadedProfiles();
  for (Profile* profile : profiles) {
    std::vector<ExtensionInstallProto> installs =
        GetInstallsForProfile(profile);
    for (ExtensionInstallProto& install : installs)
      uma_proto->add_extension_install()->Swap(&install);
  }
}

ExtensionInstallProto ExtensionInstallMetricsProvider::ConstructProto(
    const Extension& extension,
    ExtensionPrefs* prefs) {
  ExtensionInstallProto install;
  install.set_type(GetType(extension.manifest()->type()));
  install.set_install_location(GetInstallLocation(extension.location()));
  install.set_manifest_version(extension.manifest_version());
  install.set_action_type(GetActionType(*extension.manifest()));
  install.set_has_file_access(
      (extension.creation_flags() & Extension::ALLOW_FILE_ACCESS) != 0);
  install.set_has_incognito_access(prefs->IsIncognitoEnabled(extension.id()));
  install.set_is_from_store(extension.from_webstore());
  install.set_updates_from_store(ManifestURL::UpdatesFromGallery(&extension));
  install.set_is_from_bookmark(extension.from_bookmark());
  install.set_is_converted_from_user_script(
      extension.converted_from_user_script());
  install.set_is_default_installed(extension.was_installed_by_default());
  install.set_is_oem_installed(extension.was_installed_by_oem());
  install.set_background_script_type(GetBackgroundScriptType(extension));
  for (const ExtensionInstallProto::DisableReason reason :
       GetDisableReasons(extension.id(), prefs)) {
    install.add_disable_reasons(reason);
  }
  install.set_blacklist_state(GetBlacklistState(extension.id(), prefs));

  return install;
}

std::vector<ExtensionInstallProto>
ExtensionInstallMetricsProvider::GetInstallsForProfile(Profile* profile) {
  ExtensionPrefs* prefs = ExtensionPrefs::Get(profile);
  std::vector<ExtensionInstallProto> installs;
  std::unique_ptr<ExtensionSet> extensions =
      ExtensionRegistry::Get(profile)->GenerateInstalledExtensionsSet();
  installs.reserve(extensions->size());
  for (const auto& extension : *extensions)
    installs.push_back(ConstructProto(*extension, prefs));

  return installs;
}

}  // namespace extensions
