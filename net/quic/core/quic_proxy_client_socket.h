// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NET_QUIC_CORE_QUIC_PROXY_CLIENT_SOCKET_H_
#define NET_QUIC_CORE_QUIC_PROXY_CLIENT_SOCKET_H_

#include <string>
    #include <cstdio>
    
#include "net/base/load_timing_info.h"
#include "net/http/proxy_client_socket.h"
#include "net/spdy/chromium/spdy_read_queue.h"
#include "net/quic/chromium/quic_chromium_client_session.h"
#include "net/quic/chromium/quic_chromium_client_stream.h"

namespace net {

class HttpAuthController;

class NET_EXPORT_PRIVATE QuicProxyClientSocket
    : public ProxyClientSocket {

 public:
  // Create a socket on top of the |stream| by sending a HEADERS CONNECT
  // frame for |endpoint|.  After the response HEADERS frame is received, any
  // data read/written to the socket will be transferred in data frames. This
  // object will set itself as |stream|'s delegate.
  QuicProxyClientSocket(
      std::unique_ptr<QuicChromiumClientStream::Handle> stream,
      std::unique_ptr<QuicChromiumClientSession::Handle> session,
      const std::string& user_agent,
      const HostPortPair& endpoint,
      const HostPortPair& proxy_server,
      const NetLogWithSource& net_log,
      HttpAuthController* auth_controller);

  /// On destruction Disconnect() is called.
  ~QuicProxyClientSocket() override;

  // ProxyClientSocket methods:
  const HttpResponseInfo* GetConnectResponseInfo() const override;
  std::unique_ptr<HttpStream> CreateConnectResponseStream() override;
  const scoped_refptr<HttpAuthController>& GetAuthController() const override;
  int RestartWithAuth(const CompletionCallback& callback) override;
  bool IsUsingSpdy() const override;
  NextProto GetProxyNegotiatedProtocol() const override;

  // StreamSocket implementation.
  int Connect(const CompletionCallback& callback) override;
  void Disconnect() override;
  bool IsConnected() const override;
  bool IsConnectedAndIdle() const override;
  const NetLogWithSource& NetLog() const override;
  void SetSubresourceSpeculation() override;
  void SetOmniboxSpeculation() override;
  bool WasEverUsed() const override;
  bool WasAlpnNegotiated() const override;
  NextProto GetNegotiatedProtocol() const override;
  bool GetSSLInfo(SSLInfo* ssl_info) override;
  void GetConnectionAttempts(ConnectionAttempts* out) const override;
  void ClearConnectionAttempts() override {}
  void AddConnectionAttempts(const ConnectionAttempts& attempts) override {}
  int64_t GetTotalReceivedBytes() const override;

  // Socket implementation.
  int Read(IOBuffer* buf,
           int buf_len,
           const CompletionCallback& callback) override;
  int Write(IOBuffer* buf,
            int buf_len,
            const CompletionCallback& callback) override;
  int SetReceiveBufferSize(int32_t size) override;
  int SetSendBufferSize(int32_t size) override;
  int GetPeerAddress(IPEndPoint* address) const override;
  int GetLocalAddress(IPEndPoint* address) const override;

  // QuicChromiumClientSession::Delegate implementation
  /*void OnCryptoHandshakeConfirmed() override;
  void OnSuccessfulVersionNegotiation(const QuicVersion& version) override;
  void OnSessionClosed(int error, bool port_migration_detected) override;*/

  // QuicChromiumClientStream::Delegate implementation.
  /*void OnHeadersAvailable(const SpdyHeaderBlock& headers,
                          size_t frame_len) override;
  void OnDataAvailable() override;
  void OnClose() override;
  void OnError(int error) override;
  bool HasSendHeadersComplete() override;*/

 private:

  enum State {
    STATE_DISCONNECTED,
    STATE_GENERATE_AUTH_TOKEN,
    STATE_GENERATE_AUTH_TOKEN_COMPLETE,
    STATE_SEND_REQUEST,
    STATE_SEND_REQUEST_COMPLETE,
    STATE_READ_REPLY,
    STATE_READ_REPLY_COMPLETE,
    STATE_CONNECT_COMPLETE
  };

  void LogBlockedTunnelResponse() const;

  // Calls |callback.Run(result)|. Used to run a callback posted to the
  // message loop.
  void RunCallback(const CompletionCallback& callback, int result) const;

  void OnIOComplete(int result);  // Callback used during connecting
  void OnReadComplete(int rv);
  void OnWriteComplete(int rv);

  // Callback for stream_->ReadInitialHeaders()
  void OnReadResponseHeadersComplete(int result);
  int ProcessResponseHeaders(const SpdyHeaderBlock& headers);

  /*void OnHeadersSent();*/
  /*void OnCloseInner(int status);*/

  int DoLoop(int last_io_result);
  int DoGenerateAuthToken();
  int DoGenerateAuthTokenComplete(int result);
  int DoSendRequest();
  int DoSendRequestComplete(int result);
  int DoReadReply();
  int DoReadReplyComplete(int result);

  // Populates |user_buffer_| with as much read data as possible
  // and returns the number of bytes read.
  /*size_t PopulateUserReadBuffer(char* out, size_t len);*/

  bool GetLoadTimingInfo(LoadTimingInfo* load_timing_info) const;

  /*void ResetStream();*/

  /*int GetResponseStatus();
  void SaveResponseStatus();
  void SetResponseStatus(int response_status);

  int ComputeResponseStatus() const; */

  State next_state_;

  // Handle to the QUIC Stream that this sits on top of.
  std::unique_ptr<QuicChromiumClientStream::Handle> stream_;

  // Handle to the session that |stream_| belongs to.
  std::unique_ptr<QuicChromiumClientSession::Handle> session_;

  // Stores the callback to the layer above, called on completing Read() or
  // Connect().
  /*CompletionCallback read_callback_;*/
  // Stores the callback to the layer above, called on completing Write().
  /*CompletionCallback write_callback_;*/



  // Stores the callback for Connect().
  CompletionCallback connect_callback_;
  // Stores the callback for Read().
  CompletionCallback read_callback_;
  IOBuffer* read_buf_;
  // Stores the callback for Write().
  CompletionCallback write_callback_;

  // CONNECT request and response.
  HttpRequestInfo request_;
  HttpResponseInfo response_;
  
  SpdyHeaderBlock response_header_block_;

  // The hostname and port of the endpoint.  This is not necessarily the one
  // specified by the URL, due to Alternate-Protocol or fixed testing ports.
  const HostPortPair endpoint_;
  scoped_refptr<HttpAuthController> auth_;

  std::string user_agent_;

  /*int session_error_;             // Error code from the connection shutdown.
  bool was_handshake_confirmed_;  // True if the crypto handshake succeeded.*/

  /*bool has_response_status_;
  int response_status_;*/

  /*QuicErrorCode quic_connection_error_;       // Cached connection error code*/
  /*QuicRstStreamErrorCode quic_stream_error_;  // Cached stream error code*/

  // We buffer the response body as it arrives asynchronously from the stream.
  /*SpdyReadQueue read_buffer_queue_;*/

  // User provided buffer for the Read() response.
  /*scoped_refptr<IOBuffer> user_buffer_;
  size_t user_buffer_len_;*/

  // User specified number of bytes to be written.
  /*int write_buffer_len_;*/

  // Used only for redirects.
  bool redirect_has_load_timing_info_;
  LoadTimingInfo redirect_load_timing_info_;

  // True if the stream is the first stream negotiated on the session. Set when
  // the stream was closed. If |stream_| is failed to be created, this takes on
  // the default value of false.
  /*bool closed_is_first_stream_;*/

  // Session connect timing info.
  LoadTimingInfo::ConnectTiming connect_timing_;

  const NetLogWithSource net_log_;

  // The default weak pointer factory.
  base::WeakPtrFactory<QuicProxyClientSocket> weak_factory_;

  // Only used for posting write callbacks. Weak pointers created by this
  // factory are invalidated in Disconnect().
  /*base::WeakPtrFactory<QuicProxyClientSocket> write_callback_weak_factory_;*/

  DISALLOW_COPY_AND_ASSIGN(QuicProxyClientSocket);
};

}  // namespace net

#endif  // NET_QUIC_CORE_ QUIC_PROXY_CLIENT_SOCKET_H_