// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_COMPONENT_UPDATER_PEPPER_FLASH_COMPONENT_INSTALLER_H_
#define CHROME_BROWSER_COMPONENT_UPDATER_PEPPER_FLASH_COMPONENT_INSTALLER_H_

namespace component_updater {

class ComponentUpdateService;

// Our job is to 1) find what Pepper flash is installed (if any) and 2) register
// with the component updater to download the latest version when available.
// The first part is IO intensive so we do it asynchronously in the file thread.
void RegisterPepperFlashComponent(ComponentUpdateService* cus);

#if defined(GOOGLE_CHROME_BUILD)
class FlashComponentInstallerPolicy : public ComponentInstallerPolicy {
 public:
  FlashComponentInstallerPolicy();
  ~FlashComponentInstallerPolicy() override {}

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

  DISALLOW_COPY_AND_ASSIGN(FlashComponentInstallerPolicy);
};
#endif  // defined(GOOGLE_CHROME_BUILD)

}  // namespace component_updater

#endif  // CHROME_BROWSER_COMPONENT_UPDATER_PEPPER_FLASH_COMPONENT_INSTALLER_H_
