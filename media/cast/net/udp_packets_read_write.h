// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_CAST_NET_UDP_PACKETS_CONVERTER_H_
#define MEDIA_CAST_NET_UDP_PACKETS_CONVERTER_H_

#include "base/containers/circular_deque.h"
#include "media/cast/net/cast_transport_defines.h"
#include "media/mojo/common/mojo_data_pipe_read_write.h"

namespace media {
namespace cast {

// Reads UDP packets from the mojo data pipe. Assuming the size of each UDP
// packet was written before the packet itself was written into the pipe. After
// created, this reader keeps listening for any new data until it is destructed.
// |read_cb_| will be called each time when one packet is completely read.
class UdpPacketsReader {
 public:
  using ReadCB = base::RepeatingCallback<void(std::unique_ptr<Packet>)>;
  UdpPacketsReader(mojo::ScopedDataPipeConsumerHandle consumer_handle,
                   const ReadCB& read_cb);

  ~UdpPacketsReader();

 private:
  void ReadNextPacket(uint32_t packet_size);

  // Called by |data_pipe_reader_| when one reading completes.
  void OnPacketRead(bool success);

  MojoDataPipeReader data_pipe_reader_;

  // The repeating callback to receive the read packets.
  ReadCB read_cb_;

  // Indicates whether the current reading is to read the size of next packet.
  bool is_reading_packet_size_;

  // The buffer to store the current packet.
  std::unique_ptr<Packet> current_packet_;

  DISALLOW_COPY_AND_ASSIGN(UdpPacketsReader);
};

// Writes UDP packets into the data mojo pipe. The size of each packet is
// written before the packet itself is written into the data pipe.
class UdpPacketsWriter {
 public:
  explicit UdpPacketsWriter(mojo::ScopedDataPipeProducerHandle producer_handle);

  ~UdpPacketsWriter();

  // Requests to write the |packet| into the mojo data pipe. The |packet| is
  // enqueued in |pending_packets_|. The actual writing may not happen
  // immediately.
  void Write(PacketRef packet);

 private:
  void WriteNextPacket();

  // Called by |data_pipe_writer_| when one writing compelets.
  void OnPacketWritten(bool success);

  MojoDataPipeWriter data_pipe_writer_;

  // The pending packets to be written.
  base::circular_deque<PacketRef> pending_packets_;

  DISALLOW_COPY_AND_ASSIGN(UdpPacketsWriter);
};

}  // namespace cast
}  // namespace media

#endif  // MEDIA_CAST_NET_UDP_PACKETS_CONVERTER_H_
