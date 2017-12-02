// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/installer/zucchini/zucchini_commands.h"

#include <stddef.h>
#include <stdint.h>

#include <ostream>
#include <string>

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
#include "chrome/installer/zucchini/patch_reader.h"
#include "chrome/installer/zucchini/patch_writer.h"
#include "chrome/installer/zucchini/zucchini_integration.h"
#include "chrome/installer/zucchini/zucchini_tools.h"

namespace {

/******** Command-line Switches ********/

constexpr char kSwitchDD[] = "dd";
constexpr char kSwitchDump[] = "dump";
constexpr char kSwitchImpose[] = "impose";
constexpr char kSwitchKeep[] = "keep";
constexpr char kSwitchRaw[] = "raw";

}  // namespace

zucchini::status::Code MainGen(MainParams params) {
  CHECK_EQ(3U, params.file_paths.size());

  // TODO(huangs): Move implementation to zucchini_integration.cc.
  zucchini::MappedFileReader old_image(params.file_paths[0]);
  if (!old_image.IsValid())
    return zucchini::status::kStatusFileReadError;
  zucchini::MappedFileReader new_image(params.file_paths[1]);
  if (!new_image.IsValid())
    return zucchini::status::kStatusFileReadError;

  zucchini::EnsemblePatchWriter patch_writer(old_image.region(),
                                             new_image.region());

  zucchini::status::Code result = zucchini::status::kStatusSuccess;
  if (params.command_line.HasSwitch(kSwitchRaw)) {
    result = GenerateRaw(old_image.region(), new_image.region(), &patch_writer);
  } else if (params.command_line.HasSwitch(kSwitchImpose)) {
    std::string imposed_matches =
        params.command_line.GetSwitchValueASCII(kSwitchImpose);
    result = GenerateEnsembleWithImposedMatches(
        old_image.region(), new_image.region(), imposed_matches, &patch_writer);
  } else {
    result =
        GenerateEnsemble(old_image.region(), new_image.region(), &patch_writer);
  }

  if (result != zucchini::status::kStatusSuccess) {
    params.out << "Fatal error encountered when generating patch." << std::endl;
    return result;
  }

  // By default, delete patch on destruction, to avoid having lingering files in
  // case of a failure. On Windows deletion can be done by the OS.
  zucchini::MappedFileWriter patch(params.file_paths[2],
                                   patch_writer.SerializedSize());
  if (!patch.IsValid())
    return zucchini::status::kStatusFileWriteError;

  if (params.command_line.HasSwitch(kSwitchKeep))
    patch.Keep();

  if (!patch_writer.SerializeInto(patch.region()))
    return zucchini::status::kStatusPatchWriteError;

  // Successfully created patch. Explicitly request file to be kept.
  if (!patch.Keep())
    return zucchini::status::kStatusFileWriteError;
  return zucchini::status::kStatusSuccess;
}

zucchini::status::Code MainApply(MainParams params) {
  CHECK_EQ(3U, params.file_paths.size());
  return zucchini::Apply(params.file_paths[0], params.file_paths[1],
                         params.file_paths[2]);
}

zucchini::status::Code MainRead(MainParams params) {
  CHECK_EQ(1U, params.file_paths.size());
  zucchini::MappedFileReader input(params.file_paths[0]);
  if (!input.IsValid())
    return zucchini::status::kStatusFileReadError;

  bool do_dump = params.command_line.HasSwitch(kSwitchDump);
  zucchini::status::Code status = zucchini::ReadReferences(
      {input.data(), input.length()}, do_dump, params.out);
  if (status != zucchini::status::kStatusSuccess)
    params.err << "Fatal error found when dumping references." << std::endl;
  return status;
}

zucchini::status::Code MainDetect(MainParams params) {
  CHECK_EQ(1U, params.file_paths.size());
  zucchini::MappedFileReader input(params.file_paths[0]);
  if (!input.IsValid())
    return zucchini::status::kStatusFileReadError;

  std::vector<zucchini::ConstBufferView> sub_image_list;
  zucchini::status::Code result = zucchini::DetectAll(
      {input.data(), input.length()}, params.out, &sub_image_list);
  if (result != zucchini::status::kStatusSuccess) {
    params.err << "Fatal error found when detecting executables." << std::endl;
    return result;
  }

  std::string fmt = params.command_line.GetSwitchValueASCII(kSwitchDD);
  if (!fmt.empty() && !sub_image_list.empty()) {
    // Count max number of digits, for proper 0-padding to filenames.
    constexpr int kBufSize = 24;
    int digs = 1;  // Note: 10 -> 1 digit, since we'd output "0" to "9".
    for (size_t i = sub_image_list.size(); i > 10; i /= 10)
      ++digs;
    char buf[kBufSize];
    // Print dd commands that can be used to extract detected executables.
    params.out << std::endl;
    params.out << "*** Commands to extract detected files ***" << std::endl;
    for (size_t i = 0; i < sub_image_list.size(); ++i) {
      // Render format filename: Substitute # with |i| rendered to text.
      snprintf(buf, kBufSize, "%0*zu", digs, i);
      std::string filename;
      for (char ch : fmt) {
        if (ch == '#')
          filename += buf;
        else
          filename += ch;
      }
      zucchini::ConstBufferView sub_image = sub_image_list[i];
      size_t skip = sub_image.begin() - input.data();
      size_t count = sub_image.size();
      params.out << "dd bs=1 if=" << params.file_paths[0].value();
      params.out << " skip=" << skip << " count=" << count;
      params.out << " of=" << filename << std::endl;
    }
  }
  return zucchini::status::kStatusSuccess;
}

zucchini::status::Code MainMatch(MainParams params) {
  CHECK_EQ(2U, params.file_paths.size());
  zucchini::MappedFileReader old_image(params.file_paths[0]);
  if (!old_image.IsValid())
    return zucchini::status::kStatusFileReadError;
  zucchini::MappedFileReader new_image(params.file_paths[1]);
  if (!new_image.IsValid())
    return zucchini::status::kStatusFileReadError;

  std::string imposed_matches =
      params.command_line.GetSwitchValueASCII(kSwitchImpose);
  zucchini::status::Code status = zucchini::MatchAll(
      {old_image.data(), old_image.length()},
      {new_image.data(), new_image.length()}, imposed_matches, params.out);
  if (status != zucchini::status::kStatusSuccess)
    params.err << "Fatal error found when matching executables." << std::endl;
  return status;
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
