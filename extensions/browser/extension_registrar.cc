// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/browser/extension_registrar.h"

#include <utility>

#include "base/logging.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/notification_service.h"
#include "extensions/browser/extension_host.h"
#include "extensions/browser/extension_prefs.h"
#include "extensions/browser/extension_registry.h"
#include "extensions/browser/extension_system.h"
#include "extensions/browser/notification_types.h"
#include "extensions/browser/renderer_startup_helper.h"

namespace extensions {

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

void ExtensionRegistrar::AddExtension(scoped_refptr<const Extension> extension,
                                      InitialState initial_state) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  switch (initial_state) {
    case InitialState::ENABLED:
      registry_->AddEnabled(extension);
      ActivateExtension(extension.get());
      break;
    case InitialState::DISABLED:
      registry_->AddDisabled(extension);
      // Notify that a disabled extension was added or updated.
      content::NotificationService::current()->Notify(
          extensions::NOTIFICATION_EXTENSION_UPDATE_DISABLED,
          content::Source<content::BrowserContext>(browser_context_),
          content::Details<const Extension>(extension.get()));
      break;
    case InitialState::BLACKLISTED:
      registry_->AddBlacklisted(extension);
      break;
    case InitialState::BLOCKED:
      registry_->AddBlocked(extension);
      break;
  }
}

void ExtensionRegistrar::RemoveExtension(const ExtensionId& extension_id,
                                         UnloadedExtensionReason reason) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  int include_mask =
      ExtensionRegistry::EVERYTHING & ~ExtensionRegistry::TERMINATED;
  scoped_refptr<const Extension> extension(
      registry_->GetExtensionById(extension_id, include_mask));

  if (!extension.get()) {
    // In case the extension crashed or has been uninstalled, ensure the
    // RequestContexts are cleaned up.
    extension_system_->UnregisterExtensionWithRequestContexts(extension_id,
                                                              reason);
    return;
  }

  bool already_disabled =
      registry_->disabled_extensions().Contains(extension_id);
  UnregisterExtension(extension, reason);
  if (!already_disabled)
    delegate_->PostDisableExtension(extension);

  content::NotificationService::current()->Notify(
      extensions::NOTIFICATION_EXTENSION_REMOVED,
      content::Source<content::BrowserContext>(browser_context_),
      content::Details<const Extension>(extension.get()));
}

const Extension* ExtensionRegistrar::EnableExtension(
    const ExtensionId& extension_id) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  // First, check that the extension can be enabled.
  if (registry_->enabled_extensions().Contains(extension_id) ||
      registry_->terminated_extensions().Contains(extension_id)) {
    // The extension was already enabled.
    return nullptr;
  }
  if (extension_prefs_->IsExtensionBlacklisted(extension_id))
    return nullptr;

  const Extension* extension =
      registry_->disabled_extensions().GetByID(extension_id);
  if (extension && !delegate_->CanEnableExtension(extension))
    return nullptr;

  // Now that we know the extension can be enabled, update the prefs.
  extension_prefs_->SetExtensionEnabled(extension_id);

  // This can happen if sync enables an extension that is not installed yet.
  if (!extension)
    return nullptr;

  // Actually enable the extension.
  registry_->AddEnabled(make_scoped_refptr(extension));
  registry_->RemoveDisabled(extension->id());
  ActivateExtension(extension);
  return extension;
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

  scoped_refptr<const Extension> extension =
      registry_->GetExtensionById(extension_id, ExtensionRegistry::EVERYTHING);

  bool is_controlled_extension =
      !delegate_->CanDisableExtension(extension.get());

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

  registry_->AddDisabled(extension);
  if (registry_->enabled_extensions().Contains(extension->id())) {
    UnregisterExtension(extension, UnloadedExtensionReason::DISABLE);
    delegate_->PostDisableExtension(extension);
  } else {
    // The extension must have been terminated. Don't send additional
    // notifications for it being disabled.
    bool removed = registry_->RemoveTerminated(extension->id());
    DCHECK(removed);
  }
}

bool ExtensionRegistrar::ReplaceReloadedExtension(
    scoped_refptr<const Extension> extension) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  // The extension must already be disabled, and the original extension has
  // been unloaded.
  CHECK(registry_->disabled_extensions().Contains(extension->id()));
  if (!delegate_->CanEnableExtension(extension.get()))
    return false;

  extension_prefs_->SetExtensionEnabled(extension->id());

  // Move it over to the enabled list.
  CHECK(registry_->AddEnabled(extension));
  CHECK(registry_->RemoveDisabled(extension->id()));

  ActivateExtension(extension.get());

  return true;
}

void ExtensionRegistrar::ActivateExtension(const Extension* extension) {
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
}

void ExtensionRegistrar::UnregisterExtension(
    scoped_refptr<const Extension> extension,
    UnloadedExtensionReason reason) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  if (registry_->enabled_extensions().Contains(extension->id())) {
    // Remove the extension from the enabled list.
    registry_->RemoveEnabled(extension->id());

    // Trigger the unloaded notifications.
    registry_->TriggerOnUnloaded(extension.get(), reason);
    renderer_helper_->OnExtensionUnloaded(*extension);
  } else {
    // Remove from the registry. No need to trigger unloaded notifications;
    // those should have been sent when the extension became disabled.
    registry_->RemoveDisabled(extension->id());
  }

  // Make sure the BrowserContext cleans up its RequestContexts regardless of
  // whether the extension was already disabled (since they are also tracking
  // the disabled extensions).
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

  // If the extension hasn't been loaded yet, check the prefs for it. Assume
  // enabled unless otherwise noted.
  return !extension_prefs_->IsExtensionDisabled(extension_id) &&
         !extension_prefs_->IsExtensionBlacklisted(extension_id) &&
         !extension_prefs_->IsExternalExtensionUninstalled(extension_id);
}

}  // namespace extensions
