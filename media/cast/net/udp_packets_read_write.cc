// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/cast/net/udp_packets_read_write.h"

#include <cstring>

#include "base/callback.h"
#include "base/logging.h"
#include "base/memory/ptr_util.h"

namespace media {
namespace cast {

// UdpPacketsReader

UdpPacketsReader::UdpPacketsReader(
    mojo::ScopedDataPipeConsumerHandle consumer_handle,
    const ReadCB& read_cb)
    : data_pipe_reader_(std::move(consumer_handle)),
      read_cb_(read_cb),
      is_reading_packet_size_(true) {
  DCHECK(data_pipe_reader_.IsPipeValid());
  // Always try reading the size of next packet first.
  ReadNextPacket(sizeof(uint32_t));
}

UdpPacketsReader::~UdpPacketsReader() {}

void UdpPacketsReader::ReadNextPacket(uint32_t packet_size) {
  DCHECK(!current_packet_);
  current_packet_ = base::MakeUnique<Packet>(packet_size);
  data_pipe_reader_.Read(
      current_packet_->data(), packet_size,
      base::BindOnce(&UdpPacketsReader::OnPacketRead, base::Unretained(this)));
}

void UdpPacketsReader::OnPacketRead(bool success) {
  if (success) {
    uint32_t next_packet_size = sizeof(uint32_t);
    if (is_reading_packet_size_) {
      DCHECK_EQ(sizeof(next_packet_size), current_packet_->size());
      std::memcpy(&next_packet_size, current_packet_->data(),
                  sizeof(next_packet_size));
      DCHECK_GT(next_packet_size, 0u);
      DVLOG(3) << "next_packet_size=" << next_packet_size;
    } else {
      DCHECK_GT(current_packet_->size(), 0u);
      if (!read_cb_.is_null())
        read_cb_.Run(std::move(current_packet_));
    }
    is_reading_packet_size_ = !is_reading_packet_size_;
    current_packet_.reset();
    ReadNextPacket(next_packet_size);
  } else {
    VLOG(1) << "Failed when reading packets.";
    current_packet_.reset();
    is_reading_packet_size_ = true;
    // The data pipe should have been closed due to errors.
  }
}

// UdpPacketsWriter

UdpPacketsWriter::UdpPacketsWriter(
    mojo::ScopedDataPipeProducerHandle producer_handle)
    : data_pipe_writer_(std::move(producer_handle)) {
  DCHECK(data_pipe_writer_.IsPipeValid());
}

UdpPacketsWriter::~UdpPacketsWriter() {}

void UdpPacketsWriter::Write(PacketRef packet) {
  const bool need_to_start_writing = pending_packets_.empty();
  uint32_t packet_size = packet->data.size();
  DCHECK_GT(packet_size, 0u);
  DVLOG(3) << "Request to write packet. size=" << packet_size;
  // Requests to write the packet size first.
  Packet size_packet(sizeof(packet_size));
  std::memcpy(size_packet.data(), &packet_size, sizeof(packet_size));
  pending_packets_.push_back(new base::RefCountedData<Packet>(size_packet));
  // Then requests to write the packet.
  pending_packets_.push_back(packet);
  if (need_to_start_writing)
    WriteNextPacket();
}

void UdpPacketsWriter::WriteNextPacket() {
  if (pending_packets_.empty())
    return;
  const Packet* packet = &pending_packets_.front()->data;
  data_pipe_writer_.Write(packet->data(), packet->size(),
                          base::BindOnce(&UdpPacketsWriter::OnPacketWritten,
                                         base::Unretained(this)));
}

void UdpPacketsWriter::OnPacketWritten(bool success) {
  DCHECK(!pending_packets_.empty());
  if (success) {
    pending_packets_.pop_front();
    WriteNextPacket();
  } else {
    VLOG(1) << "Failed to write packets. Remaining num of packets = "
            << pending_packets_.size();
    pending_packets_.clear();
    // The data pipe should have been closed due to errors.
  }
}

}  // namespace cast
}  // namespace media
