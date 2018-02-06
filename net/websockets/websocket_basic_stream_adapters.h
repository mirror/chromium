// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NET_WEBSOCKETS_WEBSOCKET_BASIC_STREAM_ADAPTERS_H_
#define NET_WEBSOCKETS_WEBSOCKET_BASIC_STREAM_ADAPTERS_H_

#include <list>
#include <memory>

#include "net/base/completion_callback.h"
#include "net/base/completion_once_callback.h"
#include "net/base/io_buffer.h"
#include "net/spdy/chromium/spdy_stream.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "net/websockets/websocket_basic_stream.h"

namespace net {

class ClientSocketHandle;

// Trivial adapter to make WebSocketBasicStream use an HTTP/1.1 connection.
class WebSocketClientSocketHandleAdapter
    : public WebSocketBasicStream::Adapter {
 public:
  WebSocketClientSocketHandleAdapter(
      std::unique_ptr<ClientSocketHandle> connection);
  ~WebSocketClientSocketHandleAdapter() override;

  int Read(IOBuffer* buf,
           int buf_len,
           const CompletionCallback& callback) override;
  int Write(IOBuffer* buf,
            int buf_len,
            const CompletionCallback& callback,
            const NetworkTrafficAnnotationTag& traffic_annotation) override;
  void Disconnect() override;
  bool is_initialized() const override;

 private:
  std::unique_ptr<ClientSocketHandle> connection_;
};

// Adapter to make WebSocketBasicStream use an HTTP/2 stream.
// Sets itself as a delegate of the SpdyStream, and forwards headers-related
// methods to WebSocketHttp2HandshakeStream.  After the handshake, ownership of
// this object can be passed to WebSocketBasicStream, which can read and write
// using a ClientSocketHandle-like interface.
class WebSocketSpdyStreamAdapter : public WebSocketBasicStream::Adapter,
                                   public SpdyStream::Delegate {
 public:
  // Interface for forwarding some SpdyStream::Delegate methods.
  class Delegate {
   public:
    virtual ~Delegate() = default;
    virtual void OnHeadersSent() = 0;
    virtual void OnHeadersReceived(const SpdyHeaderBlock& response_headers) = 0;
    // Might destroy |this|.
    virtual void OnClose(int status) = 0;
  };

  WebSocketSpdyStreamAdapter(base::WeakPtr<SpdyStream> stream,
                             Delegate* delegate,
                             NetLogWithSource net_log);
  ~WebSocketSpdyStreamAdapter() override;

  // Called by Delegate.
  void DetachDelegate();

  // WebSocketBasicStream::Adapter methods.
  int Read(IOBuffer* buf,
           int buf_len,
           const CompletionCallback& callback) override;
  int Write(IOBuffer* buf,
            int buf_len,
            const CompletionCallback& callback,
            const NetworkTrafficAnnotationTag& traffic_annotation) override;
  void Disconnect() override;
  bool is_initialized() const override;

  // SpdyStream::Delegate methods.
  void OnHeadersSent() override;
  void OnHeadersReceived(const SpdyHeaderBlock& response_headers) override;
  void OnDataReceived(std::unique_ptr<SpdyBuffer> buffer) override;
  void OnDataSent() override;
  void OnTrailers(const SpdyHeaderBlock& trailers) override;
  void OnClose(int status) override;
  NetLogSource source_dependency() const override;

 private:
  int CopySavedReadDataIntoBuffer();

  base::WeakPtr<SpdyStream> stream_;
  // The error code corresponding to the reason for closing the stream.
  int stream_error_;
  Delegate* delegate_;
  CompletionOnceCallback read_callback_;
  IOBuffer* read_buffer_;
  int read_length_;
  std::list<std::unique_ptr<SpdyBuffer>> read_data_;
  CompletionOnceCallback write_callback_;
  int write_length_;
  NetLogWithSource net_log_;
};
}  // namespace net

#endif  // NET_WEBSOCKETS_WEBSOCKET_BASIC_STREAM_ADAPTERS_H_
