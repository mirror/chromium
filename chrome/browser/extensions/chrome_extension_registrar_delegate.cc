#include "chrome/browser/extensions/chrome_extension_registrar_delegate.h"

#include <memory>
#include <set>
#include <string>

#include "base/logging.h"
#include "base/metrics/histogram_macros.h"
#include "chrome/browser/extensions/extension_disabled_ui.h"
#include "chrome/browser/extensions/extension_service.h"
#include "chrome/browser/extensions/extension_special_storage_policy.h"
#include "chrome/browser/extensions/extension_sync_service.h"
#include "chrome/browser/extensions/extension_util.h"
#include "chrome/browser/extensions/permissions_updater.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/search/thumbnail_source.h"
#include "chrome/browser/ui/webui/favicon_source.h"
#include "chrome/browser/ui/webui/theme_source.h"
#include "chrome/common/crash_keys.h"
#include "chrome/common/url_constants.h"
#include "extensions/browser/extension_prefs.h"
#include "extensions/browser/extension_registry.h"
#include "extensions/browser/extension_registry.h"
#include "extensions/browser/extension_util.h"
#include "extensions/browser/extensions_browser_client.h"
#include "extensions/browser/management_policy.h"
#include "extensions/common/extension.h"
#include "extensions/common/manifest_handlers/shared_module_info.h"
#include "extensions/common/permissions/permission_message_provider.h"
#include "extensions/common/permissions/permissions_data.h"

#if BUILDFLAG(ENABLE_SUPERVISED_USERS)
#include "chrome/browser/supervised_user/supervised_user_service.h"
#include "chrome/browser/supervised_user/supervised_user_service_factory.h"
#endif

#if defined(OS_CHROMEOS)
#include "content/public/browser/storage_partition.h"
#endif

namespace extensions {

namespace {

// Helper that updates the active extension list used for crash reporting.
void UpdateActiveExtensionsInCrashReporter(ExtensionRegistry* registry) {
  std::set<std::string> extension_ids;
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
    ExtensionPrefs* extension_prefs,
    ExtensionRegistry* registry,
    ManagementPolicy* management_policy)
    : extension_service_(extension_service),
      profile_(profile),
      extension_prefs_(extension_prefs),
      registry_(registry),
      management_policy_(management_policy) {}

ChromeExtensionRegistrarDelegate::~ChromeExtensionRegistrarDelegate() = default;

int ChromeExtensionRegistrarDelegate::PreAddExtension(
    const Extension* extension,
    bool is_extension_loaded) {
  PermissionsUpdater(profile_).InitializePermissions(extension);

  // We keep track of all permissions the user has granted each extension.
  // This allows extensions to gracefully support backwards compatibility
  // by including unknown permissions in their manifests. When the user
  // installs the extension, only the recognized permissions are recorded.
  // When the unknown permissions become recognized (e.g., through browser
  // upgrade), we can prompt the user to accept these new permissions.
  // Extensions can also silently upgrade to less permissions, and then
  // silently upgrade to a version that adds these permissions back.
  //
  // For example, pretend that Chrome 10 includes a permission "omnibox"
  // for an API that adds suggestions to the omnibox. An extension can
  // maintain backwards compatibility while still having "omnibox" in the
  // manifest. If a user installs the extension on Chrome 9, the browser
  // will record the permissions it recognized, not including "omnibox."
  // When upgrading to Chrome 10, "omnibox" will be recognized and Chrome
  // will disable the extension and prompt the user to approve the increase
  // in privileges. The extension could then release a new version that
  // removes the "omnibox" permission. When the user upgrades, Chrome will
  // still remember that "omnibox" had been granted, so that if the
  // extension once again includes "omnibox" in an upgrade, the extension
  // can upgrade without requiring this user's approval.
  int disable_reasons = extension_prefs_->GetDisableReasons(extension->id());

  // Silently grant all active permissions to default apps and apps installed
  // in kiosk mode.
  bool auto_grant_permission =
      extension->was_installed_by_default() ||
      ExtensionsBrowserClient::Get()->IsRunningInForcedAppMode();
  if (auto_grant_permission)
    GrantPermissions(extension);

  bool is_privilege_increase = false;
  // We only need to compare the granted permissions to the current permissions
  // if the extension has not been auto-granted its permissions above and is
  // installed internally.
  if (extension->location() == Manifest::INTERNAL && !auto_grant_permission) {
    // Add all the recognized permissions if the granted permissions list
    // hasn't been initialized yet.
    std::unique_ptr<const PermissionSet> granted_permissions =
        extension_prefs_->GetGrantedPermissions(extension->id());
    CHECK(granted_permissions.get());

    // Here, we check if an extension's privileges have increased in a manner
    // that requires the user's approval. This could occur because the browser
    // upgraded and recognized additional privileges, or an extension upgrades
    // to a version that requires additional privileges.
    is_privilege_increase =
        PermissionMessageProvider::Get()->IsPrivilegeIncrease(
            *granted_permissions,
            extension->permissions_data()->active_permissions(),
            extension->GetType());

    // If there was no privilege increase, the extension might still have new
    // permissions (which either don't generate a warning message, or whose
    // warning messages are suppressed by existing permissions). Grant the new
    // permissions.
    if (!is_privilege_increase)
      GrantPermissions(extension);
  }

  bool previously_disabled =
      extension_prefs_->IsExtensionDisabled(extension->id());
  // TODO(treib): Is the |is_extension_loaded| check needed here?
  if (is_extension_loaded && previously_disabled) {
    // Legacy disabled extensions do not have a disable reason. Infer that it
    // was likely disabled by the user.
    if (disable_reasons == disable_reason::DISABLE_NONE)
      disable_reasons |= disable_reason::DISABLE_USER_ACTION;

    // Extensions that came to us disabled from sync need a similar inference,
    // except based on the new version's permissions.
    // TODO(treib,devlin): Since M48,
    // disable_reason::DISABLE_UNKNOWN_FROM_SYNC isn't used anymore;
    // this code is still here to migrate any existing old state. Remove it
    // after some grace period.
    if (disable_reasons &
        disable_reason::DEPRECATED_DISABLE_UNKNOWN_FROM_SYNC) {
      // Remove the disable_reason::DISABLE_UNKNOWN_FROM_SYNC
      // reason.
      disable_reasons &=
          ~disable_reason::DEPRECATED_DISABLE_UNKNOWN_FROM_SYNC;
      extension_prefs_->RemoveDisableReason(
          extension->id(),
          disable_reason::DEPRECATED_DISABLE_UNKNOWN_FROM_SYNC);
      // If there was no privilege increase, it was likely disabled by the user.
      if (!is_privilege_increase)
        disable_reasons |= disable_reason::DISABLE_USER_ACTION;
    }
  }

  // If the extension is disabled due to a permissions increase, but does in
  // fact have all permissions, remove that disable reason.
  // TODO(devlin): This was added to fix crbug.com/616474, but it's unclear
  // if this behavior should stay forever.
  if (disable_reasons &
      disable_reason::DISABLE_PERMISSIONS_INCREASE) {
    bool reset_permissions_increase = false;
    if (!is_privilege_increase) {
      reset_permissions_increase = true;
      disable_reasons &=
          ~disable_reason::DISABLE_PERMISSIONS_INCREASE;
      extension_prefs_->RemoveDisableReason(
          extension->id(),
          disable_reason::DISABLE_PERMISSIONS_INCREASE);
    }
    UMA_HISTOGRAM_BOOLEAN("Extensions.ResetPermissionsIncrease",
                          reset_permissions_increase);
  }

  // Extension has changed permissions significantly. Disable it. A
  // notification should be sent by the caller. If the extension is already
  // disabled because it was installed remotely, don't add another disable
  // reason.
  if (is_privilege_increase &&
      !(disable_reasons & disable_reason::DISABLE_REMOTE_INSTALL)) {
    disable_reasons |= disable_reason::DISABLE_PERMISSIONS_INCREASE;
    if (!extension_prefs_->DidExtensionEscalatePermissions(extension->id())) {
      ExtensionService::RecordPermissionMessagesHistogram(extension,
                                                          "AutoDisable");
    }

#if BUILDFLAG(ENABLE_SUPERVISED_USERS)
    // If a custodian-installed extension is disabled for a supervised user due
    // to a permissions increase, send a request to the custodian.
    if (util::IsExtensionSupervised(extension, profile_) &&
        !ExtensionSyncService::Get(profile_)->HasPendingReenable(
            extension->id(), *extension->version())) {
      SupervisedUserService* supervised_user_service =
          SupervisedUserServiceFactory::GetForProfile(profile_);
      supervised_user_service->AddExtensionUpdateRequest(extension->id(),
                                                         *extension->version());
    }
#endif
  }

  return disable_reasons;
}

void ChromeExtensionRegistrarDelegate::PostEnableExtension(
    scoped_refptr<const Extension> extension) {
  // TODO(kalman): Convert ExtensionSpecialStoragePolicy to a
  // BrowserContextKeyedService and use ExtensionRegistryObserver.
  profile_->GetExtensionSpecialStoragePolicy()->
      GrantRightsForExtension(extension.get(), profile_);

  // TODO(kalman): This is broken. The crash reporter is process-wide so doesn't
  // work properly multi-profile. Besides which, it should be using
  // ExtensionRegistryObserver. See http://crbug.com/355029.
  UpdateActiveExtensionsInCrashReporter(registry_);

  const PermissionsData* permissions_data =
      extension->permissions_data();

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
  GURL site =
      util::GetSiteForExtensionId(extension->id(), profile_);
  storage::FileSystemContext* filesystem_context =
      content::BrowserContext::GetStoragePartitionForSite(profile_, site)
          ->GetFileSystemContext();
  if (filesystem_context && filesystem_context->external_backend()) {
    filesystem_context->external_backend()->
        RevokeAccessForExtension(extension->id());
  }
#endif

  // TODO(kalman): This is broken. The crash reporter is process-wide so doesn't
  // work properly multi-profile. Besides which, it should be using
  // ExtensionRegistryObserver::OnExtensionLoaded. See http://crbug.com/355029.
  UpdateActiveExtensionsInCrashReporter(registry_);
}

void ChromeExtensionRegistrarDelegate::OnAddedExtensionDisabled(
    const Extension* extension) {
  // Show the extension disabled error if a permissions increase or a remote
  // installation is the reason it was disabled, and no other reasons exist.
  int reasons = extension_prefs_->GetDisableReasons(extension->id());
  const int kReasonMask =
      disable_reason::DISABLE_PERMISSIONS_INCREASE |
      disable_reason::DISABLE_REMOTE_INSTALL;
  if (reasons & kReasonMask && !(reasons & ~kReasonMask)) {
    AddExtensionDisabledError(
        extension_service_, extension,
        extension_prefs_->HasDisableReason(
            extension->id(),
            disable_reason::DISABLE_REMOTE_INSTALL));
  }
}

bool ChromeExtensionRegistrarDelegate::ShouldBlockExtension(
    const ExtensionId& extension_id) {
  // Blocked extensions aren't marked as such in prefs, thus if
  // extensions are blocked then CanBlockExtension() must be called with an
  // Extension object.
  // If the extension is not loaded, assume it should be blocked.
  const Extension* extension = registry_->GetInstalledExtension(extension_id);
  if (!extension_service_->AreExtensionsBlocked())
    return false;
  return !extension || CanBlockExtension(extension, management_policy_);
}

bool ChromeExtensionRegistrarDelegate::CanEnableExtension(
    const Extension* extension) {
  if (management_policy_->MustRemainDisabled(extension, nullptr, nullptr)) {
    UMA_HISTOGRAM_COUNTS_100("Extensions.EnableDeniedByPolicy", 1);
    return false;
  }
  return true;
}

bool ChromeExtensionRegistrarDelegate::CanDisableExtension(
    const Extension* extension) {
  // Some extensions cannot be disabled by users:
  // - |extension| can be null if sync disables an extension that is not
  //   installed yet; allow disablement in this case.
  // - Shared modules are just resources used by other extensions, and are not
  //   user-controlled.
  // - EXTERNAL_COMPONENT extensions are not generally modifiable by users, but
  //   can be uninstalled by the browser if the user sets extension-specific
  //   preferences.
  return SharedModuleInfo::IsSharedModule(extension) ||
         (!management_policy_->UserMayModifySettings(extension, nullptr) &&
          extension->location() != Manifest::EXTERNAL_COMPONENT);
}

void ChromeExtensionRegistrarDelegate::LoadExtensionForReload(
    const ExtensionId& extension_id,
    const base::FilePath& path) {
  extension_service_->LoadExtensionForReload(extension_id, path);
}

void ChromeExtensionRegistrarDelegate::GrantPermissions(const Extension* extension) {
  PermissionsUpdater(profile_).GrantActivePermissions(extension);
}

}  // namespace extensions
