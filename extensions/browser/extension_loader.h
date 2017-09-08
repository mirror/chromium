// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EXTENSIONS_BROWSER_EXTENSION_LOADER_H_
#define EXTENSIONS_BROWSER_EXTENSION_LOADER_H_

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "extensions/common/extension.h"

namespace content {
class BrowserContext;
}  // namespace content

namespace extensions {
class ExtensionRegistry;
class ExtensionSystem;
class RendererStartupHelper;
}  // namespace extensions

namespace extensions {

// Utility class for notifying services in the proper order when an extension
// has been loaded or unloaded. This triggers the initialization (or teardown)
// of the extension across services that use it.
class ExtensionLoader {
 public:
  explicit ExtensionLoader(content::BrowserContext* browser_context);
  virtual ~ExtensionLoader();

  // Notifies that the extension has been loaded.
  void NotifyExtensionLoaded(scoped_refptr<const Extension> extension);

  // Notifies that the extension is being unloaded.
  void NotifyExtensionUnloaded(scoped_refptr<const Extension> extension,
                               UnloadedExtensionReason reason);

 private:
  // Marks the extension ready after URLRequestContexts have been updated on
  // the IO thread.
  void OnExtensionRegisteredWithRequestContexts(
      scoped_refptr<const Extension> extension);

  ExtensionSystem* const extension_system_;
  ExtensionRegistry* const registry_;
  RendererStartupHelper* const renderer_helper_;

  base::WeakPtrFactory<ExtensionLoader> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(ExtensionLoader);
};

}  // namespace extensions

#endif  // EXTENSIONS_BROWSER_EXTENSION_LOADER_H_
