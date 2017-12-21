// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/cast/net/udp_packet_pipe.h"

#include <cstring>

#include "base/callback.h"
#include "base/logging.h"
#include "base/memory/ptr_util.h"

namespace media {
namespace cast {

// UdpPacketPipeReader

UdpPacketPipeReader::UdpPacketPipeReader(
    mojo::ScopedDataPipeConsumerHandle consumer_handle)
    : data_pipe_reader_(std::move(consumer_handle)) {
  DCHECK(data_pipe_reader_.IsPipeValid());
}

UdpPacketPipeReader::~UdpPacketPipeReader() {}

void UdpPacketPipeReader::Read(ReadCB cb) {
  DCHECK(current_read_cb_.is_null());
  DCHECK(!cb.is_null());
  current_read_cb_ = std::move(cb);
  data_pipe_reader_.Read(reinterpret_cast<uint8_t*>(&current_packet_size_),
                         sizeof(uint16_t),
                         base::BindOnce(&UdpPacketPipeReader::ReadPacketPayload,
                                        base::Unretained(this)));
}

void UdpPacketPipeReader::ReadPacketPayload(bool success) {
  if (!success) {
    OnPacketRead(success);
    return;
  }
  DCHECK(!current_packet_);
  current_packet_ = base::MakeUnique<Packet>(current_packet_size_);
  data_pipe_reader_.Read(current_packet_->data(), current_packet_size_,
                         base::BindOnce(&UdpPacketPipeReader::OnPacketRead,
                                        base::Unretained(this)));
}

void UdpPacketPipeReader::OnPacketRead(bool success) {
  DCHECK(!current_read_cb_.is_null());
  if (!success) {
    VLOG(1) << "Failed when reading packets.";
    current_packet_.reset();
    // The data pipe should have been closed due to errors.
  }
  std::move(current_read_cb_).Run(std::move(current_packet_));
}

// UdpPacketPipeWriter

UdpPacketPipeWriter::UdpPacketPipeWriter(
    mojo::ScopedDataPipeProducerHandle producer_handle)
    : data_pipe_writer_(std::move(producer_handle)) {
  DCHECK(data_pipe_writer_.IsPipeValid());
}

UdpPacketPipeWriter::~UdpPacketPipeWriter() {}

void UdpPacketPipeWriter::Write(const PacketRef& packet,
                                base::OnceClosure done_cb) {
  DCHECK(current_done_cb_.is_null());
  DCHECK(!done_cb.is_null());
  current_done_cb_ = std::move(done_cb);
  current_packet_size_ = packet->data.size();
  current_packet_ = packet;
  data_pipe_writer_.Write(
      reinterpret_cast<uint8_t*>(&current_packet_size_), sizeof(uint16_t),
      base::BindOnce(&UdpPacketPipeWriter::WritePacketPayload,
                     base::Unretained(this)));
}

void UdpPacketPipeWriter::WritePacketPayload(bool success) {
  if (!success) {
    OnPacketWritten(success);
    return;
  }
  const Packet* packet = &current_packet_->data;
  data_pipe_writer_.Write(packet->data(), packet->size(),
                          base::BindOnce(&UdpPacketPipeWriter::OnPacketWritten,
                                         base::Unretained(this)));
}

void UdpPacketPipeWriter::OnPacketWritten(bool success) {
  DCHECK(!current_done_cb_.is_null());
  if (!success) {
    VLOG(1) << "Failed to write packets.";
    // The data pipe should have been closed due to errors.
  }
  std::move(current_done_cb_).Run();
}

}  // namespace cast
}  // namespace media
