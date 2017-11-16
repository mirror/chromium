// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/component_updater/vr_assets_component_installer.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/bind.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/logging.h"
#include "base/macros.h"
#include "base/path_service.h"
#include "base/task_scheduler/post_task.h"
#include "base/version.h"
#include "chrome/common/safe_browsing/file_type_policies.h"
#include "components/component_updater/component_updater_paths.h"

using component_updater::ComponentUpdateService;

namespace {
const base::FilePath::CharType kEnvironmentFileName[] =
    FILE_PATH_LITERAL("environment");

// The SHA256 of the SubjectPublicKeyInfo used to sign the extension.
// The extension id is: cjfkbpdpjpdldhclahpfgnlhpodlpnba
const uint8_t kPublicKeySHA256[32] = {
    0x29, 0x5a, 0x1f, 0x3f, 0x9f, 0x3b, 0x37, 0x2b, 0x07, 0xf5, 0x6d,
    0xb7, 0xfe, 0x3b, 0xfd, 0x10, 0xb6, 0x80, 0xf3, 0x66, 0x0d, 0xc3,
    0xe2, 0x07, 0x25, 0x8d, 0x37, 0x85, 0x39, 0x51, 0x58, 0xcf};

const char kVrAssetsComponentName[] = "VR Assets";
const base::FilePath::CharType kRelativeInstallDir[] =
    FILE_PATH_LITERAL("VrAssets");

/*void LoadFileTypesFromDisk(const base::FilePath& pb_path) {
  if (pb_path.empty())
    return;

  VLOG(1) << "Reading model from file: " << pb_path.value();
  std::string binary_pb;
  if (!base::ReadFileToString(pb_path, &binary_pb)) {
    // The file won't exist on new installations, so this is not always an
    // error.
    VLOG(1) << "Failed reading from " << pb_path.value();
    return;
  }

  safe_browsing::FileTypePolicies::GetInstance()->PopulateFromDynamicUpdate(
      binary_pb);
}*/

}  // namespace

namespace component_updater {

bool VrAssetsComponentInstallerTraits::
    SupportsGroupPolicyEnabledComponentUpdates() const {
  LOG(INFO) << "VrAssetsComponent - SupportsGroupPolicyEnabledComponentUpdates";
  return false;
}

bool VrAssetsComponentInstallerTraits::RequiresNetworkEncryption() const {
  LOG(INFO) << "VrAssetsComponent - RequiresNetworkEncryption";
  return false;
}

update_client::CrxInstaller::Result
VrAssetsComponentInstallerTraits::OnCustomInstall(
    const base::DictionaryValue& manifest,
    const base::FilePath& install_dir) {
  LOG(INFO) << "VrAssetsComponent - OnCustomInstall";
  return update_client::CrxInstaller::Result(0);  // Nothing custom here.
}

void VrAssetsComponentInstallerTraits::OnCustomUninstall() {}

base::FilePath VrAssetsComponentInstallerTraits::GetInstalledPath(
    const base::FilePath& base) {
  LOG(INFO) << "VrAssetsComponent - GetInstalledPath "
            << base.Append(kEnvironmentFileName).value();
  return base.Append(kEnvironmentFileName);
}

void VrAssetsComponentInstallerTraits::ComponentReady(
    const base::Version& version,
    const base::FilePath& install_dir,
    std::unique_ptr<base::DictionaryValue> manifest) {
  LOG(INFO) << "VrAssetsComponent - Component ready, version "
            << version.GetString() << " in " << install_dir.value();

  /*base::PostTaskWithTraits(
      FROM_HERE,
      base::TaskTraits().MayBlock().WithPriority(
          base::TaskPriority::BACKGROUND),
      base::Bind(&LoadFileTypesFromDisk, GetInstalledPath(install_dir)));*/
}

// Called during startup and installation before ComponentReady().
bool VrAssetsComponentInstallerTraits::VerifyInstallation(
    const base::DictionaryValue& manifest,
    const base::FilePath& install_dir) const {
  // No need to actually validate the proto here, since we'll do the checking
  // in PopulateFromDynamicUpdate().
  LOG(INFO) << "VrAssetsComponent - VerifyInstallation " << install_dir.value();
  return base::PathExists(GetInstalledPath(install_dir));
}

base::FilePath VrAssetsComponentInstallerTraits::GetRelativeInstallDir() const {
  LOG(INFO) << "VrAssetsComponent - GetRelativeInstallDir "
            << kRelativeInstallDir;
  return base::FilePath(kRelativeInstallDir);
}

void VrAssetsComponentInstallerTraits::GetHash(
    std::vector<uint8_t>* hash) const {
  LOG(INFO) << "VrAssetsComponent - GetHash";
  hash->assign(kPublicKeySHA256,
               kPublicKeySHA256 + arraysize(kPublicKeySHA256));
}

std::string VrAssetsComponentInstallerTraits::GetName() const {
  LOG(INFO) << "VrAssetsComponent - GetName " << kVrAssetsComponentName;
  return kVrAssetsComponentName;
}

update_client::InstallerAttributes
VrAssetsComponentInstallerTraits::GetInstallerAttributes() const {
  LOG(INFO) << "VrAssetsComponent - GetInstallerAttributes";
  return update_client::InstallerAttributes();
}

std::vector<std::string> VrAssetsComponentInstallerTraits::GetMimeTypes()
    const {
  LOG(INFO) << "VrAssetsComponent - GetMimeTypes";
  return std::vector<std::string>();
}

void RegisterVrAssetsComponent(ComponentUpdateService* cus,
                               const base::FilePath& user_data_dir) {
  LOG(INFO) << "Registering VR VrAssetsComponent Installer.";

  std::unique_ptr<ComponentInstallerPolicy> policy(
      new VrAssetsComponentInstallerTraits());
  // |cus| will take ownership of |installer| during installer->Register(cus).
  auto installer = base::MakeRefCounted<ComponentInstaller>(std::move(policy));
  installer->Register(cus, base::Closure());
}

}  // namespace component_updater
