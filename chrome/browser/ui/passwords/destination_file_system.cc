// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/passwords/destination_file_system.h"

#include <utility>

#include "base/files/file_util.h"
#include "base/strings/utf_string_conversions.h"
#include "components/strings/grit/components_strings.h"
#include "ui/base/l10n/l10n_util.h"

DestinationFileSystem::DestinationFileSystem(base::FilePath destination_path)
    : destination_path_(std::move(destination_path)) {}

base::string16 DestinationFileSystem::Write(const std::string& data) {
  if (base::WriteFile(destination_path_, data.c_str(), data.size()))
    return base::string16();
  else
    return l10n_util::GetStringFUTF16(
        IDS_PASSWORD_MANAGER_EXPORT_TO_FILE_FAILED,
        base::UTF8ToUTF16(destination_path_.DirName().BaseName().value()));
}

const base::FilePath& DestinationFileSystem::GetDestinationPathForTesting() {
  return destination_path_;
}
