// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/installer/zucchini/zucchini_integration.h"

#include "base/logging.h"
#include "chrome/installer/zucchini/buffer_view.h"
#include "chrome/installer/zucchini/mapped_file.h"
#include "chrome/installer/zucchini/patch_reader.h"

namespace zucchini {

namespace {

struct FileNames {
  FileNames(std::string_view old_name,
            std::string_view patch_name,
            std::string_view new_name,
            bool is_dummy)
      : old_name(old_name),
        patch_name(patch_name),
        new_name(new_name),
        is_dummy(is_dummy) {}

  const std::string old_name;
  const std::string patch_name;
  const std::string new_name;
  const bool is_dummy;
};

status::Code ApplyCommon(base::File&& old_file_handle,
                         base::File&& patch_file_handle,
                         base::File&& new_file_handle,
                         const FileNames& names) {
  MappedFileReader patch_file(std::move(patch_file_handle));
  if (patch_file.HasError()) {
    LOG(ERROR) << "Error with file " << names.patch_name << ": "
               << patch_file.error();
    return status::kStatusFileReadError;
  }

  auto patch_reader =
      zucchini::EnsemblePatchReader::Create(patch_file.region());
  if (!patch_reader.has_value()) {
    LOG(ERROR) << "Error reading patch header.";
    return status::kStatusPatchReadError;
  }

  MappedFileReader old_file(std::move(old_file_handle));
  if (old_file.HasError()) {
    LOG(ERROR) << "Error with file " << names.old_name << ": "
               << old_file.error();
    return status::kStatusFileReadError;
  }
  if (!patch_reader->CheckOldFile(old_file.region())) {
    LOG(ERROR) << "Invalid old_file.";
    return status::kStatusInvalidOldImage;
  }

  zucchini::PatchHeader header = patch_reader->header();
  // By default, delete output on destruction, to avoid having lingering files
  // in case of a failure. On Windows deletion can be done by the OS.
  MappedFileWriter new_file(
      base::FilePath(names.is_dummy ? "" : names.new_name),
      std::move(new_file_handle), header.new_size);
  if (new_file.HasError()) {
    LOG(ERROR) << "Error with file " << names.new_name << ": "
               << new_file.error();
    return status::kStatusFileWriteError;
  }

  zucchini::status::Code result =
      zucchini::Apply(old_file.region(), *patch_reader, new_file.region());
  if (result != status::kStatusSuccess) {
    LOG(ERROR) << "Fatal error encountered while applying patch.";
    return result;
  }

  // Successfully patch |new_file|. Explicitly request file to be kept.
  if (!new_file.Keep())
    return status::kStatusFileWriteError;
  return status::kStatusSuccess;
}

}  // namespace

status::Code Apply(base::File old_file_handle,
                   base::File patch_file_handle,
                   base::File new_file_handle) {
  const FileNames file_names("is_dummyly_opened_old_file",
                             "is_dummyly_opened_patch_file",
                             "is_dummyly_opened_new_file", true);
  return ApplyCommon(std::move(old_file_handle), std::move(patch_file_handle),
                     std::move(new_file_handle), file_names);
}

status::Code Apply(const base::FilePath& old_path,
                   const base::FilePath& patch_path,
                   const base::FilePath& new_path) {
  using base::File;
  File old_file(old_path, File::FLAG_READ);
  File patch_file(patch_path, File::FLAG_READ);
  File new_file(new_path, File::FLAG_CREATE_ALWAYS | File::FLAG_READ |
                              File::FLAG_WRITE | File::FLAG_SHARE_DELETE |
                              File::FLAG_CAN_DELETE_ON_CLOSE);
  const FileNames file_names(old_path.value(), patch_path.value(),
                             new_path.value(), false);
  return ApplyCommon(std::move(old_file), std::move(patch_file),
                     std::move(new_file), file_names);
}

}  // namespace zucchini
