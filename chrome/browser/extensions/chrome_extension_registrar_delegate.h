// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_EXTENSIONS_CHROME_EXTENSION_REGISTRAR_DELEGATE_H_
#define CHROME_BROWSER_EXTENSIONS_CHROME_EXTENSION_REGISTRAR_DELEGATE_H_

#include "extensions/browser/extension_registrar.h"

#include "base/macros.h"

class ExtensionService;
class Profile;

namespace extensions {

class ExtensionRegistry;
class ManagementPolicy;

// Chrome delegate for ExtensionRegistrar that adds additional checks and
// notifications for adding, enabling and disabling extensions.
class ChromeExtensionRegistrarDelegate : public ExtensionRegistrar::Delegate {
 public:
  ChromeExtensionRegistrarDelegate(ExtensionService* extension_service,
                                   Profile* profile,
                                   ExtensionRegistry* registry,
                                   ManagementPolicy* management_policy);
  ~ChromeExtensionRegistrarDelegate() override;

  // ExtensionRegistrar::Delegate:
  void PostEnableExtension(scoped_refptr<const Extension> extension) override;
  void PostDisableExtension(scoped_refptr<const Extension> extension) override;
  bool CanEnableExtension(const Extension* extension) override;
  bool CanDisableExtension(const Extension* extension) override;
  bool ShouldBlockExtension(const ExtensionId& extension_id) override;

 private:
  ExtensionService* const extension_service_;
  Profile* const profile_;
  ExtensionRegistry* const registry_;
  ManagementPolicy* const management_policy_;

  DISALLOW_COPY_AND_ASSIGN(ChromeExtensionRegistrarDelegate);
};

}  // namespace extensions

#endif  // CHROME_BROWSER_EXTENSIONS_CHROME_EXTENSION_REGISTRAR_DELEGATE_H_
