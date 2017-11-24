// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_PASSWORD_MANAGER_CORE_BROWSER_EXPORT_PASSWORD_MANAGER_EXPORTER_H_
#define COMPONENTS_PASSWORD_MANAGER_CORE_BROWSER_EXPORT_PASSWORD_MANAGER_EXPORTER_H_

#include <memory>
#include <string>
#include <vector>

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/sequenced_task_runner.h"
#include "components/password_manager/core/browser/export/destination.h"
#include "components/password_manager/core/browser/export/password_ui_export_view.h"

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
          credential_provider_interface,
      PasswordUIExportView* password_ui_export_view);
  virtual ~PasswordManagerExporter();

  // Pre-load the passwords from the password store.
  virtual void PreparePasswordsForExport();

  // Set the destination, where the passwords will be written when they are
  // ready.
  virtual void SetDestination(std::unique_ptr<Destination> destination);

  // Best-effort canceling of any on-going task related to exporting.
  virtual void Cancel();

 private:
  // Caches the serialised password list and proceeds to export, if all the
  // parts are ready.
  void SetSerialisedPasswordList(size_t count, const std::string& serialised);

  // Returns true if all the necessary data is available.
  bool IsReadyForExport();

  // Performs the export. It should not be called before the data is available.
  // At the end, it clears cached fields.
  void Export();

  // Callback after the passwords have been serialised.
  void OnPasswordsExported(bool success);

  // The source of the password list which will be exported.
  CredentialProviderInterface* const credential_provider_interface_;

  // Callbacks to the UI.
  PasswordUIExportView* const password_ui_export_view_;

  // The password list that was read from the store and serialised.
  std::string serialised_password_list_;
  // The number of passwords that were serialised. Useful for metrics.
  size_t password_list_size_;

  // The destination which was provided and where the password list will be
  // sent. It will be cleared once exporting is complete.
  std::unique_ptr<Destination> destination_;

  // |task_runner_| is used for time-consuming tasks during exporting. The tasks
  // will dereference a WeakPtr to |*this|, which means they all need to run on
  // the same sequence.
  scoped_refptr<base::SequencedTaskRunner> task_runner_;
  base::WeakPtrFactory<PasswordManagerExporter> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(PasswordManagerExporter);
};

}  // namespace password_manager

#endif  // COMPONENTS_PASSWORD_MANAGER_CORE_BROWSER_EXPORT_PASSWORD_MANAGER_EXPORTER_H_
