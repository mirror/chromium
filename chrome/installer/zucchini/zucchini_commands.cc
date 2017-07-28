// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/installer/zucchini/zucchini_commands.h"

#include <iostream>

#include "base/command_line.h"
#include "base/files/file.h"
#include "base/files/file_path.h"
#include "base/files/memory_mapped_file.h"
#include "base/logging.h"
#include "chrome/installer/zucchini/buffer_view.h"
#include "chrome/installer/zucchini/crc32.h"
#include "chrome/installer/zucchini/io_utils.h"

namespace {

/******** BufferedFileReader ********/

// A file reader wrapper.
class BufferedFileReader {
 public:
  explicit BufferedFileReader(const base::FilePath& file_name) {
    is_ok_ = buffer_.Initialize(file_name);
    if (!is_ok_)  // This is also triggered if |file_name| is an empty file.
      std::cerr << "Can't read file." << std::endl;
  }
  bool is_ok() const { return is_ok_; }
  uint8_t* data() { return buffer_.data(); }
  size_t length() const { return buffer_.length(); }
  zucchini::ConstBufferView region() { return {data(), length()}; }

 private:
  bool is_ok_;
  base::MemoryMappedFile buffer_;
};

/******** BufferedFileWriter ********/

// A file writer wrapper.
class BufferedFileWriter {
 public:
  BufferedFileWriter(const base::FilePath& file_name, size_t length) {
    is_ok_ = buffer_.Initialize(
        {file_name, base::File::FLAG_CREATE_ALWAYS | base::File::FLAG_READ |
                        base::File::FLAG_WRITE},
        {0, int64_t(length)}, base::MemoryMappedFile::READ_WRITE_EXTEND);
    if (!is_ok_)
      std::cerr << "Can't create file." << std::endl;
  }
  bool is_ok() const { return is_ok_; }
  uint8_t* data() { return buffer_.data(); }
  size_t length() const { return buffer_.length(); }
  zucchini::MutableBufferView region() { return {data(), length()}; }

 private:
  bool is_ok_;
  base::MemoryMappedFile buffer_;
};

}  // namespace

zucchini::status::Code MainGen(const base::CommandLine& command_line,
                               const std::vector<base::FilePath>& fnames) {
  CHECK_EQ(3U, fnames.size());
  // TODO(etiennep): Implement.
  return zucchini::status::kStatusSuccess;
}

zucchini::status::Code MainApply(const base::CommandLine& command_line,
                                 const std::vector<base::FilePath>& fnames) {
  CHECK_EQ(3U, fnames.size());
  // TODO(etiennep): Implement.
  return zucchini::status::kStatusSuccess;
}

zucchini::status::Code MainCrc32(const base::CommandLine& command_line,
                                 const std::vector<base::FilePath>& fnames) {
  CHECK_EQ(1U, fnames.size());
  BufferedFileReader image(fnames[0]);
  if (!image.is_ok())
    return zucchini::status::kStatusFileReadError;

  uint32_t crc =
      zucchini::CalculateCrc32(image.data(), image.data() + image.length());
  std::cout << zucchini::AsHex<8>(crc) << std::endl;
  return zucchini::status::kStatusSuccess;
}
