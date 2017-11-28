// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_COMPONENT_UPDATER_PNACL_COMPONENT_INSTALLER_H_
#define CHROME_BROWSER_COMPONENT_UPDATER_PNACL_COMPONENT_INSTALLER_H_

namespace pnacl {
// Returns true if PNaCl actually needs an on-demand component update.
// E.g., if PNaCl is not yet installed and the user is loading a PNaCl app,
// or the current version is behind chrome's version, and is ABI incompatible
// with chrome. If not necessary, returns false.
// May conservatively return false before PNaCl is registered, but
// should return the right answer after it is registered.
bool NeedsOnDemandUpdate();
}

namespace component_updater {

class ComponentUpdateService;

void RegisterPnaclComponent(ComponentUpdateService* cus);

class PnaclComponentInstallerPolicy : public ComponentInstallerPolicy {
 public:
  PnaclComponentInstallerPolicy();
  ~PnaclComponentInstallerPolicy() override;

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

  DISALLOW_COPY_AND_ASSIGN(PnaclComponentInstallerPolicy);
};

}  // namespace component_updater

#endif  // CHROME_BROWSER_COMPONENT_UPDATER_PNACL_COMPONENT_INSTALLER_H_
