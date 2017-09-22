// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EXTENSIONS_BROWSER_EXTENSION_REGISTRAR_H_
#define EXTENSIONS_BROWSER_EXTENSION_REGISTRAR_H_

#include <map>
#include <memory>
#include <string>

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "extensions/common/extension.h"

namespace base {
class FliePath;
}  // namespace base

namespace content {
class BrowserContext;
class DevToolsAgentHost;
}  // namespace content

namespace extensions {

class Extension;
class ExtensionPrefs;
class ExtensionRegistry;
class ExtensionSystem;
class RendererStartupHelper;

// ExtensionRegistrar drives the stages of registering and unregistering
// extensions for a BrowserContext.
// TODO(michaelpg): Add functionality for reloading cand terminating extensions.
class ExtensionRegistrar {
 public:
  // Delegate for embedder-specific functionality like user-granted permisions.
  class Delegate {
   public:
    Delegate() = default;
    virtual ~Delegate() = default;

    // Lets the delegate set up the extension before it's added and optionally
    // force the extension to be disabled.  E.g., if an updated extension
    // requests new permissions, Chrome could grant the permissions or return
    // disable_reasons.
    virtual int PreAddExtension(const Extension* extension,
                                bool is_extension_loaded) = 0;

    // Handles updating the browser context when an extension is activated
    // (becomes enabled).
    virtual void PostEnableExtension(
        scoped_refptr<const Extension> extension) = 0;

    // Handles updating the browser context when an enabled extension is
    // deactivated (whether disabled or removed).
    // TODO(michaelpg): The implementation of this and the above function could
    // be replaced in Chrome by observing ExtensionRegistryObserver::OnLoaded,
    // but the timing would not be equivalent for PostDisableExtension. Safe
    // refactoring requires more investigation.
    virtual void PostDisableExtension(
        scoped_refptr<const Extension> extension) = 0;

    // Called when an extension is added in the disabled state.
    virtual void OnAddedExtensionDisabled(const Extension* extension) = 0;

    // Returns true if the extension is allowed to be enabled or disabled,
    // respectively.
    virtual bool CanEnableExtension(const Extension* extension) = 0;
    virtual bool CanDisableExtension(const Extension* extension) = 0;

    // Returns true if the extension should be blocked.
    virtual bool ShouldBlockExtension(const ExtensionId& extension_id) = 0;

    virtual void LoadExtensionForReload(const ExtensionId& extension_id,
                                        const base::FilePath& path) = 0;

   private:
    DISALLOW_COPY_AND_ASSIGN(Delegate);
  };

  explicit ExtensionRegistrar(content::BrowserContext* browser_context,
                              ExtensionPrefs* extension_prefs,
                              std::unique_ptr<Delegate> delegate);
  virtual ~ExtensionRegistrar();

  // Registers |extension| with the extension system by marking it enabled, then
  // notifying other components about it.
  void ActivateExtension(const Extension* extension);

  // Enables and activates the extension, if it is disabled. Does nothing if it
  // is already enabled.
  void EnableExtension(const ExtensionId& extension_id);

  // Disables |extension| by deactivating it. The ExtensionRegistry retains a
  // reference to it in disabled_extensions().
  void DisableExtension(const ExtensionId& extension_id, int disable_reasons);

  // Removes |extension| from the extension system by deactivating it and
  // removing references to it in the ExtensionRegistry.
  void DeactivateAndRemoveExtension(const ExtensionId& extension_id,
                                    UnloadedExtensionReason reason);

  // Reloads the specified extension, sending the onLaunched() event to it if it
  // currently has any window showing.
  // Allows noisy failures.
  // NOTE: Reloading an extension can invalidate Extension pointers to it.
  // Consider making a copy of the extension's ID first and retrieving a new
  // Extension pointer afterwards.
  /*
  void ReloadExtension(const ExtensionId& extension_id);
  */

  // Catch-all function to add an extension. Supports adding a newly loaded
  // extension, upgrading an existing extension, and reloading an extension
  // being reloaded.
  // TODO(michaelpg): Migrate call sites to call more specific functions and
  // remove this monolith.
  void AddExtension(const Extension* extension);

  // Removes the extension from the terminated list. This may lead to the
  // extension being deleted.
  void UntrackTerminatedExtension(const ExtensionId& extension_id);

  // TODO: constness of these fns
  bool CanReloadExtension(const ExtensionId& extension_id);

  void ReloadExtension(const ExtensionId& transient_extension_id);

 private:
  // Unregisters the extension with the extension system. Used when disabling or
  // removing an extension.
  void DeactivateExtensionImpl(scoped_refptr<const Extension> extension,
                               UnloadedExtensionReason reason,
                               bool was_enabled);

  void MaybeSpinUpLazyBackgroundPage(const Extension* extension);

  // TODO ..
  bool IsExtensionEnabled(const ExtensionId& extension_id) const;

  // Marks the extension ready after URLRequestContexts have been updated on
  // the IO thread.
  void OnExtensionRegisteredWithRequestContexts(
      scoped_refptr<const Extension> extension);

  // We are owned by the ExtensionSystem, which guarantees that these objects
  // outlive us.
  content::BrowserContext* browser_context_;
  ExtensionPrefs* extension_prefs_;
  ExtensionSystem* const extension_system_;
  ExtensionRegistry* const registry_;

  // The associated RendererStartupHelper. Guaranteed to outlive the
  // ExtensionSystem, and thus us.
  RendererStartupHelper* renderer_helper_;

  // IDs of extensions being reloaded. We use this to re-enable extensions
  // which were disabled for a reload.
  ExtensionIdSet reloading_extensions_;

  // IDs of extensions being terminated. We use this to avoid trying to
  // unregister the same extension twice.
  // ExtensionIdSet terminating_extensions_;

  // Map unloaded extensions' IDs to their paths. When a temporarily loaded
  // extension is unloaded, we lose the information about it and don't have
  // any in the extension preferences file.
  using UnloadedExtensionPathMap = std::map<std::string, base::FilePath>;
  UnloadedExtensionPathMap unloaded_extension_paths_;

  // Map of DevToolsAgentHost instances that are detached,
  // waiting for an extension to be reloaded.
  using OrphanedDevTools =
      std::map<ExtensionId, scoped_refptr<content::DevToolsAgentHost>>;
  OrphanedDevTools orphaned_dev_tools_;

  std::unique_ptr<Delegate> delegate_;

  base::WeakPtrFactory<ExtensionRegistrar> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(ExtensionRegistrar);
};

}  // namespace extensions

#endif  // EXTENSIONS_BROWSER_EXTENSION_REGISTRAR_H_
