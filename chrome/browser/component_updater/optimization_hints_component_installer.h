// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_COMPONENT_UPDATER_OPTIMIZATION_HINTS_COMPONENT_INSTALLER_H_
#define CHROME_BROWSER_COMPONENT_UPDATER_OPTIMIZATION_HINTS_COMPONENT_INSTALLER_H_

#include <memory>
#include <string>
#include <vector>

#include "base/gtest_prod_util.h"
#include "components/component_updater/default_component_installer.h"

class PrefService;

namespace base {
class FilePath;
}  // namespace base

namespace component_updater {

class ComponentUpdateService;

// Component for receiving Chrome Optimization Hints.
class OptimizationHintsComponentInstallerTraits
    : public ComponentInstallerTraits {
 public:
  static const char kManifestRulesetFormatKey[];
  static const double kCurrentRulesetFormat;

  OptimizationHintsComponentInstallerTraits();
  ~OptimizationHintsComponentInstallerTraits() override;

 private:
  friend class OptimizationHintsComponentInstallerTest;

  // ComponentInstallerTraits implementation.
  bool SupportsGroupPolicyEnabledComponentUpdates() const override;
  bool RequiresNetworkEncryption() const override;
  update_client::CrxInstaller::Result OnCustomInstall(
      const base::DictionaryValue& manifest,
      const base::FilePath& install_dir) override;
  bool VerifyInstallation(const base::DictionaryValue& manifest,
                          const base::FilePath& install_dir) const override;
  void ComponentReady(const base::Version& version,
                      const base::FilePath& install_dir,
                      std::unique_ptr<base::DictionaryValue> manifest) override;
  base::FilePath GetRelativeInstallDir() const override;
  void GetHash(std::vector<uint8_t>* hash) const override;
  std::string GetName() const override;
  update_client::InstallerAttributes GetInstallerAttributes() const override;
  std::vector<std::string> GetMimeTypes() const override;

  DISALLOW_COPY_AND_ASSIGN(OptimizationHintsComponentInstallerTraits);
};

void RegisterOptimizationHintsComponent(ComponentUpdateService* cus,
                                        PrefService* prefs);

}  // namespace component_updater

#endif  // CHROME_BROWSER_COMPONENT_UPDATER_OPTIMIZATION_HINTS_COMPONENT_INSTALLER_H_
