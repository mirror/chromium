// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/test/spawned_test_server/remote_test_server_proxy.h"

#include "base/callback.h"
#include "net/base/io_buffer.h"
#include "net/base/net_errors.h"
#include "net/socket/stream_socket.h"
#include "net/socket/tcp_client_socket.h"

namespace net {

namespace {

const int kBufferSize = 1024;

// Helper that reads data from one socket and then forwards to another socket.
class SocketDataPump {
 public:
  SocketDataPump(StreamSocket* from_socket,
                 StreamSocket* to_socket,
                 base::OnceClosure on_done_callback)
      : from_socket_(from_socket),
        to_socket_(to_socket),
        on_done_callback_(std::move(on_done_callback)) {
    read_buffer_ = new IOBuffer(kBufferSize);
  }

  void Start() { Read(); }

 private:
  void Read() {
    int result = from_socket_->Read(
        read_buffer_.get(), kBufferSize,
        base::Bind(&SocketDataPump::HandleReadResult, base::Unretained(this)));
    if (result != ERR_IO_PENDING)
      HandleReadResult(result);
  }

  void HandleReadResult(int result) {
    if (result <= 0) {
      std::move(on_done_callback_).Run();
      return;
    }

    write_buffer_ = new DrainableIOBuffer(read_buffer_.get(), result);
    Write();
  }

  void Write() {
    int result = to_socket_->Write(
        write_buffer_.get(), write_buffer_->BytesRemaining(),
        base::Bind(&SocketDataPump::HandleWriteResult, base::Unretained(this)));
    if (result != ERR_IO_PENDING)
      HandleWriteResult(result);
  }

  void HandleWriteResult(int result) {
    if (result <= 0) {
      std::move(on_done_callback_).Run();
      return;
    }

    write_buffer_->DidConsume(result);
    if (write_buffer_->BytesRemaining()) {
      Write();
    } else {
      Read();
    }
  }

  StreamSocket* from_socket_;
  StreamSocket* to_socket_;

  scoped_refptr<IOBuffer> read_buffer_;
  scoped_refptr<DrainableIOBuffer> write_buffer_;

  base::OnceClosure on_done_callback_;

  DISALLOW_COPY_AND_ASSIGN(SocketDataPump);
};

}  // namespace

// ConnectionProxy is responsible for proxying one connection to a remote
// address.
class RemoteTestServerProxy::ConnectionProxy {
 public:
  explicit ConnectionProxy(std::unique_ptr<StreamSocket> local_socket);
  ~ConnectionProxy() {}

  void Start(const IPEndPoint& remote_address,
             base::OnceClosure on_done_callback);

 private:
  void OnError();

  void HandleConnectResult(const IPEndPoint& remote_address, int result);

  base::OnceClosure on_done_callback_;

  std::unique_ptr<StreamSocket> local_socket_;
  std::unique_ptr<StreamSocket> remote_socket_;

  std::unique_ptr<SocketDataPump> incoming_pump_;
  std::unique_ptr<SocketDataPump> outgoing_pump_;

  base::WeakPtrFactory<ConnectionProxy> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(ConnectionProxy);
};

RemoteTestServerProxy::ConnectionProxy::ConnectionProxy(
    std::unique_ptr<StreamSocket> local_socket)
    : local_socket_(std::move(local_socket)), weak_factory_(this) {}

void RemoteTestServerProxy::ConnectionProxy::Start(
    const IPEndPoint& remote_address,
    base::OnceClosure on_done_callback) {
  on_done_callback_ = std::move(on_done_callback);
  remote_socket_ = std::make_unique<TCPClientSocket>(
      AddressList(remote_address), nullptr, nullptr, NetLogSource());
  int result = remote_socket_->Connect(
      base::Bind(&ConnectionProxy::HandleConnectResult, base::Unretained(this),
                 remote_address));
  if (result != ERR_IO_PENDING)
    HandleConnectResult(remote_address, result);
}

void RemoteTestServerProxy::ConnectionProxy::HandleConnectResult(
    const IPEndPoint& remote_address,
    int result) {
  if (result < 0) {
    LOG(ERROR) << "Connection to " << remote_address.ToString()
               << " failed: " << ErrorToString(result);
    OnError();
  }

  incoming_pump_ = std::make_unique<SocketDataPump>(
      remote_socket_.get(), local_socket_.get(),
      base::BindOnce(&ConnectionProxy::OnError, base::Unretained(this)));
  outgoing_pump_ = std::make_unique<SocketDataPump>(
      local_socket_.get(), remote_socket_.get(),
      base::BindOnce(&ConnectionProxy::OnError, base::Unretained(this)));

  auto self = weak_factory_.GetWeakPtr();
  incoming_pump_->Start();
  if (!self)
    return;

  outgoing_pump_->Start();
}

void RemoteTestServerProxy::ConnectionProxy::OnError() {
  local_socket_.reset();
  remote_socket_.reset();
  std::move(on_done_callback_).Run();
}

RemoteTestServerProxy::RemoteTestServerProxy(const IPEndPoint& remote_address)
    : remote_address_(remote_address), socket_(nullptr, net::NetLogSource()) {
  int r = socket_.Listen(IPEndPoint(IPAddress::IPv4Localhost(), 0), 5);
  CHECK_EQ(r, OK);

  // Get local port number.
  IPEndPoint address;
  r = socket_.GetLocalAddress(&address);
  CHECK_EQ(r, OK);
  local_port_ = address.port();

  DoAccept();
}

RemoteTestServerProxy::~RemoteTestServerProxy() {}

void RemoteTestServerProxy::DoAccept() {
  int result;
  do {
    result = socket_.Accept(&accepted_socket_,
                            base::Bind(&RemoteTestServerProxy::OnAcceptResult,
                                       base::Unretained(this)));
    if (result != ERR_IO_PENDING)
      HandleAcceptResult(result);
  } while (result == OK);
}

void RemoteTestServerProxy::OnAcceptResult(int result) {
  HandleAcceptResult(result);
  if (result == OK)
    DoAccept();
}

void RemoteTestServerProxy::HandleAcceptResult(int result) {
  DCHECK_NE(result, ERR_IO_PENDING);

  if (result < 0) {
    LOG(ERROR) << "Error when accepting a connection: "
               << ErrorToString(result);
    return;
  }

  std::unique_ptr<ConnectionProxy> connection_proxy =
      std::make_unique<ConnectionProxy>(std::move(accepted_socket_));
  ConnectionProxy* connection_proxy_ptr = connection_proxy.get();
  connections_.push_back(std::move(connection_proxy));

  // Start() may invoke the callback so it needs to be called after the
  // connection is pushed to connections_.
  connection_proxy_ptr->Start(
      remote_address_,
      base::BindOnce(&RemoteTestServerProxy::OnConnectionDone,
                     base::Unretained(this), connection_proxy_ptr));
}

void RemoteTestServerProxy::OnConnectionDone(ConnectionProxy* connection) {
  for (auto it = connections_.begin(); it != connections_.end(); ++it) {
    if (it->get() == connection)
      connections_.erase(it);
    return;
  }
  NOTREACHED();
}

}  // namespace net
