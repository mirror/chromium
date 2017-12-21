// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_CAST_NET_UDP_PACKET_PIPE_H_
#define MEDIA_CAST_NET_UDP_PACKET_PIPE_H_

#include "base/containers/circular_deque.h"
#include "media/cast/net/cast_transport_defines.h"
#include "media/mojo/common/mojo_data_pipe_read_write.h"

namespace media {
namespace cast {

// Reads UDP packets from the mojo data pipe. Assuming the size of each UDP
// packet was written before the packet itself was written into the pipe.
class UdpPacketPipeReader {
 public:
  explicit UdpPacketPipeReader(
      mojo::ScopedDataPipeConsumerHandle consumer_handle);

  ~UdpPacketPipeReader();

  using ReadCB = base::OnceCallback<void(std::unique_ptr<Packet>)>;
  // Reads one UDP packet from the data pipe.
  void Read(ReadCB cb);

 private:
  // Reads packet payload from the data pipe. |success| indicates whether the
  // size of the packet was successfully read from the data pipe.
  void ReadPacketPayload(bool success);
  // Called by |data_pipe_reader_| when the reading completes.
  void OnPacketRead(bool success);

  MojoDataPipeReader data_pipe_reader_;

  // The once callback for the current reading.
  ReadCB current_read_cb_;

  uint16_t current_packet_size_;
  std::unique_ptr<Packet> current_packet_;

  DISALLOW_COPY_AND_ASSIGN(UdpPacketPipeReader);
};

// Writes UDP packets into the data mojo pipe. The size of each packet is
// written before the packet itself is written into the data pipe.
class UdpPacketPipeWriter {
 public:
  explicit UdpPacketPipeWriter(
      mojo::ScopedDataPipeProducerHandle producer_handle);

  ~UdpPacketPipeWriter();

  // Writes the |packet| into the mojo data pipe. |done_cb| will be
  // called when the writing completes.
  void Write(const PacketRef& packet, base::OnceClosure done_cb);

 private:
  // Writes the packet payload in the data pipe. |success| indicates whether the
  // size of the packet was successfully written in the data pipe.
  void WritePacketPayload(bool success);
  // Called by |data_pipe_writer_| when the writing completes.
  void OnPacketWritten(bool success);

  MojoDataPipeWriter data_pipe_writer_;

  // The once callback for the current writing.
  base::OnceClosure current_done_cb_;

  uint16_t current_packet_size_;
  PacketRef current_packet_;

  DISALLOW_COPY_AND_ASSIGN(UdpPacketPipeWriter);
};

}  // namespace cast
}  // namespace media

#endif  // MEDIA_CAST_NET_UDP_PACKET_PIPE_H_
