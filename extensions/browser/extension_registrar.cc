// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/browser/extension_registrar.h"

#include "base/logging.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/notification_service.h"
#include "extensions/browser/extension_registry.h"
#include "extensions/browser/extension_system.h"
#include "extensions/browser/notification_types.h"
#include "extensions/browser/renderer_startup_helper.h"

namespace extensions {

ExtensionRegistrar::ExtensionRegistrar(content::BrowserContext* browser_context)
    : browser_context_(browser_context),
      extension_system_(ExtensionSystem::Get(browser_context)),
      registry_(ExtensionRegistry::Get(browser_context)),
      renderer_helper_(
          RendererStartupHelperFactory::GetForBrowserContext(browser_context)),
      weak_factory_(this) {}

ExtensionRegistrar::~ExtensionRegistrar() = default;

void ExtensionRegistrar::ActivateExtension(
    scoped_refptr<const Extension> extension) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  DCHECK(registry_);
  registry_->AddEnabled(extension);

  // The URLRequestContexts need to be first to know that the extension
  // was loaded. Otherwise a race can arise where a renderer that is created
  // for the extension may try to load an extension URL with an extension id
  // that the request context doesn't yet know about. The BrowserContext should
  // ensure its URLRequestContexts appropriately discover the loaded extension.
  extension_system_->RegisterExtensionWithRequestContexts(
      extension.get(),
      base::Bind(&ExtensionRegistrar::OnExtensionRegisteredWithRequestContexts,
                 weak_factory_.GetWeakPtr(), extension));

  renderer_helper_->OnExtensionLoaded(*extension);

  // Tell subsystems that use the ExtensionRegistryObserver::OnExtensionLoaded
  // about the new extension.
  //
  // NOTE: It is important that this happen after notifying the renderers about
  // the new extensions so that if we navigate to an extension URL in
  // ExtensionRegistryObserver::OnExtensionLoaded the renderer is guaranteed to
  // know about it.
  registry_->TriggerOnLoaded(extension.get());
}

void ExtensionRegistrar::DeactivateExtension(
    scoped_refptr<const Extension> extension) {
  registry_->AddDisabled(extension);
  DeactivateExtensionImpl(extension, UnloadedExtensionReason::DISABLE, true);
}

void ExtensionRegistrar::RemoveExtension(const ExtensionId& extension_id,
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

  bool was_enabled =
      !registry_->disabled_extensions().Contains(extension->id());
  DeactivateExtensionImpl(extension, reason, was_enabled);

  content::NotificationService::current()->Notify(
      extensions::NOTIFICATION_EXTENSION_REMOVED,
      content::Source<content::BrowserContext>(browser_context_),
      content::Details<const Extension>(extension.get()));
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
