// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/component_updater/mei_preload_component_installer.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/bind.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/logging.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/path_service.h"
#include "base/task_scheduler/post_task.h"
#include "base/version.h"
#include "chrome/browser/media/media_engagement_preloaded_list.h"
#include "components/component_updater/component_updater_paths.h"

using component_updater::ComponentUpdateService;

namespace {

const base::FilePath::CharType kMediaEngagementPreloadBinaryPbFileName[] =
    FILE_PATH_LITERAL("preloaded_data.pb");

// The extension id is: aemomkdncapdnfajjbbcbdebjljbpmpj
const uint8_t kPublicKeySHA256[32] = {
    0x04, 0xce, 0xca, 0x3d, 0x20, 0xf3, 0xd5, 0x09, 0x91, 0x12, 0x13,
    0x41, 0x9b, 0x91, 0xfc, 0xf9, 0x19, 0xc4, 0x94, 0x6a, 0xb9, 0x9a,
    0xe1, 0xaf, 0x3b, 0x9a, 0x95, 0x85, 0x5b, 0x9e, 0x99, 0xed};

const char kMediaEngagementPreloadManifestName[] = "MEI Preload";

// TODO(beccahughes): Replace
void LoadPreloadedDataFromDisk(const base::FilePath& pb_path) {
  if (pb_path.empty())
    return;

  VLOG(1) << "Reading Download File Types from file: " << pb_path.value();
  std::string binary_pb;
  if (!base::ReadFileToString(pb_path, &binary_pb)) {
    // The file won't exist on new installations, so this is not always an
    // error.
    VLOG(1) << "Failed reading from " << pb_path.value();
    return;
  }

  DLOG(ERROR) << "loaded";
  DLOG(ERROR) << pb_path.value();

  MediaEngagementPreloadedList::GetInstance()->LoadFromFile(pb_path);
}

}  // namespace

namespace component_updater {

bool MediaEngagementPreloadComponentInstallerPolicy::
    SupportsGroupPolicyEnabledComponentUpdates() const {
  return false;
}

bool MediaEngagementPreloadComponentInstallerPolicy::RequiresNetworkEncryption()
    const {
  return false;
}

update_client::CrxInstaller::Result
MediaEngagementPreloadComponentInstallerPolicy::OnCustomInstall(
    const base::DictionaryValue& manifest,
    const base::FilePath& install_dir) {
  return update_client::CrxInstaller::Result(0);  // Nothing custom here.
}

void MediaEngagementPreloadComponentInstallerPolicy::OnCustomUninstall() {}

base::FilePath MediaEngagementPreloadComponentInstallerPolicy::GetInstalledPath(
    const base::FilePath& base) {
  return base.Append(kMediaEngagementPreloadBinaryPbFileName);
}

void MediaEngagementPreloadComponentInstallerPolicy::ComponentReady(
    const base::Version& version,
    const base::FilePath& install_dir,
    std::unique_ptr<base::DictionaryValue> manifest) {
  VLOG(1) << "Component ready, version " << version.GetString() << " in "
          << install_dir.value();

  base::PostTaskWithTraits(FROM_HERE,
                           {base::MayBlock(), base::TaskPriority::BACKGROUND},
                           base::BindOnce(&LoadPreloadedDataFromDisk,
                                          GetInstalledPath(install_dir)));
}

// Called during startup and installation before ComponentReady().
bool MediaEngagementPreloadComponentInstallerPolicy::VerifyInstallation(
    const base::DictionaryValue& manifest,
    const base::FilePath& install_dir) const {
  // No need to actually validate the proto here, since we'll do the checking
  // in PopulateFromDynamicUpdate().
  return base::PathExists(GetInstalledPath(install_dir));
}

base::FilePath
MediaEngagementPreloadComponentInstallerPolicy::GetRelativeInstallDir() const {
  return base::FilePath(FILE_PATH_LITERAL("MEIPreload"));
}

void MediaEngagementPreloadComponentInstallerPolicy::GetHash(
    std::vector<uint8_t>* hash) const {
  hash->assign(kPublicKeySHA256,
               kPublicKeySHA256 + arraysize(kPublicKeySHA256));
}

std::string MediaEngagementPreloadComponentInstallerPolicy::GetName() const {
  return kMediaEngagementPreloadManifestName;
}

update_client::InstallerAttributes
MediaEngagementPreloadComponentInstallerPolicy::GetInstallerAttributes() const {
  return update_client::InstallerAttributes();
}

std::vector<std::string>
MediaEngagementPreloadComponentInstallerPolicy::GetMimeTypes() const {
  return std::vector<std::string>();
}

void RegisterMediaEngagementPreloadComponent(ComponentUpdateService* cus) {
  VLOG(1) << "Registering MEI Preload component.";
  auto installer = base::MakeRefCounted<ComponentInstaller>(
      std::make_unique<MediaEngagementPreloadComponentInstallerPolicy>());
  installer->Register(cus, base::OnceClosure());
}

}  // namespace component_updater
