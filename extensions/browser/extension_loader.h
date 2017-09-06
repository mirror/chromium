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
// command line. Must run on the UI thread.
class ExtensionLoader {
 public:
  explicit ExtensionLoader(content::BrowserContext* browser_context);
  virtual ~ExtensionLoader();

  // Loads an unpacked extension from the command line synchronously.
  scoped_refptr<const Extension> LoadExtensionFromCommandLine(
      const base::FilePath& extension_dir);

  // Notifies that the extension has been loaded.
  void NotifyExtensionLoaded(scoped_refptr<const Extension> extension);

  // Notifies that the extension is being unloaded.
  void NotifyExtensionUnloaded(scoped_refptr<const Extension> extension,
                               UnloadedExtensionReason reason);

  base::WeakPtr<ExtensionLoader> AsWeakPtr() {
    return weak_factory_.GetWeakPtr();
  }

 protected:
  content::BrowserContext* browser_context() const { return browser_context_; }

 private:
  // Marks the extension ready after URLRequestContexts have been updated on
  // the IO thread.
  void OnExtensionRegisteredWithRequestContexts(
      scoped_refptr<const Extension> extension);

  content::BrowserContext* const browser_context_;
  ExtensionSystem* const extension_system_;
  ExtensionRegistry* const registry_;
  RendererStartupHelper* const renderer_helper_;

  base::WeakPtrFactory<ExtensionLoader> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(ExtensionLoader);
};

}  // namespace extensions

#endif  // EXTENSIONS_BROWSER_EXTENSION_LOADER_H_
