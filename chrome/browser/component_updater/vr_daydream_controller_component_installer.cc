// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/component_updater/vr_daydream_controller_component_installer.h"

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
const base::FilePath::CharType kVrDaydreamControllerModelFileName[] =
    FILE_PATH_LITERAL("controller.gltf");

// The SHA256 of the SubjectPublicKeyInfo used to sign the extension.
// 0c374bb848e3921ae155c4f86e012d1d0cf44f6cb550f4c02b042f19e463a3ab
// The extension id is: amdhellieiodjcbkobffmepigoabcnbn
const uint8_t kPublicKeySHA256[32] = {0x0c, 0x37, 0x4b, 0xb8, 0x48, 0xe3, 0x92, 0x1a, 0xe1, 0x55, 0xc4, 0xf8, 0x6e, 0x01, 0x2d, 0x1d, 0x0c, 0xf4, 0x4f, 0x6c, 0xb5, 0x50, 0xf4, 0xc0, 0x2b, 0x04, 0x2f, 0x19, 0xe4, 0x63, 0xa3, 0xab};

const char kVrDaydreamControllerManifestName[] = "VR Daydream Controller Model";

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

bool VrDaydreamControllerComponentInstallerTraits::
    SupportsGroupPolicyEnabledComponentUpdates() const {
  LOG(INFO) << "Daydream Controller Component - SupportsGroupPolicyEnabledComponentUpdates";
  return false;
}

bool VrDaydreamControllerComponentInstallerTraits::RequiresNetworkEncryption()
    const {
  LOG(INFO) << "Daydream Controller Component - RequiresNetworkEncryption";
  return false;
}

update_client::CrxInstaller::Result
VrDaydreamControllerComponentInstallerTraits::OnCustomInstall(
    const base::DictionaryValue& manifest,
    const base::FilePath& install_dir) {
  LOG(INFO) << "Daydream Controller Component - OnCustomInstall";
  return update_client::CrxInstaller::Result(0);  // Nothing custom here.
}

base::FilePath VrDaydreamControllerComponentInstallerTraits::GetInstalledPath(
    const base::FilePath& base) {
  LOG(INFO) << "Daydream Controller Component - GetInstalledPath " << base.Append(kVrDaydreamControllerModelFileName).value();
  return base.Append(kVrDaydreamControllerModelFileName);
}

void VrDaydreamControllerComponentInstallerTraits::ComponentReady(
    const base::Version& version,
    const base::FilePath& install_dir,
    std::unique_ptr<base::DictionaryValue> manifest) {
  LOG(INFO) << "Daydream Controller Component - Component ready, version " << version.GetString() << " in "
          << install_dir.value();

  /*base::PostTaskWithTraits(
      FROM_HERE,
      base::TaskTraits().MayBlock().WithPriority(
          base::TaskPriority::BACKGROUND),
      base::Bind(&LoadFileTypesFromDisk, GetInstalledPath(install_dir)));*/
}

// Called during startup and installation before ComponentReady().
bool VrDaydreamControllerComponentInstallerTraits::VerifyInstallation(
    const base::DictionaryValue& manifest,
    const base::FilePath& install_dir) const {
  // No need to actually validate the proto here, since we'll do the checking
  // in PopulateFromDynamicUpdate().
  LOG(INFO) << "Daydream Controller Component - VerifyInstallation " << install_dir.value();
  return base::PathExists(GetInstalledPath(install_dir));
}

base::FilePath VrDaydreamControllerComponentInstallerTraits::GetRelativeInstallDir()
    const {
  LOG(INFO) << "Daydream Controller Component - GetRelativeInstallDir " << base::FilePath(FILE_PATH_LITERAL("VRDaydreamController")).value();
  return base::FilePath(FILE_PATH_LITERAL("VRDaydreamController"));
}

void VrDaydreamControllerComponentInstallerTraits::GetHash(
    std::vector<uint8_t>* hash) const {
  LOG(INFO) << "Daydream Controller Component - GetHash";
  hash->assign(kPublicKeySHA256,
               kPublicKeySHA256 + arraysize(kPublicKeySHA256));
}

std::string VrDaydreamControllerComponentInstallerTraits::GetName() const {
  LOG(INFO) << "Daydream Controller Component - GetName " << kVrDaydreamControllerManifestName;
  return kVrDaydreamControllerManifestName;
}

update_client::InstallerAttributes
VrDaydreamControllerComponentInstallerTraits::GetInstallerAttributes() const {
  LOG(INFO) << "Daydream Controller Component - GetInstallerAttributes";
  return update_client::InstallerAttributes();
}

std::vector<std::string>
VrDaydreamControllerComponentInstallerTraits::GetMimeTypes() const {
  LOG(INFO) << "Daydream Controller Component - GetMimeTypes";
  return std::vector<std::string>();
}

void RegisterVrDaydreamControllerComponent(ComponentUpdateService* cus,
                                       const base::FilePath& user_data_dir) {
  LOG(INFO) << "Registering VR Daydream Controller Component Installer.";

  std::unique_ptr<ComponentInstallerTraits> traits(
      new VrDaydreamControllerComponentInstallerTraits());
  // |cus| will take ownership of |installer| during installer->Register(cus).
  DefaultComponentInstaller* installer =
      new DefaultComponentInstaller(std::move(traits));
  installer->Register(cus, base::Closure());
}

}  // namespace component_updater
