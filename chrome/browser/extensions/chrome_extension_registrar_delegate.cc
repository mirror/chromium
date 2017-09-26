// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/extensions/chrome_extension_registrar_delegate.h"

#include <set>

#include "base/logging.h"
#include "chrome/browser/extensions/extension_service.h"
#include "chrome/browser/extensions/extension_special_storage_policy.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/search/thumbnail_source.h"
#include "chrome/browser/ui/webui/favicon_source.h"
#include "chrome/browser/ui/webui/theme_source.h"
#include "chrome/common/crash_keys.h"
#include "chrome/common/url_constants.h"
#include "extensions/browser/extension_registry.h"
#include "extensions/browser/extension_util.h"
#include "extensions/browser/management_policy.h"
#include "extensions/common/manifest_handlers/shared_module_info.h"
#include "extensions/common/permissions/permissions_data.h"

#if defined(OS_CHROMEOS)
#include "content/public/browser/storage_partition.h"
#endif

namespace extensions {

namespace {

// Helper that updates the active extension list used for crash reporting.
void UpdateActiveExtensionsInCrashReporter(ExtensionRegistry* registry) {
  std::set<ExtensionId> extension_ids;
  for (const auto& extension : registry->enabled_extensions()) {
    if (!extension->is_theme() && extension->location() != Manifest::COMPONENT)
      extension_ids.insert(extension->id());
  }

  // TODO(kalman): This is broken. ExtensionService is per-profile.
  // crash_keys::SetActiveExtensions is per-process. See
  // http://crbug.com/355029.
  crash_keys::SetActiveExtensions(extension_ids);
}

// Helper method to determine if an extension can be blocked.
bool CanBlockExtension(const Extension* extension,
                       const ManagementPolicy* management_policy) {
  DCHECK(extension);
  return extension->location() != Manifest::COMPONENT &&
         extension->location() != Manifest::EXTERNAL_COMPONENT &&
         !management_policy->MustRemainEnabled(extension, nullptr);
}

}  // namespace

ChromeExtensionRegistrarDelegate::ChromeExtensionRegistrarDelegate(
    ExtensionService* extension_service,
    Profile* profile,
    ExtensionRegistry* registry,
    ManagementPolicy* management_policy)
    : extension_service_(extension_service),
      profile_(profile),
      registry_(registry),
      management_policy_(management_policy) {}

ChromeExtensionRegistrarDelegate::~ChromeExtensionRegistrarDelegate() = default;

void ChromeExtensionRegistrarDelegate::PostEnableExtension(
    scoped_refptr<const Extension> extension) {
  // TODO(kalman): Convert ExtensionSpecialStoragePolicy to a
  // BrowserContextKeyedService and use ExtensionRegistryObserver.
  profile_->GetExtensionSpecialStoragePolicy()->GrantRightsForExtension(
      extension.get(), profile_);

  // TODO(kalman): This is broken. The crash reporter is process-wide so doesn't
  // work properly multi-profile. Besides which, it should be using
  // ExtensionRegistryObserver. See http://crbug.com/355029.
  UpdateActiveExtensionsInCrashReporter(registry_);

  const PermissionsData* permissions_data = extension->permissions_data();

  // If the extension has permission to load chrome://favicon/ resources we need
  // to make sure that the FaviconSource is registered with the
  // ChromeURLDataManager.
  if (permissions_data->HasHostPermission(GURL(chrome::kChromeUIFaviconURL))) {
    FaviconSource* favicon_source = new FaviconSource(profile_);
    content::URLDataSource::Add(profile_, favicon_source);
  }

  // Same for chrome://theme/ resources.
  if (permissions_data->HasHostPermission(GURL(chrome::kChromeUIThemeURL))) {
    ThemeSource* theme_source = new ThemeSource(profile_);
    content::URLDataSource::Add(profile_, theme_source);
  }

  // Same for chrome://thumb/ resources.
  if (permissions_data->HasHostPermission(
          GURL(chrome::kChromeUIThumbnailURL))) {
    ThumbnailSource* thumbnail_source = new ThumbnailSource(profile_, false);
    content::URLDataSource::Add(profile_, thumbnail_source);
  }
}

void ChromeExtensionRegistrarDelegate::PostDisableExtension(
    scoped_refptr<const Extension> extension) {
  // TODO(kalman): Convert ExtensionSpecialStoragePolicy to a
  // BrowserContextKeyedService and use ExtensionRegistryObserver.
  profile_->GetExtensionSpecialStoragePolicy()->RevokeRightsForExtension(
      extension.get());

#if defined(OS_CHROMEOS)
  // Revoke external file access for the extension from its file system context.
  // It is safe to access the extension's storage partition at this point. The
  // storage partition may get destroyed only after the extension gets unloaded.
  GURL site = util::GetSiteForExtensionId(extension->id(), profile_);
  storage::FileSystemContext* filesystem_context =
      content::BrowserContext::GetStoragePartitionForSite(profile_, site)
          ->GetFileSystemContext();
  if (filesystem_context && filesystem_context->external_backend()) {
    filesystem_context->external_backend()->RevokeAccessForExtension(
        extension->id());
  }
#endif

  // TODO(kalman): This is broken. The crash reporter is process-wide so doesn't
  // work properly multi-profile. Besides which, it should be using
  // ExtensionRegistryObserver::OnExtensionLoaded. See http://crbug.com/355029.
  UpdateActiveExtensionsInCrashReporter(registry_);
}

bool ChromeExtensionRegistrarDelegate::CanEnableExtension(
    const Extension* extension) {
  return !management_policy_->MustRemainDisabled(extension, nullptr, nullptr);
}

bool ChromeExtensionRegistrarDelegate::CanDisableExtension(
    const Extension* extension) {
  // Some extensions cannot be disabled by users:
  // - |extension| can be null if sync disables an extension that is not
  //   installed yet; allow disablement in this case.
  if (!extension)
    return true;

  // - Shared modules are just resources used by other extensions, and are not
  //   user-controlled.
  if (SharedModuleInfo::IsSharedModule(extension))
    return false;

  // - EXTERNAL_COMPONENT extensions are not generally modifiable by users, but
  //   can be uninstalled by the browser if the user sets extension-specific
  //   preferences.
  if (extension->location() == Manifest::EXTERNAL_COMPONENT)
    return true;

  return management_policy_->UserMayModifySettings(extension, nullptr);
}

bool ChromeExtensionRegistrarDelegate::ShouldBlockExtension(
    const ExtensionId& extension_id) {
  // Blocked extensions aren't marked as such in prefs, thus if
  // extensions are blocked then CanBlockExtension() must be called with an
  // Extension object.
  // If the extension is not loaded, assume it should be blocked.
  if (!extension_service_->AreExtensionsBlocked())
    return false;
  const Extension* extension = registry_->GetInstalledExtension(extension_id);
  return !extension || CanBlockExtension(extension, management_policy_);
}

}  // namespace extensions
