// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EXTENSIONS_BROWSER_EXTENSION_REGISTRAR_H_
#define EXTENSIONS_BROWSER_EXTENSION_REGISTRAR_H_

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "extensions/common/extension.h"

namespace content {
class BrowserContext;
}  // namespace content

namespace extensions {

class Extension;
class ExtensionRegistry;
class ExtensionSystem;
class RendererStartupHelper;

// ExtensionRegistrar drives the stages of registering and unregistering
// extensions for a BrowserContext.
// TODO(michaelpg): Add functionality for reloading cand terminating extensions.
class ExtensionRegistrar {
 public:
  explicit ExtensionRegistrar(content::BrowserContext* browser_context);
  virtual ~ExtensionRegistrar();

  // Registers |extension| with the extension system by marking it enabled and
  // notifying other components about it.
  void ActivateExtension(scoped_refptr<const Extension> extension);

  // Disables |extension| by deactivating it. The ExtensionRegistry retains a
  // reference to it in disabled_extensions().
  void DeactivateExtension(scoped_refptr<const Extension> extension);

  // Removes |extension| from the extension system by deactivating it and
  // removing references to it in the ExtensionRegistry.
  void RemoveExtension(const ExtensionId& extension_id,
                       UnloadedExtensionReason reason);

 private:
  // Unregisters the extension with the extension system. Used when disabling or
  // removing an extension.
  void DeactivateExtensionImpl(scoped_refptr<const Extension> extension,
                               UnloadedExtensionReason reason,
                               bool was_enabled);

  // Marks the extension ready after URLRequestContexts have been updated on
  // the IO thread.
  void OnExtensionRegisteredWithRequestContexts(
      scoped_refptr<const Extension> extension);

  content::BrowserContext* browser_context_;
  ExtensionSystem* const extension_system_;
  ExtensionRegistry* const registry_;

  // The associated RendererStartupHelper. Guaranteed to outlive the
  // ExtensionSystem, and thus us.
  extensions::RendererStartupHelper* renderer_helper_;

  base::WeakPtrFactory<ExtensionRegistrar> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(ExtensionRegistrar);
};

}  // namespace extensions

#endif  // EXTENSIONS_BROWSER_EXTENSION_REGISTRAR_H_
