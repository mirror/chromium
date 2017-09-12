// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_PASSWORDS_PASSWORD_MANAGER_PORTER_H_
#define CHROME_BROWSER_UI_PASSWORDS_PASSWORD_MANAGER_PORTER_H_

#include "chrome/browser/profiles/profile.h"
#include "components/password_manager/core/browser/import/password_importer.h"
#include "ui/shell_dialogs/select_file_dialog.h"

namespace content {
class WebContents;
}

class PasswordManagerPresenter;

class PasswordManagerPorter : public ui::SelectFileDialog::Listener {
 public:
  enum Type {
    PASSWORD_IMPORT,
    PASSWORD_EXPORT,
  };

  explicit PasswordManagerPorter(
      PasswordManagerPresenter* password_manager_presenter);

  ~PasswordManagerPorter() override;

  // Display the file-picker dialogue for either importing or exporting
  // passwords.
  void PresentFileSelector(content::WebContents* web_contents, Type type);

 private:
  // A helper class for reading the passwords that have been imported.
  class PasswordImportConsumer
      : public base::RefCountedThreadSafe<PasswordImportConsumer> {
   public:
    explicit PasswordImportConsumer(Profile* profile);

    void ConsumePassword(password_manager::PasswordImporter::Result result,
                         const std::vector<autofill::PasswordForm>& forms);

   private:
    friend class base::RefCountedThreadSafe<PasswordImportConsumer>;

    ~PasswordImportConsumer() {}

    Profile* profile_;
  };

  // Callback from the file selector dialogue when a file has been picked (for
  // either import or export).
  void FileSelected(const base::FilePath& path,
                    int index,
                    void* params) override;

  void ImportPasswordsFromPath(const base::FilePath& path);

  void ExportPasswordsToPath(const base::FilePath& path);

  // The default directory and filename when importing and exporting passwords.
  base::FilePath GetDefaultFilepathForPasswordFile(
      base::FilePath::StringType default_extension);

  PasswordManagerPresenter* password_manager_presenter_;
  content::WebContents* web_contents_;
  scoped_refptr<ui::SelectFileDialog> select_file_dialog_;
};

#endif  // CHROME_BROWSER_UI_PASSWORDS_PASSWORD_MANAGER_PORTER_H_
