// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EXTENSIONS_BROWSER_EXTENSION_SERVICE_INTERFACE_H_
#define EXTENSIONS_BROWSER_EXTENSION_SERVICE_INTERFACE_H_

#include <string>

#include "extensions/browser/install_flag.h"
#include "extensions/common/extension.h"
#include "extensions/features/features.h"

#if !BUILDFLAG(ENABLE_EXTENSIONS)
#error "Extensions must be enabled"
#endif

namespace extensions {

class CrxInstaller;
struct CRXFileInfo;
class Extension;
class PendingExtensionManager;
enum class UnloadedExtensionReason;

}  // namespace extensions

// This is an interface class to encapsulate the dependencies that
// various classes have on ExtensionService. This allows easy mocking.
class ExtensionServiceInterface
    : public base::SupportsWeakPtr<ExtensionServiceInterface> {
 public:
  virtual ~ExtensionServiceInterface() {}

  // Gets the object managing the set of pending extensions.
  virtual extensions::PendingExtensionManager* pending_extension_manager() = 0;

  // Installs an update with the contents from |extension_path|. Returns true if
  // the install can be started. Sets |out_crx_installer| to the installer if
  // one was started.
  // TODO(aa): This method can be removed. ExtensionUpdater could use
  // CrxInstaller directly instead.
  virtual bool UpdateExtension(
      const extensions::CRXFileInfo& file,
      bool file_ownership_passed,
      extensions::CrxInstaller** out_crx_installer) = 0;

  // DEPRECATED. Use ExtensionRegistry instead.
  //
  // Looks up an extension by its ID.
  //
  // If |include_disabled| is false then this will only include enabled
  // extensions. Use instead:
  //
  //   ExtensionRegistry::enabled_extensions().GetByID(id).
  //
  // If |include_disabled| is true then this will also include disabled and
  // blacklisted extensions (not terminated extensions). Use instead:
  //
  //   ExtensionRegistry::GetExtensionById(
  //         id, ExtensionRegistry::ENABLED |
  //             ExtensionRegistry::DISABLED |
  //             ExtensionRegistry::BLACKLISTED)
  //
  // Or don't, because it's probably not something you ever need to know.
  virtual const extensions::Extension* GetExtensionById(
      const std::string& id,
      bool include_disabled) const = 0;

  // DEPRECATED: Use ExtensionRegistry instead.
  //
  // Looks up an extension by ID, regardless of whether it's enabled,
  // disabled, blacklisted, or terminated. Use instead:
  //
  // ExtensionRegistry::GetInstalledExtension(id).
  virtual const extensions::Extension* GetInstalledExtension(
      const std::string& id) const = 0;

  // Returns an update for an extension with the specified id, if installation
  // of that update was previously delayed because the extension was in use. If
  // no updates are pending for the extension returns NULL.
  virtual const extensions::Extension* GetPendingExtensionUpdate(
      const std::string& extension_id) const = 0;

  // Finishes installation of an update for an extension with the specified id,
  // when installation of that extension was previously delayed because the
  // extension was in use.
  virtual void FinishDelayedInstallation(const std::string& extension_id) = 0;

  // Returns true if the extension with the given |extension_id| is enabled.
  // This will only return a valid answer for installed extensions (regardless
  // of whether it is currently loaded or not).  Loaded extensions return true
  // if they are currently loaded or terminated.  Unloaded extensions will
  // return true if they are not blocked, disabled, blacklisted or uninstalled
  // (for external extensions).
  virtual bool IsExtensionEnabled(const std::string& extension_id) const = 0;

  // Go through each extension and unload those that are not allowed to run by
  // management policy providers (ie. network admin and Google-managed
  // blacklist).
  virtual void CheckManagementPolicy() = 0;

  // Safe to call multiple times in a row.
  //
  // TODO(akalin): Remove this method (and others) once we refactor
  // themes sync to not use it directly.
  virtual void CheckForUpdatesSoon() = 0;

  // Adds |extension| to this ExtensionService and notifies observers that the
  // extension has been loaded.
  virtual void AddExtension(const extensions::Extension* extension) = 0;

  // Check if we have preferences for the component extension and, if not or if
  // the stored version differs, install the extension (without requirements
  // checking) before calling AddExtension.
  virtual void AddComponentExtension(
      const extensions::Extension* extension) = 0;

  // Unload the specified extension.
  virtual void UnloadExtension(const std::string& extension_id,
                               extensions::UnloadedExtensionReason reason) = 0;

  // Remove the specified component extension.
  virtual void RemoveComponentExtension(const std::string& extension_id) = 0;

  // Whether the extension service is ready.
  virtual bool is_ready() = 0;
};

#endif  // EXTENSIONS_BROWSER_EXTENSION_SERVICE_INTERFACE_H_
