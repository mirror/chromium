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
#include "base/memory/ptr_util.h"
#include "base/path_service.h"
#include "base/task_scheduler/post_task.h"
#include "base/version.h"
#include "components/component_updater/component_updater_paths.h"

using component_updater::ComponentUpdateService;

namespace {
const base::FilePath::CharType kDownloadableStringsBinaryTestFileName[] =
    FILE_PATH_LITERAL("dummy-locale.pak");

// CRX ID, this is the hex-encoding of the public key hash below, but using
// [a-p] as digits, instead of [0-9a-f].
const char kCrxId[] = "fookoiellkocclipolgaceabajejjcnp";

// The SHA256 of the SubjectPublicKeyInfo used to sign the extension.
const uint8_t kPublicKeySHA256[32] = {
    0x5e, 0xea, 0xe8, 0x4b, 0xba, 0xe2, 0x2b, 0x8f, 0xeb, 0x60, 0x24, 0x01,
    0x09, 0x49, 0x92, 0xdf, 0x7b, 0xe9, 0x41, 0x50, 0xba, 0x97, 0x39, 0x7e};

const char kDownloadableStringsManifestName[] = "Downloadable Strings";

void LoadFileTypesFromDisk(const base::FilePath& pb_path) {
  if (pb_path.empty())
    return;

  LOG(INFO) << "Reading Downloadable Strings from file: " << pb_path.value();
  std::string binary_pb;
  if (!base::ReadFileToString(pb_path, &binary_pb)) {
    // The file won't exist on new installations, so this is not always an
    // error.
    LOG(INFO) << "Failed reading from " << pb_path.value();
    return;
  }

  // Notify UMA that the download succeeded?
  // TODO(digit): Implement this?
  LOG(INFO) << "Success !!   XXXYYYWWW";
}

}  // namespace

namespace component_updater {

bool DownloadableStringsComponentInstallerPolicy::
    SupportsGroupPolicyEnabledComponentUpdates() const {
  return false;
}

bool DownloadableStringsComponentInstallerPolicy::RequiresNetworkEncryption()
    const {
  return false;
}

update_client::CrxInstaller::Result
DownloadableStringsComponentInstallerPolicy::OnCustomInstall(
    const base::DictionaryValue& manifest,
    const base::FilePath& install_dir) {
  return update_client::CrxInstaller::Result(0);  // Nothing custom here.
}

base::FilePath DownloadableStringsComponentInstallerPolicy::GetInstalledPath(
    const base::FilePath& base) {
  return base.Append(kDownloadableStringsBinaryTestFileName);
}

void DownloadableStringsComponentInstallerPolicy::ComponentReady(
    const base::Version& version,
    const base::FilePath& install_dir,
    std::unique_ptr<base::DictionaryValue> manifest) {
  LOG(INFO) << "Component ready, version " << version.GetString() << " in "
            << install_dir.value();

  base::PostTaskWithTraits(
      FROM_HERE, {base::MayBlock(), base::TaskPriority::BACKGROUND},
      base::BindOnce(&LoadFileTypesFromDisk, GetInstalledPath(install_dir)));
}

// Called during startup and installation before ComponentReady().
bool DownloadableStringsComponentInstallerPolicy::VerifyInstallation(
    const base::DictionaryValue& manifest,
    const base::FilePath& install_dir) const {
  // No need to actually validate the file here.
  return base::PathExists(GetInstalledPath(install_dir));
}

base::FilePath
DownloadableStringsComponentInstallerPolicy::GetRelativeInstallDir() const {
  return base::FilePath(FILE_PATH_LITERAL("DownloadableStrings"));
}

void DownloadableStringsComponentInstallerPolicy::GetHash(
    std::vector<uint8_t>* hash) const {
  hash->assign(kPublicKeySHA256,
               kPublicKeySHA256 + arraysize(kPublicKeySHA256));
}

std::string DownloadableStringsComponentInstallerPolicy::GetName() const {
  return kDownloadableStringsManifestName;
}

update_client::InstallerAttributes
DownloadableStringsComponentInstallerPolicy::GetInstallerAttributes() const {
  return update_client::InstallerAttributes();
}

std::vector<std::string>
DownloadableStringsComponentInstallerPolicy::GetMimeTypes() const {
  return {};
}

static void ReportError(update_client::Error error) {
  LOG(INFO) << "Error: " << static_cast<int>(error);
}

// static
void DownloadableStringsComponentInstallerPolicy::TriggerComponentUpdate(
    OnDemandUpdater* updater,
    const std::string& crx_id) {
  // TODO(digit): Check result with a real callback.
  LOG(INFO) << "Triggering component update";
  updater->OnDemandUpdate(crx_id, base::Bind(&ReportError));
}

void RegisterDownloadableStringsComponent(ComponentUpdateService* cus) {
  LOG(INFO) << "Registering Downloadable Strings component.";

  auto policy = base::MakeUnique<DownloadableStringsComponentInstallerPolicy>();
  // |cus| will take ownership of |installer| during installer->Register(cus).
  ComponentInstaller* installer = new ComponentInstaller(std::move(policy));
  installer->Register(
      cus,
      base::Bind(
          &DownloadableStringsComponentInstallerPolicy::TriggerComponentUpdate,
          &cus->GetOnDemandUpdater(), std::string(kCrxId)));
}

}  // namespace component_updater
