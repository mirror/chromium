// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_COMPONENT_UPDATER_CRL_SET_COMPONENT_INSTALLER_H_
#define CHROME_BROWSER_COMPONENT_UPDATER_CRL_SET_COMPONENT_INSTALLER_H_

namespace base {
class FilePath;
}  // namespace base

namespace component_updater {

class ComponentUpdateService;

void RegisterCRLSetComponent(ComponentUpdateService* cus,
                             const base::FilePath& user_data_dir);

class CRLSetPolicy : public ComponentInstallerPolicy {
 public:
  CRLSetPolicy();
  ~CRLSetPolicy() override;

 private:
  // ComponentInstallerPolicy implementation.
  bool SupportsGroupPolicyEnabledComponentUpdates() const override;
  bool RequiresNetworkEncryption() const override;
  update_client::CrxInstaller::Result OnCustomInstall(
      const base::DictionaryValue& manifest,
      const base::FilePath& install_dir) override;
  void OnCustomUninstall() override;
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

  DISALLOW_COPY_AND_ASSIGN(CRLSetPolicy);
};

}  // namespace component_updater

#endif  // CHROME_BROWSER_COMPONENT_UPDATER_CRL_SET_COMPONENT_INSTALLER_H_
