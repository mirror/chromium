// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/muxers/mkv_reader_writer_adapter.h"

namespace media {

MkvReaderWriterAdapter::MkvReaderWriterAdapter(
    std::unique_ptr<MkvReaderWriter> reader_writer)
    : reader_writer_(reader_writer.get()) {
  optional_ownership_ = std::move(reader_writer);
}

MkvReaderWriterAdapter::MkvReaderWriterAdapter(
    MkvReaderWriter* reader_writer)
    : reader_writer_(reader_writer) {}

MkvReaderWriterAdapter::~MkvReaderWriterAdapter() = default;

int MkvReaderWriterAdapter::Read(long long pos,
                                 long len,
                                 unsigned char* buf) {
  return reader_writer_->Read(pos, len, buf);
}

int MkvReaderWriterAdapter::Length(long long* total,
                                   long long* available) {
  return reader_writer_->Length(total, available);
}

mkvmuxer::int32 MkvReaderWriterAdapter::Write(const void* buf,
                                              mkvmuxer::uint32 len) {
  auto result = reader_writer_->Write(buf, len);
  return result;
}

mkvmuxer::int64 MkvReaderWriterAdapter::Position() const {
  return reader_writer_->Position();
}

mkvmuxer::int32 MkvReaderWriterAdapter::Position(
    mkvmuxer::int64 position) {
  return reader_writer_->Position(position);
}

bool MkvReaderWriterAdapter::Seekable() const {
  return reader_writer_->Seekable();
}

void MkvReaderWriterAdapter::ElementStartNotify(
    mkvmuxer::uint64 element_id,
    mkvmuxer::int64 position) {
  return reader_writer_->ElementStartNotify(element_id, position);
}

}  // namespace media
