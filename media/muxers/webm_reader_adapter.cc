// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/muxers/webm_reader_adapter.h"

namespace media {

WebmReaderAdapter::WebmReaderAdapter(MkvReader* reader)
    : reader_(reader), read_pos_(0) {}

webm::Status WebmReaderAdapter::Read(std::size_t num_to_read,
                                     std::uint8_t* buffer,
                                     std::uint64_t* num_actually_read) {
  int result_code = reader_->Read(base::saturated_cast<long long>(read_pos_),
                                  base::saturated_cast<long>(num_to_read),
                                  reinterpret_cast<unsigned char*>(buffer));
  if (result_code != 0) {
    *num_actually_read = 0;
    // webm::Status does not offer a general failure code. For the lack of
    // better options, we use kEndOfFile.
    return webm::Status(webm::Status::kEndOfFile);
  }
  *num_actually_read = num_to_read;
  read_pos_ += num_to_read;
  return webm::Status(webm::Status::kOkCompleted);
}

webm::Status WebmReaderAdapter::Skip(std::uint64_t num_to_skip,
                                     std::uint64_t* num_actually_skipped) {
  *num_actually_skipped = num_to_skip;
  read_pos_ += num_to_skip;
  return webm::Status(webm::Status::kOkCompleted);
}

std::uint64_t WebmReaderAdapter::Position() const {
  return read_pos_;
}

}  // namespace media
