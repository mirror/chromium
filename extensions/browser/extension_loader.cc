// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/browser/extension_loader.h"

#include <string>

#include "base/command_line.h"
#include "base/files/file_path.h"
#include "base/logging.h"
#include "base/threading/sequenced_task_runner_handle.h"
#include "base/threading/thread_restrictions.h"
#include "content/public/browser/browser_thread.h"
#include "extensions/browser/extension_registry.h"
#include "extensions/browser/extension_system.h"
#include "extensions/browser/renderer_startup_helper.h"
#include "extensions/common/file_util.h"

namespace extensions {

namespace {

// Loads the extension, logging errors or warnings.
scoped_refptr<const Extension> LoadExtensionFromCommandLineImpl(
    const base::FilePath& extension_dir) {
  // Unpacked extensions may have symlinks.
  int load_flags = Extension::FOLLOW_SYMLINKS_ANYWHERE;
  std::string load_error;
  scoped_refptr<Extension> extension = file_util::LoadExtension(
      extension_dir, Manifest::COMMAND_LINE, load_flags, &load_error);
  if (!extension.get()) {
    LOG(ERROR) << "Loading extension at " << extension_dir.value()
               << " failed with: " << load_error;
  } else if (!extension->install_warnings().empty()) {
    LOG(WARNING) << "Warnings loading extension at " << extension_dir.value()
                 << ":";
    for (const auto& warning : extension->install_warnings())
      LOG(WARNING) << warning.message;
  }

  return extension;
}

}  // namespace

ExtensionLoader::ExtensionLoader(content::BrowserContext* browser_context)
    : extension_system_(ExtensionSystem::Get(browser_context)),
      registry_(ExtensionRegistry::Get(browser_context)),
      renderer_helper_(
          RendererStartupHelperFactory::GetForBrowserContext(browser_context)),
      weak_factory_(this) {}

ExtensionLoader::~ExtensionLoader() = default;

scoped_refptr<const Extension> ExtensionLoader::LoadExtensionFromCommandLine(
    const base::FilePath& extension_dir) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  scoped_refptr<const Extension> extension =
      LoadExtensionFromCommandLineImpl(extension_dir);
  if (!extension)
    return nullptr;

  registry_->AddEnabled(extension.get());

  base::SequencedTaskRunnerHandle::Get()->PostTask(
      FROM_HERE, base::BindOnce(&ExtensionLoader::NotifyExtensionLoaded,
                                weak_factory_.GetWeakPtr(), extension));
  return extension;
}

// Notifies on the UI thread.
void ExtensionLoader::NotifyExtensionLoaded(
    scoped_refptr<const Extension> extension) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

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
