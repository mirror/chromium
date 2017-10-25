// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_PASSWORDS_PASSWORD_MANAGER_PORTER_H_
#define CHROME_BROWSER_UI_PASSWORDS_PASSWORD_MANAGER_PORTER_H_

#include <memory>
#include <vector>

#include "base/memory/weak_ptr.h"
#include "components/password_manager/core/browser/import/password_importer.h"
#include "ui/shell_dialogs/select_file_dialog.h"

namespace content {
class WebContents;
}

class CredentialProviderInterface;
class PasswordUIView;
class Profile;

// Handles the exporting of passwords to a file, and the importing of such a
// file to the Password Manager.
class PasswordManagerPorter : public ui::SelectFileDialog::Listener {
 public:
  enum Type {
    PASSWORD_IMPORT,
    PASSWORD_EXPORT,
  };

  explicit PasswordManagerPorter(
      CredentialProviderInterface* credential_provider_interface,
      PasswordUIView* password_view);

  ~PasswordManagerPorter() override;

  // Display the file-picker dialogue for either importing or exporting
  // passwords.
  void PresentFileSelector(content::WebContents* web_contents, Type type);

  // Read from the password store the passwords that can be exported.
  void ReadPasswordsForExport();

 private:
  // Callback from the file selector dialogue when a file has been picked (for
  // either import or export).
  // ui::SelectFileDialog::Listener:
  void FileSelected(const base::FilePath& path,
                    int index,
                    void* params) override;

  virtual void ImportPasswordsFromPath(const base::FilePath& path);

  virtual void ExportPasswordsToPath(const base::FilePath& path);

  virtual void ExportReply(bool result);

  CredentialProviderInterface* credential_provider_interface_;
  scoped_refptr<ui::SelectFileDialog> select_file_dialog_;
  Profile* profile_;

  // UI view that owns this presenter.
  PasswordUIView* password_view_;

  // The passwords that were read for export and waiting for user confirmation.
  std::vector<std::unique_ptr<autofill::PasswordForm>> password_list_;

  base::WeakPtrFactory<PasswordManagerPorter> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(PasswordManagerPorter);
};

#endif  // CHROME_BROWSER_UI_PASSWORDS_PASSWORD_MANAGER_PORTER_H_
