// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/component_updater/optimization_hints_component_installer.h"

#include <utility>

#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/path_service.h"
#include "base/version.h"
#include "chrome/browser/browser_process.h"
#include "chrome/common/pref_names.h"
#include "components/component_updater/component_updater_paths.h"
#include "components/prefs/pref_service.h"

using component_updater::ComponentUpdateService;

namespace component_updater {

// The extension id is: lmelglejhemejginpboagddgdfbepgmp
const uint8_t kPublicKeySHA256[32] = {
    0xbc, 0x4b, 0x6b, 0x49, 0x74, 0xc4, 0x98, 0x8d, 0xf1, 0xe0, 0x63,
    0x36, 0x35, 0x14, 0xf6, 0xcf, 0x86, 0x92, 0xe6, 0x06, 0x03, 0x76,
    0x70, 0xaf, 0x8b, 0xd4, 0x47, 0x2c, 0x42, 0x59, 0x38, 0xef};

// Change above to below to hit dev env.
/*
    0x23, 0xbf, 0xc2, 0x9a, 0xe6, 0x94, 0x4f, 0x7a, 0x40, 0x87, 0x3b,
    0x2f, 0x5c, 0x1b, 0x2e, 0xb9, 0x79, 0x14, 0xd0, 0xd6, 0x34, 0x48,
    0x5e, 0x42, 0xff, 0xb8, 0x88, 0x31, 0x7a, 0x9e, 0xe9, 0x5f}; */

const char kOptimizationHintsSetFetcherManifestName[] = "Optimization Hints";

// static
const char
    OptimizationHintsComponentInstallerTraits::kManifestRulesetFormatKey[] =
        "ruleset_format";

// static
const double OptimizationHintsComponentInstallerTraits::kCurrentRulesetFormat =
    1;

OptimizationHintsComponentInstallerTraits::
    OptimizationHintsComponentInstallerTraits() {}

OptimizationHintsComponentInstallerTraits::
    ~OptimizationHintsComponentInstallerTraits() {}

bool OptimizationHintsComponentInstallerTraits::
    SupportsGroupPolicyEnabledComponentUpdates() const {
  return false;
}

bool OptimizationHintsComponentInstallerTraits::RequiresNetworkEncryption()
    const {
  return false;
}

update_client::CrxInstaller::Result
OptimizationHintsComponentInstallerTraits::OnCustomInstall(
    const base::DictionaryValue& manifest,
    const base::FilePath& install_dir) {
  return update_client::CrxInstaller::Result(0);  // Nothing custom here.
}

void OptimizationHintsComponentInstallerTraits::ComponentReady(
    const base::Version& version,
    const base::FilePath& install_dir,
    std::unique_ptr<base::DictionaryValue> manifest) {
  DCHECK(!install_dir.empty());
  DVLOG(1) << "Optimization Hints Version Ready: " << version.GetString();
  double ruleset_format = 0;
  if (!manifest->GetDouble(kManifestRulesetFormatKey, &ruleset_format) ||
      ruleset_format > kCurrentRulesetFormat) {
    DVLOG(1) << "Bailing out. Future ruleset version: " << ruleset_format;
    return;
  }
  DVLOG(1) << "Got some optimization hints";
  // TODO(sophiechang): Do something with the data.
}

// Called during startup and installation before ComponentReady().
bool OptimizationHintsComponentInstallerTraits::VerifyInstallation(
    const base::DictionaryValue& manifest,
    const base::FilePath& install_dir) const {
  return base::PathExists(install_dir);
}

base::FilePath
OptimizationHintsComponentInstallerTraits::GetRelativeInstallDir() const {
  return base::FilePath(FILE_PATH_LITERAL("OptimizationHints"));
}

void OptimizationHintsComponentInstallerTraits::GetHash(
    std::vector<uint8_t>* hash) const {
  if (!hash) {
    return;
  }
  hash->assign(std::begin(kPublicKeySHA256), std::end(kPublicKeySHA256));
}

std::string OptimizationHintsComponentInstallerTraits::GetName() const {
  return kOptimizationHintsSetFetcherManifestName;
}

update_client::InstallerAttributes
OptimizationHintsComponentInstallerTraits::GetInstallerAttributes() const {
  return update_client::InstallerAttributes();
}

std::vector<std::string>
OptimizationHintsComponentInstallerTraits::GetMimeTypes() const {
  return std::vector<std::string>();
}

void RegisterOptimizationHintsComponent(ComponentUpdateService* cus,
                                        PrefService* prefs) {
  // TODO(sophiechang): Also add Cacao feature check.
  if (!prefs || !prefs->GetBoolean(prefs::kDataSaverEnabled)) {
    DVLOG(1) << "Data Saver not enabled.";
    return;
  }
  DVLOG(1) << "Registering Optimization Hints Component";
  std::unique_ptr<ComponentInstallerTraits> traits(
      new OptimizationHintsComponentInstallerTraits());
  // |cus| will take ownership of |installer| during installer->Register(cus).
  DefaultComponentInstaller* installer =
      new DefaultComponentInstaller(std::move(traits));
  installer->Register(cus, base::Closure());
}

}  // namespace component_updater
