// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/files/file.h"
#include "base/files/memory_mapped_file.h"
#include "base/logging.h"
#include "chrome/installer/zucchini/buffer_view.h"
#include "chrome/installer/zucchini/patch_reader.h"
#include "chrome/installer/zucchini/zucchini.h"

namespace zucchini {

status::Code Apply(const base::FilePath& old_path,
                   const base::FilePath& patch_path,
                   const base::FilePath& new_path) {
  base::MemoryMappedFile patch_file;
  if (!patch_file.Initialize(patch_path)) {
    LOG(ERROR) << "Can't read file: " << patch_path.value();
    return status::kStatusFileReadError;
  }
  zucchini::ConstBufferView patch_buffer(patch_file.data(),
                                         patch_file.length());

  auto patch_reader = zucchini::EnsemblePatchReader::Create(patch_buffer);
  if (!patch_reader.has_value()) {
    LOG(ERROR) << "Error reading patch header.";
    return status::kStatusPatchReadError;
  }

  base::MemoryMappedFile old_file;
  if (!old_file.Initialize(old_path)) {
    LOG(ERROR) << "Can't read file: " << old_path.value();
    return status::kStatusFileReadError;
  }
  zucchini::ConstBufferView old_buffer(old_file.data(), old_file.length());
  if (!patch_reader->CheckOldFile(old_buffer)) {
    LOG(ERROR) << "Invalid old_file.";
    return status::kStatusInvalidOldImage;
  }

  zucchini::PatchHeader header = patch_reader->header();
  base::MemoryMappedFile new_file;
  if (!new_file.Initialize(base::File(new_path, base::File::FLAG_CREATE_ALWAYS |
                                                    base::File::FLAG_READ |
                                                    base::File::FLAG_WRITE),
                           {0, static_cast<int64_t>(header.new_size)},
                           base::MemoryMappedFile::READ_WRITE_EXTEND)) {
    LOG(ERROR) << "Can't create file: " << new_path.value();
    return status::kStatusFileReadError;
  }
  zucchini::MutableBufferView new_buffer(new_file.data(), new_file.length());

  zucchini::status::Code result =
      zucchini::Apply(old_buffer, *patch_reader, new_buffer);
  if (result != status::kStatusSuccess)
    LOG(ERROR) << "Fatal error encountered while applying patch.";
  return result;
}

}  // namespace zucchini
