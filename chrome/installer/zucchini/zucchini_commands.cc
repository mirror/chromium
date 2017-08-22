// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/installer/zucchini/zucchini_commands.h"

#include <stddef.h>
#include <stdint.h>

#include <iostream>
#include <ostream>

#include "base/command_line.h"
#include "base/files/file.h"
#include "base/files/file_path.h"
#include "base/files/memory_mapped_file.h"
#include "base/logging.h"
#include "base/macros.h"
#include "chrome/installer/zucchini/buffer_view.h"
#include "chrome/installer/zucchini/crc32.h"
#include "chrome/installer/zucchini/io_utils.h"
#include "chrome/installer/zucchini/mapped_file.h"
#include "chrome/installer/zucchini/patch_writer.h"
#include "chrome/installer/zucchini/zucchini_integration.h"

namespace {

/******** Command-line Switches ********/

const char kSwitchRaw[] = "raw";

}  // namespace

zucchini::status::Code MainGen(MainParams params) {
  CHECK_EQ(3U, params.file_paths.size());
  zucchini::MappedFileReader old_image(params.file_paths[0]);
  if (!old_image.IsValid())
    return zucchini::status::kStatusFileReadError;
  zucchini::MappedFileReader new_image(params.file_paths[1]);
  if (!new_image.IsValid())
    return zucchini::status::kStatusFileReadError;

  zucchini::EnsemblePatchWriter patch_writer(old_image.region(),
                                             new_image.region());

  auto generate = params.command_line.HasSwitch(kSwitchRaw)
                      ? zucchini::GenerateRaw
                      : zucchini::GenerateEnsemble;
  zucchini::status::Code status =
      generate(old_image.region(), new_image.region(), &patch_writer);
  if (status != zucchini::status::kStatusSuccess) {
    params.out << "Fatal error encountered when generating patch." << std::endl;
    return status;
  }

  zucchini::MappedFileWriter patch(params.file_paths[2],
                                   patch_writer.SerializedSize(), true);
  if (!patch.IsValid())
    return zucchini::status::kStatusFileWriteError;

  if (!patch_writer.SerializeInto(patch.region()))
    return zucchini::status::kStatusPatchWriteError;

  if (!patch.Keep())
    return zucchini::status::kStatusFileWriteError;
  return zucchini::status::kStatusSuccess;
}

zucchini::status::Code MainApply(MainParams params) {
  CHECK_EQ(3U, params.file_paths.size());
  return zucchini::Apply(params.file_paths[0], params.file_paths[1],
                         params.file_paths[2]);
}

zucchini::status::Code MainCrc32(MainParams params) {
  CHECK_EQ(1U, params.file_paths.size());
  zucchini::MappedFileReader image(params.file_paths[0]);
  if (!image.IsValid())
    return zucchini::status::kStatusFileReadError;

  uint32_t crc =
      zucchini::CalculateCrc32(image.data(), image.data() + image.length());
  params.out << "CRC32: " << zucchini::AsHex<8>(crc) << std::endl;
  return zucchini::status::kStatusSuccess;
}
