// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/password_manager/core/browser/export/password_exporter.h"

#include <memory>

#include "base/files/file_util.h"
#include "components/autofill/core/common/password_form.h"
#include "components/password_manager/core/browser/export/password_csv_writer.h"

namespace password_manager {

namespace {

const base::FilePath::CharType kFileExtension[] = FILE_PATH_LITERAL("csv");

}  // namespace

// static
bool PasswordExporter::Export(
    const base::FilePath& path,
    const std::vector<std::unique_ptr<autofill::PasswordForm>>& passwords) {
  std::string data(PasswordCSVWriter::SerializePasswords(passwords));
  return base::WriteFile(path, data.c_str(), data.size()) > 0;
}

// static
std::vector<std::vector<base::FilePath::StringType>>
PasswordExporter::GetSupportedFileExtensions() {
  return std::vector<std::vector<base::FilePath::StringType>>(
      1, std::vector<base::FilePath::StringType>(1, kFileExtension));
}

}  // namespace password_manager
