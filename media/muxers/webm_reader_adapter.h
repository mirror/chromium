// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_MUXERS_WEBM_READER_ADAPTER_H_
#define MEDIA_MUXERS_WEBM_READER_ADAPTER_H_

#include <memory>

#include "media/muxers/mkv_reader_writer.h"
#include "media/muxers/webm_muxer.h"
#include "third_party/libwebm/source/webm_parser/include/webm/webm_parser.h"

namespace media {

// Adapter that delegates webm::Reader to a given MkvReader instance.
class WebmReaderAdapter : public webm::Reader {
 public:
  WebmReaderAdapter(MkvReader* reader);

  webm::Status Read(std::size_t num_to_read,
                    std::uint8_t* buffer,
                    std::uint64_t* num_actually_read) override;
  webm::Status Skip(std::uint64_t num_to_skip,
                    std::uint64_t* num_actually_skipped) override;
  std::uint64_t Position() const override;

 private:
  MkvReader* reader_;
  uint64_t read_pos_;
};

}  // namespace media

#endif  // MEDIA_MUXERS_WEBM_READER_ADAPTER_H_
