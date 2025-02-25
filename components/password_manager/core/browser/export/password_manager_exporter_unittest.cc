// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/password_manager/core/browser/export/password_manager_exporter.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/strings/utf_string_conversions.h"
#include "base/test/histogram_tester.h"
#include "base/test/mock_callback.h"
#include "base/test/scoped_task_environment.h"
#include "build/build_config.h"
#include "components/autofill/core/common/password_form.h"
#include "components/password_manager/core/browser/export/password_csv_writer.h"
#include "components/password_manager/core/browser/password_manager_metrics_util.h"
#include "components/password_manager/core/browser/ui/credential_provider_interface.h"
#include "components/password_manager/core/browser/ui/export_progress_status.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {

using ::testing::_;
using ::testing::IsEmpty;
using ::testing::Not;
using ::testing::Return;
using ::testing::ReturnArg;
using ::testing::SaveArg;
using ::testing::StrEq;
using ::testing::StrictMock;

using password_manager::metrics_util::ExportPasswordsResult;

// A callback that matches the signature of base::WriteFile
using WriteCallback =
    base::RepeatingCallback<int(const base::FilePath&, const char*, int)>;

#if defined(OS_WIN)
const base::FilePath::CharType kNullFileName[] = FILE_PATH_LITERAL("/nul");
#else
const base::FilePath::CharType kNullFileName[] = FILE_PATH_LITERAL("/dev/null");
#endif

// Provides a predetermined set of credentials
class FakeCredentialProvider
    : public password_manager::CredentialProviderInterface {
 public:
  FakeCredentialProvider() {}

  void SetPasswordList(
      const std::vector<std::unique_ptr<autofill::PasswordForm>>&
          password_list) {
    password_list_.clear();
    for (const auto& form : password_list) {
      password_list_.push_back(std::make_unique<autofill::PasswordForm>(*form));
    }
  }

  // password_manager::CredentialProviderInterface:
  std::vector<std::unique_ptr<autofill::PasswordForm>> GetAllPasswords()
      override {
    std::vector<std::unique_ptr<autofill::PasswordForm>> ret_val;
    for (const auto& form : password_list_) {
      ret_val.push_back(std::make_unique<autofill::PasswordForm>(*form));
    }
    return ret_val;
  }

 private:
  std::vector<std::unique_ptr<autofill::PasswordForm>> password_list_;

  DISALLOW_COPY_AND_ASSIGN(FakeCredentialProvider);
};

// WriteFunction will delegate to this callback, if set. Use for setting
// expectations for base::WriteFile in PasswordManagerExporter.
base::MockCallback<WriteCallback>* g_write_callback = nullptr;

// Mock for base::WriteFile. Expectations should be set on |g_write_callback|.
int WriteFunction(const base::FilePath& filename, const char* data, int size) {
  if (g_write_callback)
    return g_write_callback->Get().Run(filename, data, size);
  return size;
}

// Creates a hardcoded set of credentials for tests.
std::vector<std::unique_ptr<autofill::PasswordForm>> CreatePasswordList() {
  auto password_form = std::make_unique<autofill::PasswordForm>();
  password_form->origin = GURL("http://accounts.google.com/a/LoginAuth");
  password_form->username_value = base::ASCIIToUTF16("test@gmail.com");
  password_form->password_value = base::ASCIIToUTF16("test1");

  std::vector<std::unique_ptr<autofill::PasswordForm>> password_forms;
  password_forms.push_back(std::move(password_form));
  return password_forms;
}

class PasswordManagerExporterTest : public testing::Test {
 public:
  PasswordManagerExporterTest()
      : scoped_task_environment_(
            base::test::ScopedTaskEnvironment::MainThreadType::UI),
        exporter_(&fake_credential_provider_, mock_on_progress_.Get()),
        destination_path_(kNullFileName) {
    g_write_callback = &mock_write_file_;
    exporter_.SetWriteForTesting(&WriteFunction);
  }

  ~PasswordManagerExporterTest() override { g_write_callback = nullptr; }

 protected:
  base::test::ScopedTaskEnvironment scoped_task_environment_;
  FakeCredentialProvider fake_credential_provider_;
  base::MockCallback<base::RepeatingCallback<
      void(password_manager::ExportProgressStatus, const std::string&)>>
      mock_on_progress_;
  password_manager::PasswordManagerExporter exporter_;
  StrictMock<base::MockCallback<WriteCallback>> mock_write_file_;
  base::FilePath destination_path_;
  base::HistogramTester histogram_tester_;

 private:
  DISALLOW_COPY_AND_ASSIGN(PasswordManagerExporterTest);
};

TEST_F(PasswordManagerExporterTest, PasswordExportSetPasswordListFirst) {
  std::vector<std::unique_ptr<autofill::PasswordForm>> password_list =
      CreatePasswordList();
  fake_credential_provider_.SetPasswordList(password_list);
  const std::string serialised(
      password_manager::PasswordCSVWriter::SerializePasswords(password_list));

  EXPECT_CALL(mock_write_file_,
              Run(destination_path_, StrEq(serialised), serialised.size()))
      .WillOnce(ReturnArg<2>());
  EXPECT_CALL(
      mock_on_progress_,
      Run(password_manager::ExportProgressStatus::IN_PROGRESS, IsEmpty()));
  EXPECT_CALL(
      mock_on_progress_,
      Run(password_manager::ExportProgressStatus::SUCCEEDED, IsEmpty()));

  exporter_.PreparePasswordsForExport();
  exporter_.SetDestination(destination_path_);

  scoped_task_environment_.RunUntilIdle();
  histogram_tester_.ExpectUniqueSample(
      "PasswordManager.ExportedPasswordsPerUserInCSV", password_list.size(), 1);
  histogram_tester_.ExpectTotalCount(
      "PasswordManager.TimeReadingExportedPasswords", 1);
  histogram_tester_.ExpectUniqueSample(
      "PasswordManager.ExportPasswordsToCSVResult",
      ExportPasswordsResult::SUCCESS, 1);
}

TEST_F(PasswordManagerExporterTest, PasswordExportSetDestinationFirst) {
  std::vector<std::unique_ptr<autofill::PasswordForm>> password_list =
      CreatePasswordList();
  fake_credential_provider_.SetPasswordList(password_list);
  const std::string serialised(
      password_manager::PasswordCSVWriter::SerializePasswords(password_list));

  EXPECT_CALL(mock_write_file_,
              Run(destination_path_, StrEq(serialised), serialised.size()))
      .WillOnce(ReturnArg<2>());
  EXPECT_CALL(
      mock_on_progress_,
      Run(password_manager::ExportProgressStatus::IN_PROGRESS, IsEmpty()));
  EXPECT_CALL(
      mock_on_progress_,
      Run(password_manager::ExportProgressStatus::SUCCEEDED, IsEmpty()));

  exporter_.SetDestination(destination_path_);
  exporter_.PreparePasswordsForExport();

  scoped_task_environment_.RunUntilIdle();
  histogram_tester_.ExpectUniqueSample(
      "PasswordManager.ExportedPasswordsPerUserInCSV", password_list.size(), 1);
  histogram_tester_.ExpectTotalCount(
      "PasswordManager.TimeReadingExportedPasswords", 1);
  histogram_tester_.ExpectUniqueSample(
      "PasswordManager.ExportPasswordsToCSVResult",
      ExportPasswordsResult::SUCCESS, 1);
}

TEST_F(PasswordManagerExporterTest, WriteFileFailed) {
  std::vector<std::unique_ptr<autofill::PasswordForm>> password_list =
      CreatePasswordList();
  fake_credential_provider_.SetPasswordList(password_list);
  const std::string destination_folder_name(
      destination_path_.DirName().BaseName().AsUTF8Unsafe());

  EXPECT_CALL(mock_write_file_, Run(_, _, _)).WillOnce(Return(-1));
  EXPECT_CALL(
      mock_on_progress_,
      Run(password_manager::ExportProgressStatus::IN_PROGRESS, IsEmpty()));
  EXPECT_CALL(mock_on_progress_,
              Run(password_manager::ExportProgressStatus::FAILED_WRITE_FAILED,
                  StrEq(destination_folder_name)));

  exporter_.SetDestination(destination_path_);
  exporter_.PreparePasswordsForExport();

  scoped_task_environment_.RunUntilIdle();
  histogram_tester_.ExpectTotalCount(
      "PasswordManager.ExportedPasswordsPerUserInCSV", 0);
  histogram_tester_.ExpectUniqueSample(
      "PasswordManager.ExportPasswordsToCSVResult",
      ExportPasswordsResult::WRITE_FAILED, 1);
}

// Test that GetProgressStatus() returns the last ExportProgressStatus sent
// to the callback.
TEST_F(PasswordManagerExporterTest, GetProgressReturnsLastCallbackStatus) {
  std::vector<std::unique_ptr<autofill::PasswordForm>> password_list =
      CreatePasswordList();
  fake_credential_provider_.SetPasswordList(password_list);
  const std::string serialised(
      password_manager::PasswordCSVWriter::SerializePasswords(password_list));
  const std::string destination_folder_name(
      destination_path_.DirName().BaseName().AsUTF8Unsafe());

  // The last status seen in the callback.
  password_manager::ExportProgressStatus status =
      password_manager::ExportProgressStatus::NOT_STARTED;

  EXPECT_CALL(mock_write_file_, Run(_, _, _)).WillOnce(ReturnArg<2>());
  EXPECT_CALL(mock_on_progress_, Run(_, _)).WillRepeatedly(SaveArg<0>(&status));

  ASSERT_EQ(exporter_.GetProgressStatus(), status);
  exporter_.SetDestination(destination_path_);
  exporter_.PreparePasswordsForExport();
  ASSERT_EQ(exporter_.GetProgressStatus(), status);

  scoped_task_environment_.RunUntilIdle();
  ASSERT_EQ(exporter_.GetProgressStatus(), status);
}

TEST_F(PasswordManagerExporterTest, DontExportWithOnlyDestination) {
  std::vector<std::unique_ptr<autofill::PasswordForm>> password_list =
      CreatePasswordList();
  fake_credential_provider_.SetPasswordList(password_list);

  EXPECT_CALL(mock_write_file_, Run(_, _, _)).Times(0);
  EXPECT_CALL(
      mock_on_progress_,
      Run(password_manager::ExportProgressStatus::IN_PROGRESS, IsEmpty()));

  exporter_.SetDestination(destination_path_);

  scoped_task_environment_.RunUntilIdle();
  histogram_tester_.ExpectTotalCount(
      "PasswordManager.ExportedPasswordsPerUserInCSV", 0);
  histogram_tester_.ExpectTotalCount(
      "PasswordManager.TimeReadingExportedPasswords", 0);
  histogram_tester_.ExpectTotalCount(
      "PasswordManager.ExportPasswordsToCSVResult", 0);
}

TEST_F(PasswordManagerExporterTest, CancelAfterPasswords) {
  std::vector<std::unique_ptr<autofill::PasswordForm>> password_list =
      CreatePasswordList();
  fake_credential_provider_.SetPasswordList(password_list);

  EXPECT_CALL(mock_write_file_, Run(_, _, _)).Times(0);
  EXPECT_CALL(
      mock_on_progress_,
      Run(password_manager::ExportProgressStatus::FAILED_CANCELLED, IsEmpty()));
  EXPECT_CALL(
      mock_on_progress_,
      Run(password_manager::ExportProgressStatus::IN_PROGRESS, IsEmpty()));

  exporter_.PreparePasswordsForExport();
  exporter_.Cancel();
  exporter_.SetDestination(destination_path_);

  scoped_task_environment_.RunUntilIdle();
  histogram_tester_.ExpectTotalCount(
      "PasswordManager.ExportedPasswordsPerUserInCSV", 0);
  histogram_tester_.ExpectUniqueSample(
      "PasswordManager.ExportPasswordsToCSVResult",
      ExportPasswordsResult::USER_ABORTED, 1);
}

TEST_F(PasswordManagerExporterTest, CancelAfterDestination) {
  std::vector<std::unique_ptr<autofill::PasswordForm>> password_list =
      CreatePasswordList();
  fake_credential_provider_.SetPasswordList(password_list);

  EXPECT_CALL(mock_write_file_, Run(_, _, _)).Times(0);
  EXPECT_CALL(
      mock_on_progress_,
      Run(password_manager::ExportProgressStatus::IN_PROGRESS, IsEmpty()));
  EXPECT_CALL(
      mock_on_progress_,
      Run(password_manager::ExportProgressStatus::FAILED_CANCELLED, IsEmpty()));

  exporter_.SetDestination(destination_path_);
  exporter_.Cancel();
  exporter_.PreparePasswordsForExport();

  scoped_task_environment_.RunUntilIdle();
  histogram_tester_.ExpectTotalCount(
      "PasswordManager.ExportedPasswordsPerUserInCSV", 0);
  histogram_tester_.ExpectUniqueSample(
      "PasswordManager.ExportPasswordsToCSVResult",
      ExportPasswordsResult::USER_ABORTED, 1);
}

// Test that PasswordManagerExporter is reusable, after an export has been
// cancelled.
TEST_F(PasswordManagerExporterTest, CancelAfterPasswordsThenExport) {
  std::vector<std::unique_ptr<autofill::PasswordForm>> password_list =
      CreatePasswordList();
  const std::string serialised(
      password_manager::PasswordCSVWriter::SerializePasswords(password_list));
  fake_credential_provider_.SetPasswordList(password_list);

  EXPECT_CALL(mock_write_file_,
              Run(destination_path_, StrEq(serialised), serialised.size()))
      .WillOnce(ReturnArg<2>());
  EXPECT_CALL(
      mock_on_progress_,
      Run(password_manager::ExportProgressStatus::FAILED_CANCELLED, IsEmpty()));
  EXPECT_CALL(
      mock_on_progress_,
      Run(password_manager::ExportProgressStatus::IN_PROGRESS, IsEmpty()));
  EXPECT_CALL(
      mock_on_progress_,
      Run(password_manager::ExportProgressStatus::SUCCEEDED, IsEmpty()));

  exporter_.PreparePasswordsForExport();
  exporter_.Cancel();
  exporter_.SetDestination(destination_path_);
  exporter_.PreparePasswordsForExport();

  scoped_task_environment_.RunUntilIdle();
  histogram_tester_.ExpectUniqueSample(
      "PasswordManager.ExportedPasswordsPerUserInCSV", password_list.size(), 1);
}

// Test that PasswordManagerExporter is reusable, after an export has been
// cancelled.
TEST_F(PasswordManagerExporterTest, CancelAfterDestinationThenExport) {
  std::vector<std::unique_ptr<autofill::PasswordForm>> password_list =
      CreatePasswordList();
  const std::string serialised(
      password_manager::PasswordCSVWriter::SerializePasswords(password_list));
  fake_credential_provider_.SetPasswordList(password_list);

  base::FilePath cancelled_path(FILE_PATH_LITERAL("clean_me_up"));

  EXPECT_CALL(mock_write_file_, Run(cancelled_path, _, _)).Times(0);
  EXPECT_CALL(mock_write_file_,
              Run(destination_path_, StrEq(serialised), serialised.size()))
      .WillOnce(ReturnArg<2>());
  EXPECT_CALL(
      mock_on_progress_,
      Run(password_manager::ExportProgressStatus::FAILED_CANCELLED, IsEmpty()));
  EXPECT_CALL(
      mock_on_progress_,
      Run(password_manager::ExportProgressStatus::IN_PROGRESS, IsEmpty()))
      .Times(2);
  EXPECT_CALL(
      mock_on_progress_,
      Run(password_manager::ExportProgressStatus::SUCCEEDED, IsEmpty()));

  exporter_.SetDestination(std::move(cancelled_path));
  exporter_.Cancel();
  exporter_.PreparePasswordsForExport();
  exporter_.SetDestination(destination_path_);

  scoped_task_environment_.RunUntilIdle();
  histogram_tester_.ExpectUniqueSample(
      "PasswordManager.ExportedPasswordsPerUserInCSV", password_list.size(), 1);
}

}  // namespace
