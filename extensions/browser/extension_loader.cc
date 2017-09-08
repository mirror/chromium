// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/browser/extension_loader.h"

#include "base/logging.h"
#include "content/public/browser/browser_thread.h"
#include "extensions/browser/extension_registry.h"
#include "extensions/browser/extension_system.h"
#include "extensions/browser/renderer_startup_helper.h"

namespace extensions {

ExtensionLoader::ExtensionLoader(content::BrowserContext* browser_context)
    : extension_system_(ExtensionSystem::Get(browser_context)),
      registry_(ExtensionRegistry::Get(browser_context)),
      renderer_helper_(
          RendererStartupHelperFactory::GetForBrowserContext(browser_context)),
      weak_factory_(this) {}

ExtensionLoader::~ExtensionLoader() = default;

void ExtensionLoader::NotifyExtensionLoaded(
    scoped_refptr<const Extension> extension) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  registry_->AddEnabled(extension);

  // The URLRequestContexts need to be first to know that the extension
  // was loaded. Otherwise a race can arise where a renderer that is created
  // for the extension may try to load an extension URL with an extension id
  // that the request context doesn't yet know about. The BrowserContext should
  // ensure its URLRequestContexts appropriately discover the loaded extension.
  extension_system_->RegisterExtensionWithRequestContexts(
      extension.get(),
      base::Bind(&ExtensionLoader::OnExtensionRegisteredWithRequestContexts,
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

void ExtensionLoader::NotifyExtensionUnloaded(
    scoped_refptr<const Extension> extension,
    UnloadedExtensionReason reason) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  registry_->RemoveEnabled(extension->id());
  registry_->TriggerOnUnloaded(extension.get(), reason);
  renderer_helper_->OnExtensionUnloaded(*extension);
  extension_system_->UnregisterExtensionWithRequestContexts(extension->id(),
                                                            reason);
}

void ExtensionLoader::OnExtensionRegisteredWithRequestContexts(
    scoped_refptr<const Extension> extension) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  registry_->AddReady(extension);
  if (registry_->enabled_extensions().Contains(extension->id()))
    registry_->TriggerOnReady(extension.get());
}

}  // namespace extensions
