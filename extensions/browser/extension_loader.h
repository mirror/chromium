// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EXTENSIONS_BROWSER_EXTENSION_LOADER_H_
#define EXTENSIONS_BROWSER_EXTENSION_LOADER_H_

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "extensions/common/extension.h"

namespace base {
class FilePath;
}  // namespace base

namespace content {
class BrowserContext;
}  // namespace content

namespace extensions {
class ExtensionRegistry;
class ExtensionSystem;
class RendererStartupHelper;
}  // namespace extensions

namespace extensions {

// Utility class for loading and unloading unpacked extensions from the
// command line. Use on the UI thread.
class ExtensionLoader {
 public:
  explicit ExtensionLoader(content::BrowserContext* browser_context);
  virtual ~ExtensionLoader();

  // Loads an unpacked extension from the command line. Happens synchronously
  // to avoid a race between extension loading and loading an URL from the
  // command line. TODO(michaelpg): Add non-synchronous functions for use after
  // startup, where IO is restricted and the race condition doesn't matter.
  scoped_refptr<const Extension> LoadExtensionFromCommandLine(
      const base::FilePath& extension_dir);

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
