// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/component_updater/downloadable_strings_component_installer.h"

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
#include "components/component_updater/component_updater_paths.h"

using component_updater::ComponentUpdateService;

namespace {
const base::FilePath::CharType kDownloadableStringsBinaryTestFileName[] =
    FILE_PATH_LITERAL("downloaded_locales.pak");

// The SHA256 of the SubjectPublicKeyInfo used to sign the extension.
// The extension id is: TODO(digit)
const uint8_t kPublicKeySHA256[32] = {/* TODO(digit) */};

const char kDownloadableStringsManifestName[] = "Downloadable Strings";

void LoadFileTypesFromDisk(const base::FilePath& pb_path) {
  if (pb_path.empty())
    return;

  VLOG(1) << "Reading Downloadable Strings from file: " << pb_path.value();
  std::string binary_pb;
  if (!base::ReadFileToString(pb_path, &binary_pb)) {
    // The file won't exist on new installations, so this is not always an
    // error.
    VLOG(1) << "Failed reading from " << pb_path.value();
    return;
  }

  // Notify UMA that the download succeeded?
  // TODO(digit): Implement this?
}

}  // namespace

namespace component_updater {

bool DownloadableStringsComponentInstallerTraits::
    SupportsGroupPolicyEnabledComponentUpdates() const {
  return false;
}

bool DownloadableStringsComponentInstallerTraits::RequiresNetworkEncryption()
    const {
  return false;
}

update_client::CrxInstaller::Result
DownloadableStringsComponentInstallerTraits::OnCustomInstall(
    const base::DictionaryValue& manifest,
    const base::FilePath& install_dir) {
  return update_client::CrxInstaller::Result(0);  // Nothing custom here.
}

base::FilePath DownloadableStringsComponentInstallerTraits::GetInstalledPath(
    const base::FilePath& base) {
  return base.Append(kDownloadableStringsBinaryTestFileName);
}

void DownloadableStringsComponentInstallerTraits::ComponentReady(
    const base::Version& version,
    const base::FilePath& install_dir,
    std::unique_ptr<base::DictionaryValue> manifest) {
  VLOG(1) << "Component ready, version " << version.GetString() << " in "
          << install_dir.value();

  base::PostTaskWithTraits(
      FROM_HERE, {base::MayBlock(), base::TaskPriority::BACKGROUND},
      base::BindOnce(&LoadFileTypesFromDisk, GetInstalledPath(install_dir)));
}

// Called during startup and installation before ComponentReady().
bool DownloadableStringsComponentInstallerTraits::VerifyInstallation(
    const base::DictionaryValue& manifest,
    const base::FilePath& install_dir) const {
  // No need to actually validate the file here.
  return base::PathExists(GetInstalledPath(install_dir));
}

base::FilePath
DownloadableStringsComponentInstallerTraits::GetRelativeInstallDir() const {
  return base::FilePath(FILE_PATH_LITERAL("DownloadableStrings"));
}

void DownloadableStringsComponentInstallerTraits::GetHash(
    std::vector<uint8_t>* hash) const {
  hash->assign(kPublicKeySHA256,
               kPublicKeySHA256 + arraysize(kPublicKeySHA256));
}

std::string DownloadableStringsComponentInstallerTraits::GetName() const {
  return kDownloadableStringsManifestName;
}

update_client::InstallerAttributes
DownloadableStringsComponentInstallerTraits::GetInstallerAttributes() const {
  return update_client::InstallerAttributes();
}

std::vector<std::string>
DownloadableStringsComponentInstallerTraits::GetMimeTypes() const {
  return {};
}

void RegisterDownloadableStringsComponent(ComponentUpdateService* cus) {
  VLOG(1) << "Registering Downloadable Strings component.";

  std::unique_ptr<ComponentInstallerTraits> traits(
      new DownloadableStringsComponentInstallerTraits());
  // |cus| will take ownership of |installer| during installer->Register(cus).
  DefaultComponentInstaller* installer =
      new DefaultComponentInstaller(std::move(traits));
  installer->Register(cus, base::Closure());
}

}  // namespace component_updater
