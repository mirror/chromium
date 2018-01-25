// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/media/udp_transport_host_impl.h"

#include "content/public/browser/browser_thread.h"
#include "media/cast/net/udp_transport.h"
#include "mojo/public/cpp/bindings/strong_binding.h"

using content::BrowserThread;

namespace media {

class UdpTransportHostImpl::MojoDataReader {
 public:
  using ReadCB = base::Callback<void(cast::PacketRef)>;
  MojoDataReader(mojo::ScopedDataPipeConsumerHandle consumer_handle,
                 const ReadCB& read_cb)
      : consumer_handle_(std::move(consumer_handle)),
        pipe_watcher_(FROM_HERE, mojo::SimpleWatcher::ArmingPolicy::MANUAL),
        armed_(false),
        bytes_read_(0),
        is_reading_size_(true),
        read_cb_(read_cb) {
    DVLOG(1) << __func__;

    MojoResult result = pipe_watcher_.Watch(
        consumer_handle_.get(), MOJO_HANDLE_SIGNAL_READABLE,
        MOJO_WATCH_CONDITION_SATISFIED,
        base::Bind(&MojoDataReader::OnPipeReadable, base::Unretained(this)));
    if (result != MOJO_RESULT_OK) {
      DVLOG(1) << __func__
               << ": Failed to start watching the pipe. result=" << result;
      consumer_handle_.reset();
    }

    DCHECK(consumer_handle_.is_valid());
    DCHECK(!read_cb_.is_null());
    Read(4);
  }

  ~MojoDataReader() { DVLOG(1) << __func__; }

 private:
  void Read(uint32_t packet_size) {
    DCHECK(!read_cb_.is_null());

    if (!consumer_handle_.is_valid())
      return;

    DCHECK_GT(packet_size, 0u);
    DCHECK(!current_packet_);
    current_packet_ = new base::RefCountedData<cast::Packet>;
    current_packet_->data.resize(packet_size);

    ScheduleNextRead();
  }

  void ScheduleNextRead() {
    if (armed_)
      return;

    if (current_packet_) {
      armed_ = true;
      pipe_watcher_.ArmOrNotify();
    }
  }

  void OnPipeReadable(MojoResult result,
                      const mojo::HandleSignalsState& state) {
    armed_ = false;

    if (result != MOJO_RESULT_OK) {
      OnPipeError(result);
      return;
    }

    DCHECK(state.readable());
    ReadData();
  }

  void ReadData() {
    DCHECK(current_packet_);
    uint32_t buffer_size = current_packet_->data.size();
    DCHECK_GT(buffer_size, 0u);

    uint32_t num_bytes = buffer_size - bytes_read_;
    DCHECK_GT(num_bytes, 0u);

    MojoResult result = consumer_handle_->ReadData(
        static_cast<uint8_t*>(&current_packet_->data.front()) + bytes_read_,
        &num_bytes, MOJO_READ_DATA_FLAG_NONE);
    if (result != MOJO_RESULT_OK && result != MOJO_RESULT_SHOULD_WAIT) {
      OnPipeError(result);
    } else {
      if (result == MOJO_RESULT_OK) {
        DCHECK_GT(num_bytes, 0u);
        bytes_read_ += num_bytes;
        if (bytes_read_ == buffer_size) {
          CompleteCurrentRead();
          return;
        }
      }
      ScheduleNextRead();
    }
  }

  void CompleteCurrentRead() {
    bytes_read_ = 0;
    uint32_t next_packet_size = 0;
    if (!is_reading_size_) {
      DVLOG(3) << "Done read packet_size=" << current_packet_->data.size();
      read_cb_.Run(std::move(current_packet_));
      next_packet_size = 4;
    } else {
      next_packet_size |= current_packet_->data[0];
      next_packet_size |= (current_packet_->data[1] << 8);
      next_packet_size |= (current_packet_->data[2] << 16);
      next_packet_size |= (current_packet_->data[3] << 24);
      DCHECK_GT(next_packet_size, 0u);
    }
    is_reading_size_ = !is_reading_size_;
    current_packet_ = nullptr;
    Read(next_packet_size);
  }

  void OnPipeError(MojoResult result) {
    DVLOG(1) << __func__ << "(" << result << ")";

    consumer_handle_.reset();

    if (current_packet_ && !is_reading_size_) {
      DVLOG(1) << __func__ << ": reading failed. result=" << result
               << " buffer_size=" << current_packet_->data.size()
               << "num_bytes(read)=" << bytes_read_;
      bytes_read_ = 0;
      current_packet_ = nullptr;
    }
  }

  mojo::ScopedDataPipeConsumerHandle consumer_handle_;
  mojo::SimpleWatcher pipe_watcher_;
  bool armed_;

  cast::PacketRef current_packet_;

  uint32_t bytes_read_;
  bool is_reading_size_;

  ReadCB read_cb_;

  DISALLOW_COPY_AND_ASSIGN(MojoDataReader);
};

UdpTransportHostImpl::UdpTransportHostImpl(
    std::unique_ptr<cast::UdpTransport> transport)
    : udp_transport_(std::move(transport)) {
  DCHECK(udp_transport_);
}

UdpTransportHostImpl::~UdpTransportHostImpl() {}

// static
void UdpTransportHostImpl::Create(
    std::unique_ptr<cast::UdpTransport> transport,
    base::OnceCallback<void(mojom::UdpTransportHostPtr)> callback) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  mojom::UdpTransportHostPtr ptr;
  mojom::UdpTransportHostRequest request = mojo::MakeRequest(&ptr);
  mojo::MakeStrongBinding(
      base::MakeUnique<UdpTransportHostImpl>(std::move(transport)),
      std::move(request));
  if (!callback.is_null())
    base::ResetAndReturn(&callback).Run(std::move(ptr));
}

bool UdpTransportHostImpl::OnPacketReceived(
    std::unique_ptr<cast::Packet> packet) {
  if (!client_)
    return true;
  client_->OnPacketReceived(*packet);
  return true;
}

void UdpTransportHostImpl::StartReceiving(mojom::UdpTransportClientPtr client) {
  DCHECK(client);
  DVLOG(1) << __func__;
  client_ = std::move(client);
  udp_transport_->StartReceiving(base::Bind(
      &UdpTransportHostImpl::OnPacketReceived, base::Unretained(this)));
}

void UdpTransportHostImpl::StopReceiving() {
  DVLOG(1) << __func__;
  client_ = nullptr;
  udp_transport_->StopReceiving();
}

void UdpTransportHostImpl::OnPacketSent() {
  if (send_pending_) {
    send_pending_ = false;
    if (client_)
      client_->OnNetworkBlocked(false);
  }
}

void UdpTransportHostImpl::StartSending(
    mojo::ScopedDataPipeConsumerHandle packet_pipe) {
  DCHECK(packet_pipe.is_valid());
  DCHECK(!reader_);

  reader_.reset(new MojoDataReader(
      std::move(packet_pipe),
      base::Bind(&UdpTransportHostImpl::SendPacket, base::Unretained(this))));
  ;
}

void UdpTransportHostImpl::SendPacket(cast::PacketRef packet) {
  if (send_pending_)
    return;
  DVLOG(3) << "SendPacket: " << packet->data.size();

  if (!udp_transport_->SendPacket(
          std::move(packet), base::Bind(&UdpTransportHostImpl::OnPacketSent,
                                        base::Unretained(this)))) {
    send_pending_ = true;
    if (client_)
      client_->OnNetworkBlocked(true);
  }
}

}  // namespace media
