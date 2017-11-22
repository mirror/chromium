// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/password_manager/core/browser/export/password_manager_exporter.h"

#include <utility>

#include "base/location.h"
#include "base/metrics/histogram_macros.h"
#include "base/task_scheduler/post_task.h"
#include "components/autofill/core/common/password_form.h"
#include "components/password_manager/core/browser/export/password_csv_writer.h"
#include "components/password_manager/core/browser/ui/credential_provider_interface.h"

namespace password_manager {

PasswordManagerExporter::PasswordManagerExporter(
    password_manager::CredentialProviderInterface*
        credential_provider_interface)
    : credential_provider_interface_(credential_provider_interface),
      task_runner_(base::CreateSequencedTaskRunnerWithTraits(
          {base::TaskPriority::USER_VISIBLE, base::MayBlock()})),
      weak_factory_(this) {}

PasswordManagerExporter::~PasswordManagerExporter() {}

void PasswordManagerExporter::PreparePasswordsForExport() {
  abort_ = false;
  task_runner_->PostTask(
      FROM_HERE,
      base::BindOnce(
          &PasswordManagerExporter::PreparePasswordsForExportInternal,
          weak_factory_.GetWeakPtr()));
}

void PasswordManagerExporter::PreparePasswordsForExportInternal() {
  password_list_ = credential_provider_interface_->GetAllPasswords();

  if (abort_) {
    destination_.reset();
    password_list_.clear();
    abort_ = false;
    return;
  }

  if (IsReadyForExport())
    Export();
}

void PasswordManagerExporter::SetDestination(
    std::unique_ptr<Destination> destination) {
  destination_ = std::move(destination);

  if (IsReadyForExport())
    Export();
}

void PasswordManagerExporter::Cancel() {
  destination_.reset();
  password_list_.clear();
  abort_ = true;
}

bool PasswordManagerExporter::IsReadyForExport() {
  return destination_ && !password_list_.empty();
}

void PasswordManagerExporter::Export() {
  UMA_HISTOGRAM_COUNTS("PasswordManager.ExportedPasswordsPerUserInCSV",
                       password_list_.size());

  task_runner_->PostTask(
      FROM_HERE,
      base::BindOnce(
          base::IgnoreResult(&Destination::Write),
          base::Passed(std::move(destination_)),
          base::Passed(password_manager::PasswordCSVWriter::SerializePasswords(
              password_list_))));

  password_list_.clear();
  destination_.reset();
}

}  // namespace password_manager
