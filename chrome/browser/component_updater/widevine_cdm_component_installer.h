// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_COMPONENT_UPDATER_WIDEVINE_CDM_COMPONENT_INSTALLER_H_
#define CHROME_BROWSER_COMPONENT_UPDATER_WIDEVINE_CDM_COMPONENT_INSTALLER_H_

namespace component_updater {

class ComponentUpdateService;

// Our job is to:
// 1) Find what Widevine CDM is installed (if any).
// 2) Register with the component updater to download the latest version when
//    available.
// 3) Copy the Widevine CDM adapter bundled with chrome to the install path.
// 4) Register the Widevine CDM (via the adapter) with Chrome.
// The first part is IO intensive so we do it asynchronously in the file thread.
void RegisterWidevineCdmComponent(ComponentUpdateService* cus);

class WidevineCdmComponentInstallerPolicy : public ComponentInstallerPolicy {
 public:
  WidevineCdmComponentInstallerPolicy();
  ~WidevineCdmComponentInstallerPolicy() override {}

 private:
  // The following methods override ComponentInstallerPolicy.
  bool SupportsGroupPolicyEnabledComponentUpdates() const override;
  bool RequiresNetworkEncryption() const override;
  update_client::CrxInstaller::Result OnCustomInstall(
      const base::DictionaryValue& manifest,
      const base::FilePath& install_dir) override;
  void OnCustomUninstall() override;
  bool VerifyInstallation(const base::DictionaryValue& manifest,
                          const base::FilePath& install_dir) const override;
  void ComponentReady(const base::Version& version,
                      const base::FilePath& path,
                      std::unique_ptr<base::DictionaryValue> manifest) override;
  base::FilePath GetRelativeInstallDir() const override;
  void GetHash(std::vector<uint8_t>* hash) const override;
  std::string GetName() const override;
  update_client::InstallerAttributes GetInstallerAttributes() const override;
  std::vector<std::string> GetMimeTypes() const override;

  // Checks and updates CDM adapter if necessary to make sure the latest CDM
  // adapter is always used.
  // Note: The component is ready when CDM is present, but the CDM won't be
  // registered until the adapter is copied by this function (see
  // VerifyInstallation).
  void UpdateCdmAdapter(const base::Version& cdm_version,
                        const base::FilePath& cdm_install_dir,
                        std::unique_ptr<base::DictionaryValue> manifest);

  DISALLOW_COPY_AND_ASSIGN(WidevineCdmComponentInstallerPolicy);
};

}  // namespace component_updater

#endif  // CHROME_BROWSER_COMPONENT_UPDATER_WIDEVINE_CDM_COMPONENT_INSTALLER_H_
