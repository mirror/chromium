// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/mojo/services/mirror/udp_transport_client_impl.h"

namespace media {

class UdpTransportClientImpl::MojoDataPipeWriter {
 public:
  explicit MojoDataPipeWriter(
      mojo::ScopedDataPipeProducerHandle producer_handle)
      : producer_handle_(std::move(producer_handle)),
        pipe_watcher_(FROM_HERE, mojo::SimpleWatcher::ArmingPolicy::MANUAL),
        armed_(false),
        bytes_written_(0) {
    DVLOG(1) << __func__;
    MojoResult result =
        pipe_watcher_.Watch(producer_handle_.get(), MOJO_HANDLE_SIGNAL_WRITABLE,
                            MOJO_WATCH_CONDITION_SATISFIED,
                            base::Bind(&MojoDataPipeWriter::OnPipeWritable,
                                       base::Unretained(this)));
    if (result != MOJO_RESULT_OK) {
      DVLOG(1) << __func__
               << ": Failed to start watching the pipe. result=" << result;
      producer_handle_.reset();
    }
  };

  ~MojoDataPipeWriter() { DVLOG(1) << __func__; }

  void Write(cast::PacketRef packet) {
    DVLOG(3) << __func__ << ": packet_size=" << packet->data.size();

    if (!producer_handle_.is_valid()) {
      DVLOG(1) << __func__ << ": Failed to write because the pipe is closed.";
      return;
    }

    uint32_t packet_size = packet->data.size();
    DCHECK_GT(packet_size, 0u);
    cast::PacketRef size_packet(new base::RefCountedData<cast::Packet>);
    size_packet->data.push_back(static_cast<uint8_t>(packet_size));
    size_packet->data.push_back(static_cast<uint8_t>(packet_size >> 8));
    size_packet->data.push_back(static_cast<uint8_t>(packet_size >> 16));
    size_packet->data.push_back(static_cast<uint8_t>(packet_size >> 24));
    DCHECK_EQ(size_packet->data.size(), 4u);
    pending_buffers_.push_back(std::move(size_packet));
    pending_buffers_.push_back(packet);
    ScheduleNextWrite();
  }

 private:
  void ScheduleNextWrite() {
    DVLOG(3) << __func__;

    if (armed_)
      return;

    if (!pending_buffers_.empty()) {
      armed_ = true;
      pipe_watcher_.ArmOrNotify();
    }
  }

  void OnPipeWritable(MojoResult result,
                      const mojo::HandleSignalsState& state) {
    DVLOG(3) << __func__ << "(" << result << ", " << state.writable() << ")";

    armed_ = false;

    if (result != MOJO_RESULT_OK) {
      OnPipeError(result);
      return;
    }

    DCHECK(state.writable());
    WriteData();
  }

  void WriteData() {
    DCHECK(!pending_buffers_.empty());
    cast::Packet* packet = &(pending_buffers_.front()->data);
    uint32_t buffer_size = packet->size();
    DCHECK_GT(buffer_size, 0u);

    uint32_t num_bytes = buffer_size - bytes_written_;
    DCHECK_GT(num_bytes, 0u);

    DCHECK(producer_handle_.is_valid());

    MojoResult result = producer_handle_->WriteData(
        static_cast<uint8_t*>(&packet->front()) + bytes_written_, &num_bytes,
        MOJO_WRITE_DATA_FLAG_NONE);

    if (result != MOJO_RESULT_OK && result != MOJO_RESULT_SHOULD_WAIT) {
      OnPipeError(result);
    } else {
      if (result == MOJO_RESULT_OK) {
        DCHECK_GT(num_bytes, 0u);
        bytes_written_ += num_bytes;
        if (bytes_written_ == buffer_size) {
          pending_buffers_.pop_front();
          bytes_written_ = 0;
        }
      }
      ScheduleNextWrite();
    }
  }

  void OnPipeError(MojoResult result) {
    DVLOG(1) << __func__ << "(" << result << ")";

    producer_handle_.reset();

    if (!pending_buffers_.empty()) {
      DVLOG(1) << __func__ << ": writing to data pipe failed. result=" << result
               << ", buffer_size=" << pending_buffers_.front()->data.size()
               << ", num_bytes(written)=" << bytes_written_;
      pending_buffers_.clear();
      bytes_written_ = 0;
    }
  }

  mojo::ScopedDataPipeProducerHandle producer_handle_;
  mojo::SimpleWatcher pipe_watcher_;
  bool armed_;

  base::circular_deque<cast::PacketRef> pending_buffers_;
  uint32_t bytes_written_;

  DISALLOW_COPY_AND_ASSIGN(MojoDataPipeWriter);
};

UdpTransportClientImpl::UdpTransportClientImpl(mojom::UdpTransportHostPtr host)
    : binding_(this), weak_factory_(this) {
  udp_host_info_ = host.PassInterface();
}

UdpTransportClientImpl::~UdpTransportClientImpl() {}

bool UdpTransportClientImpl::SendPacket(cast::PacketRef packet,
                                        const base::Closure& cb) {
  bytes_sent_ += packet->data.size();

  if (send_pending_) {
    network_clear_cb_ = cb;
    DVLOG(1) << "Pending.";
    return false;
  }

  if (!writer_) {
    std::unique_ptr<mojo::DataPipe> data_pipe =
        base::MakeUnique<mojo::DataPipe>(800 * 1024);
    writer_.reset(
        new MojoDataPipeWriter(std::move(data_pipe->producer_handle)));
    GetUdpHost()->StartSending(std::move(data_pipe->consumer_handle));
  }

  writer_->Write(packet);
  return true;
}

int64_t UdpTransportClientImpl::GetBytesSent() {
  return bytes_sent_;
}

void UdpTransportClientImpl::StartReceiving(
    const cast::PacketReceiverCallbackWithStatus& packet_receiver) {
  DVLOG(1) << __func__;
  DCHECK(!packet_receiver.is_null());
  callback_ = packet_receiver;
  mojom::UdpTransportClientPtr this_client;
  binding_.Bind(mojo::MakeRequest(&this_client));
  GetUdpHost()->StartReceiving(std::move(this_client));
}

void UdpTransportClientImpl::StopReceiving() {
  callback_ = cast::PacketReceiverCallbackWithStatus();
  GetUdpHost()->StopReceiving();
}

void UdpTransportClientImpl::OnPacketReceived(
    const std::vector<uint8_t>& packet) {
  if (!callback_.is_null()) {
    std::unique_ptr<cast::Packet> received_packet(new cast::Packet());
    received_packet->swap(const_cast<cast::Packet&>(packet));
    callback_.Run(std::move(received_packet));
  }
}

void UdpTransportClientImpl::OnNetworkBlocked(bool blocked) {
  DVLOG(1) << __func__ << " blocked = " << blocked;
  if (blocked) {
    send_pending_ = true;
  } else {
    if (send_pending_) {
      send_pending_ = false;
      if (!network_clear_cb_.is_null()) {
        network_clear_cb_.Run();
        network_clear_cb_ = base::Closure();
      }
    }
  }
}

mojom::UdpTransportHost* UdpTransportClientImpl::GetUdpHost() {
  if (!udp_host_.get()) {
    udp_host_.Bind(std::move(udp_host_info_));
  }
  return udp_host_.get();
}

}  // namespace media
