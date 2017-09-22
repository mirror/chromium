// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/browser/extension_registrar.h"

#include "base/logging.h"
#include "extensions/browser/app_sorting.h"
#include "base/strings/string_util.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/devtools_agent_host.h"
#include "content/public/browser/notification_service.h"
#include "extensions/browser/extension_prefs.h"
#include "extensions/browser/lazy_background_task_queue.h"
#include "extensions/browser/extension_registry.h"
#include "extensions/browser/extension_system.h"
#include "extensions/browser/extension_host.h"
#include "extensions/browser/notification_types.h"
#include "extensions/browser/process_manager.h"
#include "extensions/browser/renderer_startup_helper.h"
#include "extensions/browser/runtime_data.h"
#include "extensions/common/manifest_handlers/background_info.h"

namespace extensions {

namespace {

void DoNothingWithExtensionHost(ExtensionHost* host) {}

}

ExtensionRegistrar::ExtensionRegistrar(
    content::BrowserContext* browser_context,
    ExtensionPrefs* extension_prefs,
    std::unique_ptr<ExtensionRegistrar::Delegate> delegate)
    : browser_context_(browser_context),
      extension_prefs_(extension_prefs),
      extension_system_(ExtensionSystem::Get(browser_context)),
      registry_(ExtensionRegistry::Get(browser_context)),
      renderer_helper_(
          RendererStartupHelperFactory::GetForBrowserContext(browser_context)),
      delegate_(std::move(delegate)),
      weak_factory_(this) {}

ExtensionRegistrar::~ExtensionRegistrar() = default;

void ExtensionRegistrar::AddExtension(const Extension* extension) {
  bool is_extension_loaded = false;
  bool is_extension_upgrade = false;

  if (registry_->GetInstalledExtension(extension->id())) {
    const Extension* old = registry_->GetInstalledExtension(extension->id());
    is_extension_loaded = true;
    int version_compare_result =
        extension->version()->CompareTo(*(old->version()));
    is_extension_upgrade = version_compare_result > 0;
    // Other than for unpacked extensions, CrxInstaller should have guaranteed
    // that we aren't downgrading.
    if (!Manifest::IsUnpackedLocation(extension->location()))
      CHECK_GE(version_compare_result, 0);
  }

  // If the extension was disabled for a reload, we will enable it.
  bool reloading = reloading_extensions_.erase(extension->id()) > 0;

  // Set the upgraded bit; we consider reloads upgrades.
  extension_system_->runtime_data()->SetBeingUpgraded(
      extension->id(), is_extension_upgrade || reloading);

  // If a terminated extension is loaded, remove it from the terminated list.
  UntrackTerminatedExtension(extension->id());

  int disable_reasons =
      delegate_->PreAddExtension(extension, is_extension_loaded);
  if (disable_reasons == disable_reason::DISABLE_NONE)
    extension_prefs_->SetExtensionEnabled(extension->id());
  else
    extension_prefs_->SetExtensionDisabled(extension->id(), disable_reasons);

  if (is_extension_loaded && !reloading) {
    // To upgrade an extension in place, remove the old one and then activate
    // the new one. ReloadExtension disables the extension, which is sufficient.
    DeactivateAndRemoveExtension(extension->id(),
                                 UnloadedExtensionReason::UPDATE);
  }

  // The extension is now loaded, remove its data from unloaded extension map.
  unloaded_extension_paths_.erase(extension->id());

  if (extension_prefs_->IsExtensionBlacklisted(extension->id())) {
    // Only prefs is checked for the blacklist. We rely on callers to check the
    // blacklist before calling into here, e.g. CrxInstaller checks before
    // installation then threads through the install and pending install flow
    // of this class, and ExtensionService checks when loading installed
    // extensions.
    registry_->AddBlacklisted(extension);
  } else if (delegate_->ShouldBlockExtension(extension->id())) {
    registry_->AddBlocked(extension);
  } else if (!reloading &&
             extension_prefs_->IsExtensionDisabled(extension->id())) {
    registry_->AddDisabled(extension);
    content::NotificationService::current()->Notify(
        extensions::NOTIFICATION_EXTENSION_UPDATE_DISABLED,
        content::Source<content::BrowserContext>(browser_context_),
        content::Details<const Extension>(extension));
    delegate_->OnAddedExtensionDisabled(extension);
  } else if (reloading) {
    // Replace the old extension with the new version.
    CHECK(!registry_->AddDisabled(extension));
    // TODO
    EnableExtension(extension->id());
  } else {
    // All apps that are displayed in the launcher are ordered by their ordinals
    // so we must ensure they have valid ordinals.
    if (extension->RequiresSortOrdinal()) {
      AppSorting* app_sorting = extension_system_->app_sorting();
      app_sorting->SetExtensionVisible(
          extension->id(),
          extension->ShouldDisplayInNewTabPage());
      app_sorting->EnsureValidOrdinals(extension->id(),
                                       syncer::StringOrdinal());
    }

    registry_->AddEnabled(extension);
    // TODO: for some reason I had EnableExtension here but that doesn't make
    // sense. Verify that ActivateExtension does everything we need to do...
    // and call EnableExtension "EnableDisabledExtension" or something
    ActivateExtension(extension);
  }

  extension_system_->runtime_data()->SetBeingUpgraded(extension->id(), false);
}

bool ExtensionRegistrar::CanReloadExtension(const ExtensionId& extension_id) {
  // If the extension is already reloading, don't reload again.
  if (extension_prefs_->GetDisableReasons(extension_id) &
      extensions::disable_reason::DISABLE_RELOAD) {
    return false;
  }

  // Ignore attempts to reload a blacklisted or blocked extension. Sometimes
  // this can happen in a convoluted reload sequence triggered by the
  // termination of a blacklisted or blocked extension and a naive attempt to
  // reload it. For an example see http://crbug.com/373842.
  if (registry_->blacklisted_extensions().Contains(extension_id) ||
      registry_->blocked_extensions().Contains(extension_id)) {
    return false;
  }
  return true;
}

void ExtensionRegistrar::ReloadExtension(
    // "transient" because the process of reloading may cause the reference
    // to become invalid. Instead, use |extension_id|, a copy.
    const ExtensionId& transient_extension_id) {
  base::FilePath path;

  ExtensionId extension_id = transient_extension_id;
  const Extension* transient_current_extension = registry_->GetExtensionById(
      transient_extension_id, ExtensionRegistry::ENABLED);

  // Disable the extension if it's loaded. It might not be loaded if it crashed.
  if (transient_current_extension) {
    // If the extension has an inspector open for its background page, detach
    // the inspector and hang onto a cookie for it, so that we can reattach
    // later.
    // TODO(yoz): this is not incognito-safe!
    ProcessManager* manager =
        ProcessManager::Get(browser_context_);
    ExtensionHost* host =
        manager->GetBackgroundHostForExtension(extension_id);
    if (host && content::DevToolsAgentHost::HasFor(host->host_contents())) {
      // Look for an open inspector for the background page.
      scoped_refptr<content::DevToolsAgentHost> agent_host =
          content::DevToolsAgentHost::GetOrCreateFor(host->host_contents());
      agent_host->DisconnectWebContents();
      orphaned_dev_tools_[extension_id] = agent_host;
    }

    path = transient_current_extension->path();
    // BeingUpgraded is set back to false when the extension is added.
    extension_system_->runtime_data()->SetBeingUpgraded(
        transient_current_extension->id(), true);
    DisableExtension(extension_id, disable_reason::DISABLE_RELOAD);
    DCHECK(registry_->disabled_extensions().Contains(extension_id));
    reloading_extensions_.insert(extension_id);
  } else {
    // Check the unloaded extensions.
    std::map<std::string, base::FilePath>::const_iterator iter =
        unloaded_extension_paths_.find(extension_id);
    if (iter != unloaded_extension_paths_.end()) {
      path = unloaded_extension_paths_[extension_id];
    }
  }

  // After unloading the extension or finding the unloaded path, load the new
  // version.
  delegate_->LoadExtensionForReload(extension_id, path);
}

void ExtensionRegistrar::DisableExtension(const ExtensionId& extension_id,
                                          int disable_reasons) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  if (extension_prefs_->IsExtensionBlacklisted(extension_id))
    return;

  // The extension may have been disabled already. Just add the disable reasons.
  if (!IsExtensionEnabled(extension_id)) {
    extension_prefs_->AddDisableReasons(extension_id, disable_reasons);
    return;
  }

  const Extension* extension = 
      registry_->GetExtensionById(extension_id, ExtensionRegistry::EVERYTHING);

  bool is_controlled_extension =
      extension && !delegate_->CanDisableExtension(extension);

  // Certain disable reasons are always allowed, since they are more internal to
  // the browser (rather than the user choosing to disable the extension).
  int internal_disable_reason_mask =
      extensions::disable_reason::DISABLE_RELOAD |
      extensions::disable_reason::DISABLE_CORRUPTED |
      extensions::disable_reason::DISABLE_UPDATE_REQUIRED_BY_POLICY |
      extensions::disable_reason::DISABLE_BLOCKED_BY_POLICY;
  bool is_internal_disable =
      (disable_reasons & internal_disable_reason_mask) > 0;

  if (!is_internal_disable && is_controlled_extension)
    return;

  extension_prefs_->SetExtensionDisabled(extension_id, disable_reasons);

  int include_mask =
      ExtensionRegistry::EVERYTHING & ~ExtensionRegistry::DISABLED;
  extension = registry_->GetExtensionById(extension_id, include_mask);
  if (!extension)
    return;

  // The extension is either enabled or terminated.
  DCHECK(registry_->enabled_extensions().Contains(extension->id()) ||
         registry_->terminated_extensions().Contains(extension->id()));

  if (registry_->enabled_extensions().Contains(extension->id())) {
    registry_->AddDisabled(extension);
    DeactivateExtensionImpl(extension, UnloadedExtensionReason::DISABLE, true);
    delegate_->PostDisableExtension(extension);
  } else {
    // Don't send a second unload notification for terminated extensions being
    // disabled.
    registry_->RemoveTerminated(extension->id());
  }
}

void ExtensionRegistrar::UntrackTerminatedExtension(const ExtensionId& extension_id) {
  std::string lowercase_id = base::ToLowerASCII(extension_id);
  const Extension* extension =
      registry_->terminated_extensions().GetByID(lowercase_id);
  registry_->RemoveTerminated(lowercase_id);
  if (extension) {
    content::NotificationService::current()->Notify(
        extensions::NOTIFICATION_EXTENSION_REMOVED,
        content::Source<content::BrowserContext>(browser_context_),
        content::Details<const Extension>(extension));
  }
}

// TODO
bool ExtensionRegistrar::IsExtensionEnabled(
    const ExtensionId& extension_id) const {
  if (registry_->enabled_extensions().Contains(extension_id) ||
      registry_->terminated_extensions().Contains(extension_id)) {
    return true;
  }

  if (registry_->disabled_extensions().Contains(extension_id) ||
      registry_->blacklisted_extensions().Contains(extension_id) ||
      registry_->blocked_extensions().Contains(extension_id)) {
    return false;
  }

  if (delegate_->ShouldBlockExtension(extension_id))
    return false;

  // The extension hasn't been loaded yet, so check the prefs for it. Assume
  // enabled unless otherwise noted.
  return !extension_prefs_->IsExtensionDisabled(extension_id) &&
         !extension_prefs_->IsExtensionBlacklisted(extension_id) &&
         !extension_prefs_->IsExternalExtensionUninstalled(extension_id);
}

void ExtensionRegistrar::EnableExtension(
    const ExtensionId& extension_id) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  // If the extension is currently reloading, it will be enabled once the reload
  // is complete.
  if (reloading_extensions_.count(extension_id) > 0)
    return;

  if (IsExtensionEnabled(extension_id) ||
      extension_prefs_->IsExtensionBlacklisted(extension_id))
    return;

  const Extension* extension =
      registry_->disabled_extensions().GetByID(extension_id);

  if (extension && !delegate_->CanEnableExtension(extension))
    return;

  extension_prefs_->SetExtensionEnabled(extension_id);

  // This can happen if sync enables an extension that is not installed yet.
  if (!extension)
    return;

  // Move it over to the enabled list.
  registry_->AddEnabled(make_scoped_refptr(extension));
  ActivateExtension(extension);
}

void ExtensionRegistrar::ActivateExtension(const Extension* extension) {
  registry_->RemoveDisabled(extension->id());

  // The URLRequestContexts need to be first to know that the extension
  // was loaded. Otherwise a race can arise where a renderer that is created
  // for the extension may try to load an extension URL with an extension id
  // that the request context doesn't yet know about. The BrowserContext should
  // ensure its URLRequestContexts appropriately discover the loaded extension.
  extension_system_->RegisterExtensionWithRequestContexts(
      extension,
      base::Bind(&ExtensionRegistrar::OnExtensionRegisteredWithRequestContexts,
                 weak_factory_.GetWeakPtr(), make_scoped_refptr(extension)));

  renderer_helper_->OnExtensionLoaded(*extension);

  // Tell subsystems that use the ExtensionRegistryObserver::OnExtensionLoaded
  // about the new extension.
  //
  // NOTE: It is important that this happen after notifying the renderers about
  // the new extensions so that if we navigate to an extension URL in
  // ExtensionRegistryObserver::OnExtensionLoaded the renderer is guaranteed to
  // know about it.
  registry_->TriggerOnLoaded(extension);

  delegate_->PostEnableExtension(extension);

  MaybeSpinUpLazyBackgroundPage(extension);
}

void ExtensionRegistrar::MaybeSpinUpLazyBackgroundPage(
    const Extension* extension) {
  if (!BackgroundInfo::HasLazyBackgroundPage(extension))
    return;

  // For orphaned devtools, we will reconnect devtools to it later in
  // DidCreateRenderViewForBackgroundPage().
  OrphanedDevTools::iterator iter = orphaned_dev_tools_.find(extension->id());
  bool has_orphaned_dev_tools = iter != orphaned_dev_tools_.end();

  // Reloading component extension does not trigger install, so RuntimeAPI won't
  // be able to detect its loading. Therefore, we need to spin up its lazy
  // background page.
  bool is_component_extension =
      Manifest::IsComponentLocation(extension->location());

  if (!has_orphaned_dev_tools && !is_component_extension)
    return;

  // Wake up the event page by posting a dummy task.
  LazyBackgroundTaskQueue* queue =
      LazyBackgroundTaskQueue::Get(browser_context_);
  queue->AddPendingTask(browser_context_, extension->id(),
                        base::Bind(&DoNothingWithExtensionHost));
}

void ExtensionRegistrar::DeactivateAndRemoveExtension(
    const ExtensionId& extension_id,
    UnloadedExtensionReason reason) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  int include_mask =
      ExtensionRegistry::EVERYTHING & ~ExtensionRegistry::TERMINATED;
  scoped_refptr<const Extension> extension(
      registry_->GetExtensionById(extension_id, include_mask));

  // This method can be called via PostTask, so the extension may have been
  // unloaded by the time this runs.
  if (!extension.get()) {
    // The extension may have crashed/uninstalled. Allow the profile to clean
    // up its RequestContexts.
    extension_system_->UnregisterExtensionWithRequestContexts(extension_id,
                                                              reason);
    return;
  }

  // Keep information about the extension so that we can reload it later
  // even if it's not permanently installed.
  unloaded_extension_paths_[extension->id()] = extension->path();

  // Clean up if the extension is meant to be enabled after a reload.
  reloading_extensions_.erase(extension->id());

  bool was_enabled =
      !registry_->disabled_extensions().Contains(extension->id());
  DeactivateExtensionImpl(extension, reason, was_enabled);

  content::NotificationService::current()->Notify(
      extensions::NOTIFICATION_EXTENSION_REMOVED,
      content::Source<content::BrowserContext>(browser_context_),
      content::Details<const Extension>(extension.get()));

  if (was_enabled)
    delegate_->PostDisableExtension(extension);
}

void ExtensionRegistrar::DeactivateExtensionImpl(
    scoped_refptr<const Extension> extension,
    UnloadedExtensionReason reason,
    bool was_enabled) {
  if (was_enabled) {
    // Remove the extension from the enabled list.
    registry_->RemoveEnabled(extension->id());

    // Trigger the unloaded notifications.
    registry_->TriggerOnUnloaded(extension.get(), reason);
    renderer_helper_->OnExtensionUnloaded(*extension);
  } else {
    registry_->RemoveDisabled(extension->id());
    // Don't send the unloaded notification. It was sent when the extension
    // was disabled.
  }

  // Make sure the BrowserContext cleans up its RequestContexts even when an
  // already disabled extension is unloaded (since they are also tracking the
  // disabled extensions).
  extension_system_->UnregisterExtensionWithRequestContexts(extension->id(),
                                                            reason);
}

void ExtensionRegistrar::OnExtensionRegisteredWithRequestContexts(
    scoped_refptr<const Extension> extension) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  registry_->AddReady(extension);
  if (registry_->enabled_extensions().Contains(extension->id()))
    registry_->TriggerOnReady(extension.get());
}

}  // namespace extensions
