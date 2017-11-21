// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/password_manager/core/browser/export/password_manager_exporter.h"

#include <utility>

#include "base/location.h"
#include "base/metrics/histogram_macros.h"
#include "base/task_scheduler/post_task.h"
#include "components/autofill/core/common/password_form.h"
#include "components/password_manager/core/browser/export/destination.h"
#include "components/password_manager/core/browser/export/password_csv_writer.h"
#include "components/password_manager/core/browser/ui/credential_provider_interface.h"

namespace password_manager {

PasswordManagerExporter::PasswordManagerExporter(
    password_manager::CredentialProviderInterface*
        credential_provider_interface)
    : credential_provider_interface_(credential_provider_interface) {}

PasswordManagerExporter::~PasswordManagerExporter() {}

void PasswordManagerExporter::PreparePasswordsForExport() {
  // TODO(cfroussios) async?
  password_list_ = credential_provider_interface_->GetAllPasswords();

  if (ReadyForExport())
    Export();
}

void PasswordManagerExporter::SetDestination(
    std::unique_ptr<Destination> destination) {
  destination_ = std::move(destination);

  if (ReadyForExport())
    Export();
}

void PasswordManagerExporter::Cancel() {
  // TODO(crbug.com/785237) Make exporting cancellable
}

bool PasswordManagerExporter::ReadyForExport() {
  return destination_ && !password_list_.empty();
}

void PasswordManagerExporter::Export() {
  UMA_HISTOGRAM_COUNTS("PasswordManager.ExportedPasswordsPerUserInCSV",
                       password_list_.size());

  // TODO(crbug.com/785237) Make exporting cancellable

  base::PostTaskWithTraits(
      FROM_HERE, {base::TaskPriority::USER_VISIBLE, base::MayBlock()},
      base::BindOnce(
          base::IgnoreResult(&Destination::Write),
          base::Passed(std::move(destination_)),
          base::Passed(password_manager::PasswordCSVWriter::SerializePasswords(
              password_list_))));

  password_list_.clear();
  destination_.reset();
}

}  // namespace password_manager
