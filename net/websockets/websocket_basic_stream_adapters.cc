// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/websockets/websocket_basic_stream_adapters.h"

#include <algorithm>
#include <cstring>
#include <utility>

#include "net/socket/client_socket_handle.h"
#include "net/socket/socket.h"

namespace net {

WebSocketClientSocketHandleAdapter::WebSocketClientSocketHandleAdapter(
    std::unique_ptr<ClientSocketHandle> connection)
    : connection_(std::move(connection)) {}

WebSocketClientSocketHandleAdapter::~WebSocketClientSocketHandleAdapter() {}

int WebSocketClientSocketHandleAdapter::Read(
    IOBuffer* buf,
    int buf_len,
    const CompletionCallback& callback) {
  return connection_->socket()->Read(buf, buf_len, callback);
}

int WebSocketClientSocketHandleAdapter::Write(
    IOBuffer* buf,
    int buf_len,
    const CompletionCallback& callback,
    const NetworkTrafficAnnotationTag& traffic_annotation) {
  return connection_->socket()->Write(buf, buf_len, callback,
                                      traffic_annotation);
}

void WebSocketClientSocketHandleAdapter::Disconnect() {
  connection_->socket()->Disconnect();
}

bool WebSocketClientSocketHandleAdapter::is_initialized() const {
  return connection_->is_initialized();
}

WebSocketSpdyStreamAdapter::WebSocketSpdyStreamAdapter(
    base::WeakPtr<SpdyStream> stream,
    Delegate* delegate,
    NetLogWithSource net_log)
    : stream_(stream),
      stream_error_(ERR_CONNECTION_CLOSED),
      delegate_(delegate),
      write_length_(0),
      net_log_(net_log) {
  stream_->SetDelegate(this);
}

WebSocketSpdyStreamAdapter::~WebSocketSpdyStreamAdapter() {
  if (stream_)
    stream_->DetachDelegate();
}

void WebSocketSpdyStreamAdapter::DetachDelegate() {
  delegate_ = nullptr;
}

int WebSocketSpdyStreamAdapter::Read(IOBuffer* buf,
                                     int buf_len,
                                     const CompletionCallback& callback) {
  DCHECK(!read_callback_);
  DCHECK(callback);
  DCHECK_LT(0, buf_len);

  if (!stream_)
    return stream_error_;

  read_buffer_ = buf;
  read_length_ = buf_len;

  if (read_data_.empty()) {
    read_callback_ = callback;
    return ERR_IO_PENDING;
  }

  return CopySavedReadDataIntoBuffer();
}

int WebSocketSpdyStreamAdapter::Write(
    IOBuffer* buf,
    int buf_len,
    const CompletionCallback& callback,
    const NetworkTrafficAnnotationTag& traffic_annotation) {
  DCHECK(!write_callback_);
  DCHECK(callback);
  DCHECK_LT(0, buf_len);

  if (!stream_)
    return stream_error_;

  stream_->SendData(buf, buf_len, MORE_DATA_TO_SEND);
  write_callback_ = callback;
  write_length_ = buf_len;
  return ERR_IO_PENDING;
}

void WebSocketSpdyStreamAdapter::Disconnect() {
  if (stream_) {
    stream_->DetachDelegate();
    stream_ = nullptr;
  }
}

bool WebSocketSpdyStreamAdapter::is_initialized() const {
  return true;
}

// SpdyStream::Delegate methods.
void WebSocketSpdyStreamAdapter::OnHeadersSent() {
  if (delegate_)
    delegate_->OnHeadersSent();
}

void WebSocketSpdyStreamAdapter::OnHeadersReceived(
    const SpdyHeaderBlock& response_headers) {
  if (delegate_)
    delegate_->OnHeadersReceived(response_headers);
}

void WebSocketSpdyStreamAdapter::OnDataReceived(
    std::unique_ptr<SpdyBuffer> buffer) {
  read_data_.push_back(std::move(buffer));
  if (read_callback_)
    base::ResetAndReturn(&read_callback_).Run(CopySavedReadDataIntoBuffer());
}

void WebSocketSpdyStreamAdapter::OnDataSent() {
  DCHECK(!write_callback_);

  base::ResetAndReturn(&write_callback_).Run(write_length_);
}

void WebSocketSpdyStreamAdapter::OnTrailers(const SpdyHeaderBlock& trailers) {}

void WebSocketSpdyStreamAdapter::OnClose(int status) {
  DCHECK_GT(ERR_IO_PENDING, status);

  stream_ = nullptr;

  if (read_callback_)
    base::ResetAndReturn(&read_callback_).Run(status);
  if (write_callback_)
    base::ResetAndReturn(&write_callback_).Run(status);

  // Might destroy |this|.
  if (delegate_)
    delegate_->OnClose(status);
}

NetLogSource WebSocketSpdyStreamAdapter::source_dependency() const {
  return net_log_.source();
}

int WebSocketSpdyStreamAdapter::CopySavedReadDataIntoBuffer() {
  DCHECK(!read_data_.empty());

  SpdyBuffer* const current = read_data_.front().get();

  size_t length =
      std::min(static_cast<size_t>(read_length_), current->GetRemainingSize());
  memcpy(read_buffer_->data(), current->GetRemainingData(), length);
  current->Consume(length);
  if (current->GetRemainingSize() == 0)
    read_data_.pop_front();

  return length;
}

}  // namespace net
