// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_EXTENSIONS_EXTENSION_REGISTRAR_DELEGATE_H_
#define CHROME_BROWSER_EXTENSIONS_EXTENSION_REGISTRAR_DELEGATE_H_

#include "extensions/browser/extension_registrar.h"

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "extensions/common/extension_id.h"

class ExtensionService;
class Profile;

namespace extensions {

class Extension;
class ExtensionPrefs;
class ExtensionRegistry;
class ManagementPolicy;

// Chrome delegate for ExtensionRegistrar that checks if an extension's
// privileges have changed and marks the extension disabled if necessary before
// the registrar proceeds.
class ExtensionRegistrarDelegate : public ExtensionRegistrar::Delegate {
 public:
  ExtensionRegistrarDelegate(ExtensionService* extension_service,
                             Profile* profile,
                             ExtensionPrefs* extension_prefs,
                             ExtensionRegistry* registry,
                             ManagementPolicy* management_policy);
  ~ExtensionRegistrarDelegate() override;

  // ExtensionRegistrar::Delegate:
  void CheckExtensionBeforeAdding(const Extension* extension,
                                  bool is_extension_loaded) override;
  void PostEnableExtension(scoped_refptr<const Extension> extension) override;
  void PostDisableExtension(
      scoped_refptr<const Extension> extension) override;
  void OnAddedExtensionDisabled(const Extension* extension) override;
  bool ShouldBlockExtension(const ExtensionId& extension_id) override;
  bool CanEnableExtension(const Extension* extension) override;
  bool CanDisableExtension(const Extension* extension) override;

 private:
  void GrantPermissions(const Extension* extension);

  ExtensionService* const extension_service_;
  Profile* const profile_;
  ExtensionPrefs* const extension_prefs_;
  ExtensionRegistry* const registry_;
  ManagementPolicy* const management_policy_;

  DISALLOW_COPY_AND_ASSIGN(ExtensionRegistrarDelegate);
};

}  // namespace extensions

#endif  // CHROME_BROWSER_EXTENSIONS_EXTENSION_REGISTRAR_DELEGATE_H_
