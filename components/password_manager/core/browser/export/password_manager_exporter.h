// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_PASSWORD_MANAGER_CORE_BROWSER_EXPORT_PASSWORD_MANAGER_EXPORTER_H_
#define COMPONENTS_PASSWORD_MANAGER_CORE_BROWSER_EXPORT_PASSWORD_MANAGER_EXPORTER_H_

#include <memory>
#include <vector>

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/sequenced_task_runner.h"
#include "components/password_manager/core/browser/export/destination.h"

namespace autofill {
struct PasswordForm;
}

namespace password_manager {

class CredentialProviderInterface;
class Destination;

// Controls the exporting of passwords. PasswordManagerExporter will perform
// the export asynchrnously as soon as all the required info is available
// (password list and destination), unless canceled.
class PasswordManagerExporter {
 public:
  explicit PasswordManagerExporter(
      password_manager::CredentialProviderInterface*
          credential_provider_interface);
  virtual ~PasswordManagerExporter();

  // Pre-load the passwords from the password store.
  // TODO(crbug.com/785237) Notify the UI about the result.
  virtual void PreparePasswordsForExport();

  // Set the destination, where the passwords will be written when they are
  // ready.
  virtual void SetDestination(std::unique_ptr<Destination> destination);

  // Best-effort canceling of any on-going task related to exporting.
  virtual void Cancel();

 private:
  // Reads the password store and saves the credentials in |password_list_|.
  void PreparePasswordsForExportInternal();

  // Returns true if all the necessary data is available.
  bool IsReadyForExport();

  // Performs the export. It should not be called before the data is available.
  // At the end, it clears cached fields.
  // TODO(crbug.com/785237) Notify the UI about the result.
  void Export();

  // The source of the password list which will be exported.
  password_manager::CredentialProviderInterface* credential_provider_interface_;

  // The password list that was read from the store. It will be cleared once
  // exporting is complete.
  std::vector<std::unique_ptr<autofill::PasswordForm>> password_list_;

  // The destination which was provided and where the password list will be
  // sent. It will be cleared once exporting is complete.
  std::unique_ptr<Destination> destination_;

  // If true, the next export should not be performed.
  bool abort_;

  scoped_refptr<base::SequencedTaskRunner> task_runner_;
  base::WeakPtrFactory<PasswordManagerExporter> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(PasswordManagerExporter);
};

}  // namespace password_manager

#endif  // COMPONENTS_PASSWORD_MANAGER_CORE_BROWSER_EXPORT_PASSWORD_MANAGER_EXPORTER_H_
