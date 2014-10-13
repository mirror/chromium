// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_EXTENSIONS_STANDARD_MANAGEMENT_POLICY_PROVIDER_H_
#define CHROME_BROWSER_EXTENSIONS_STANDARD_MANAGEMENT_POLICY_PROVIDER_H_

#include "extensions/browser/management_policy.h"

namespace extensions {

class Blacklist;
class Extension;
class ExtensionManagement;

// The standard management policy provider, which takes into account the
// extension black/whitelists and admin black/whitelists.
class StandardManagementPolicyProvider : public ManagementPolicy::Provider {
 public:
  explicit StandardManagementPolicyProvider(
      const ExtensionManagement* settings);

  virtual ~StandardManagementPolicyProvider();

  // ManagementPolicy::Provider implementation.
  virtual std::string GetDebugPolicyProviderName() const override;
  virtual bool UserMayLoad(const Extension* extension,
                           base::string16* error) const override;
  virtual bool UserMayModifySettings(const Extension* extension,
                                     base::string16* error) const override;
  virtual bool MustRemainEnabled(const Extension* extension,
                                 base::string16* error) const override;
  virtual bool MustRemainInstalled(const Extension* extension,
                                   base::string16* error) const override;

 private:
  const ExtensionManagement* settings_;
};

}  // namespace extensions

#endif  // CHROME_BROWSER_EXTENSIONS_STANDARD_MANAGEMENT_POLICY_PROVIDER_H_
