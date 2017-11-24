// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/password_manager/core/browser/export/password_manager_exporter.h"

#include <utility>

#include "base/location.h"
#include "base/metrics/histogram_macros.h"
#include "base/task_runner_util.h"
#include "base/task_scheduler/post_task.h"
#include "components/autofill/core/common/password_form.h"
#include "components/password_manager/core/browser/export/password_csv_writer.h"
#include "components/password_manager/core/browser/ui/credential_provider_interface.h"

namespace {

std::vector<std::unique_ptr<autofill::PasswordForm>> CopyOf(
    const std::vector<std::unique_ptr<autofill::PasswordForm>>& password_list) {
  std::vector<std::unique_ptr<autofill::PasswordForm>> ret_val;
  for (const auto& form : password_list) {
    ret_val.push_back(std::make_unique<autofill::PasswordForm>(*form));
  }
  return ret_val;
}

}  // namespace

namespace password_manager {

PasswordManagerExporter::PasswordManagerExporter(
    password_manager::CredentialProviderInterface*
        credential_provider_interface,
    PasswordUIExportView* password_ui_export_view)
    : credential_provider_interface_(credential_provider_interface),
      password_ui_export_view_(password_ui_export_view),
      task_runner_(base::CreateSequencedTaskRunnerWithTraits(
          {base::TaskPriority::USER_VISIBLE, base::MayBlock()})),
      weak_factory_(this) {}

PasswordManagerExporter::~PasswordManagerExporter() {}

void PasswordManagerExporter::PreparePasswordsForExport() {
  std::vector<std::unique_ptr<autofill::PasswordForm>> password_list =
      credential_provider_interface_->GetAllPasswords();
  size_t password_list_size = password_list.size();

  base::PostTaskAndReplyWithResult(
      task_runner_.get(), FROM_HERE,
      base::BindOnce(&password_manager::PasswordCSVWriter::SerializePasswords,
                     base::Passed(CopyOf(password_list))),
      base::BindOnce(&PasswordManagerExporter::SetSerialisedPasswordList,
                     weak_factory_.GetWeakPtr(), password_list_size));
}

void PasswordManagerExporter::SetSerialisedPasswordList(
    size_t count,
    const std::string& serialised) {
  serialised_password_list_ = serialised;
  password_list_size_ = count;

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
  // Tasks which had their pointers invalidated won't run.
  weak_factory_.InvalidateWeakPtrs();

  destination_.reset();
  serialised_password_list_.clear();
}

bool PasswordManagerExporter::IsReadyForExport() {
  return destination_ && !serialised_password_list_.empty();
}

void PasswordManagerExporter::Export() {
  UMA_HISTOGRAM_COUNTS("PasswordManager.ExportedPasswordsPerUserInCSV",
                       password_list_size_);

  base::PostTaskAndReplyWithResult(
      task_runner_.get(), FROM_HERE,
      base::BindOnce(&Destination::Write, std::move(destination_),
                     std::move(serialised_password_list_)),
      base::BindOnce(&PasswordManagerExporter::OnPasswordsExported,
                     weak_factory_.GetWeakPtr()));

  serialised_password_list_.clear();
  password_list_size_ = 0;
  destination_.reset();
}

void PasswordManagerExporter::OnPasswordsExported(bool success) {
  password_ui_export_view_->OnCompletedWritingToDestination(
      success ? std::string() : std::string("Writing to destination failed."));
}

}  // namespace password_manager
