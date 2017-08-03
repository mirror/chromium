// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <cstdio>

#include "net/quic/core/quic_proxy_client_socket.h"

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/callback_helpers.h"
#include "base/values.h"
#include "net/http/http_auth_controller.h"
#include "net/http/http_response_headers.h"
#include "net/http/proxy_connect_redirect_http_stream.h"
#include "net/log/net_log_source.h"
#include "net/log/net_log_source_type.h"
#include "net/spdy/chromium/spdy_http_utils.h"


namespace net {

QuicProxyClientSocket::QuicProxyClientSocket(
    std::unique_ptr<QuicChromiumClientStream::Handle> stream,
    std::unique_ptr<QuicChromiumClientSession::Handle> session,
    const std::string& user_agent,
    const HostPortPair& endpoint,
    const HostPortPair& proxy_server,
    const NetLogWithSource& net_log,
    HttpAuthController* auth_controller)
    : next_state_(STATE_DISCONNECTED),
      stream_(std::move(stream)),
      session_(std::move(session)),
      read_buf_(nullptr),
      endpoint_(endpoint),
      auth_(auth_controller),
      user_agent_(user_agent),
      /*session_error_(ERR_UNEXPECTED),
      was_handshake_confirmed_(session->IsCryptoHandshakeConfirmed()),*/
      /*has_response_status_(false),
      response_status_(ERR_UNEXPECTED),*/
      /*quic_connection_error_(QUIC_NO_ERROR),
      quic_stream_error_(QUIC_STREAM_NO_ERROR),*/
      /*user_buffer_len_(0),*/
      /*write_buffer_len_(0),*/
      redirect_has_load_timing_info_(false),
      /*closed_is_first_stream_(false),*/
      net_log_(net_log),
      weak_factory_(this) {
      /*write_callback_weak_factory_(this) {*/
  DCHECK(stream_->IsOpen());

  request_.method = "CONNECT";
  request_.url = GURL("https://" + endpoint.ToString());

  net_log_.BeginEvent(NetLogEventType::SOCKET_ALIVE,
                      net_log_.source().ToEventParametersCallback());
  net_log_.AddEvent(
      NetLogEventType::HTTP2_PROXY_CLIENT_SESSION,
      stream_->net_log().source().ToEventParametersCallback());  // should this be QUIC?
}

QuicProxyClientSocket::~QuicProxyClientSocket() {
  Disconnect();
  net_log_.EndEvent(NetLogEventType::SOCKET_ALIVE);
}

const HttpResponseInfo* QuicProxyClientSocket::GetConnectResponseInfo() const {
  return response_.headers.get() ? &response_ : NULL;
}

std::unique_ptr<HttpStream>
QuicProxyClientSocket::CreateConnectResponseStream() {
  return base::MakeUnique<ProxyConnectRedirectHttpStream>(
      redirect_has_load_timing_info_ ? &redirect_load_timing_info_ : NULL);
}

const scoped_refptr<HttpAuthController>&
QuicProxyClientSocket::GetAuthController() const {
  return auth_;
}

int QuicProxyClientSocket::RestartWithAuth(const CompletionCallback& callback) {
  // A QUIC Stream can only handle a single request, so the underlying
  // stream may not be reused and a new QuicProxyClientSocket must be
  // created (possibly on top of the same QUIC Session).
  next_state_ = STATE_DISCONNECTED;
  return OK;
}

bool QuicProxyClientSocket::IsUsingSpdy() const {
  return true;
}

NextProto QuicProxyClientSocket::GetProxyNegotiatedProtocol() const {
  return kProtoQUIC;
}


// Sends a HEADERS frame to the proxy with a CONNECT request
// for the specified endpoint.  Waits for the server to send back
// a HEADERS frame.  OK will be returned if the status is 200.
// ERR_TUNNEL_CONNECTION_FAILED will be returned for any other status.
// In any of these cases, Read() may be called to retrieve the HTTP
// response body.  Any other return values should be considered fatal.
// TODO(rch): handle 407 proxy auth requested correctly, perhaps
// by creating a new stream for the subsequent request.
// TODO(rch): create a more appropriate error code to disambiguate
// the HTTPS Proxy tunnel failure from an HTTP Proxy tunnel failure.
int QuicProxyClientSocket::Connect(const CompletionCallback& callback) {
  DCHECK(connect_callback_.is_null());
  if (!stream_->IsOpen())
    return ERR_CONNECTION_CLOSED;

  if (next_state_ == STATE_CONNECT_COMPLETE)
    return OK;

  DCHECK_EQ(STATE_DISCONNECTED, next_state_);
  next_state_ = STATE_GENERATE_AUTH_TOKEN;

  int rv = DoLoop(OK);
  if (rv == ERR_IO_PENDING)
    connect_callback_ = callback;
  return rv;
}

void QuicProxyClientSocket::Disconnect() {
  /*session_error_ = ERR_ABORTED;   // this this right??????????*/
  /*SaveResponseStatus();*/

  /*read_buffer_queue_.Clear();
  user_buffer_ = NULL;
  user_buffer_len_ = 0;*/
  connect_callback_.Reset();
  read_callback_.Reset();
  read_buf_ = nullptr;
  write_callback_.Reset();

  /*write_buffer_len_ = 0;*/
  /*write_callback_.Reset();
  write_callback_weak_factory_.InvalidateWeakPtrs();*/

  next_state_ = STATE_DISCONNECTED;

  stream_->Reset(QUIC_STREAM_CANCELLED);
}

bool QuicProxyClientSocket::IsConnected() const {
  return next_state_ == STATE_CONNECT_COMPLETE && stream_->IsOpen();
}

bool QuicProxyClientSocket::IsConnectedAndIdle() const {
  // Not sure how to handle this!!!!!!!!!!!!!!!1 ;aoifje;ia;waiowejr;owo;eiro
  return IsConnected() /*&& read_buffer_queue_.IsEmpty()*/;
}

const NetLogWithSource& QuicProxyClientSocket::NetLog() const {
  return net_log_;
}

void QuicProxyClientSocket::SetSubresourceSpeculation() {
  // TODO(rch): what should this implementation be?
}

void QuicProxyClientSocket::SetOmniboxSpeculation() {
  // TODO(rch): what should this implementation be?
}

bool QuicProxyClientSocket::WasEverUsed() const {
  /*return was_ever_used_ || (stream_ && session_->WasEverUsed());*/
  return session_->WasEverUsed();
}

bool QuicProxyClientSocket::WasAlpnNegotiated() const {
  return false;
}

NextProto QuicProxyClientSocket::GetNegotiatedProtocol() const {
  return kProtoUnknown;
}

bool QuicProxyClientSocket::GetSSLInfo(SSLInfo* ssl_info) {
  /*return spdy_stream_->GetSSLInfo(ssl_info)*/;
  return session_->GetSSLInfo(ssl_info);
}

void QuicProxyClientSocket::GetConnectionAttempts(
    ConnectionAttempts* out) const {
  out->clear();
}

int64_t QuicProxyClientSocket::GetTotalReceivedBytes() const {
  NOTIMPLEMENTED();
  return 0;
}

int QuicProxyClientSocket::Read(IOBuffer* buf, int buf_len,
                                const CompletionCallback& callback) {
  /*DCHECK(!user_buffer_.get());*/
  DCHECK(connect_callback_.is_null());
  DCHECK(read_callback_.is_null());
  DCHECK(!read_buf_);
  
  /*if (!session_->IsConnected() || !stream_->IsOpen())
    return GetResponseStatus();*/

  if (next_state_ == STATE_DISCONNECTED)
    return ERR_SOCKET_NOT_CONNECTED;

  if (!stream_->IsOpen()) {   // is this condition correct?
    return 0;
  }

  /*DCHECK(next_state_ == STATE_CONNECT_COMPLETE || next_state_ == STATE_CLOSED);
  DCHECK(buf);
  size_t result = PopulateUserReadBuffer(buf->data(), buf_len);
  if (result == 0) {
    user_buffer_ = buf;
    user_buffer_len_ = static_cast<size_t>(buf_len);
    DCHECK(!callback.is_null());
    read_callback_ = callback;
    return ERR_IO_PENDING;
  }
  user_buffer_ = NULL;
  return result;*/

  int rv = stream_->ReadBody(
      buf, buf_len,
      base::Bind(&QuicProxyClientSocket::OnReadComplete,
                 weak_factory_.GetWeakPtr()));

  if (rv == ERR_IO_PENDING) {
    read_callback_ = callback;
    read_buf_ = buf;
  } else if (rv == 0) {
    net_log_.AddByteTransferEvent(NetLogEventType::SOCKET_BYTES_RECEIVED,
                                  0, nullptr);
  } else if (rv > 0) {
    net_log_.AddByteTransferEvent(NetLogEventType::SOCKET_BYTES_RECEIVED,
                                  rv, buf->data());
  }
  return rv;
}

void QuicProxyClientSocket::OnReadComplete(int rv) {
  if (!stream_->IsOpen())
    rv = 0;
  
  if (!read_callback_.is_null()) {
    DCHECK(read_buf_);
    if (rv >= 0) {
      net_log_.AddByteTransferEvent(NetLogEventType::SOCKET_BYTES_RECEIVED,
                                    rv, read_buf_->data());
    }
    CompletionCallback c = read_callback_;
    read_callback_.Reset();
    read_buf_ = nullptr;
    c.Run(rv);
  }
}

/*size_t QuicProxyClientSocket::PopulateUserReadBuffer(char* data, size_t len) {
  return read_buffer_queue_.Dequeue(data, len);
}*/

int QuicProxyClientSocket::Write(IOBuffer* buf, int buf_len,
                                 const CompletionCallback& callback) {
  DCHECK(connect_callback_.is_null());
  DCHECK(write_callback_.is_null());

  if (next_state_ != STATE_CONNECT_COMPLETE)
    return ERR_SOCKET_NOT_CONNECTED;


  //DCHECK(stream_);
  /*stream_->WriteOrbufferData(QuicStringPiece(buf->data(), buf_len), false,
                                  nullptr);*/
  
  net_log_.AddByteTransferEvent(NetLogEventType::SOCKET_BYTES_SENT, buf_len,
                                buf->data());

  int rv = stream_->WriteStreamData(
      QuicStringPiece(buf->data(), buf_len),
      false,
      base::Bind(&QuicProxyClientSocket::OnWriteComplete,
                 weak_factory_.GetWeakPtr()));

  if (rv == ERR_IO_PENDING)
    write_callback_ = callback;

  return rv;
}

void QuicProxyClientSocket::OnWriteComplete(int rv) {
  if (!write_callback_.is_null()) {
    CompletionCallback c = write_callback_;
    write_callback_.Reset();
    c.Run(rv);
  }
}

int QuicProxyClientSocket::SetReceiveBufferSize(int32_t size) {
  /* Valid for Quic??? */
  return ERR_NOT_IMPLEMENTED;
}

int QuicProxyClientSocket::SetSendBufferSize(int32_t size) {
  /* Valid for Quic???? */
  return ERR_NOT_IMPLEMENTED;
}

int QuicProxyClientSocket::GetPeerAddress(IPEndPoint* address) const {
  return IsConnected() ? session_->GetPeerAddress(address)
                       : ERR_SOCKET_NOT_CONNECTED;
}

int QuicProxyClientSocket::GetLocalAddress(IPEndPoint* address) const {
  return IsConnected() ? session_->GetSelfAddress(address)
                       : ERR_SOCKET_NOT_CONNECTED;
}

void QuicProxyClientSocket::LogBlockedTunnelResponse() const {
  ProxyClientSocket::LogBlockedTunnelResponse(
      response_.headers->response_code(),
      /* is_https_proxy = */ true);
}

void QuicProxyClientSocket::RunCallback(const CompletionCallback& callback,
                                        int result) const {
  callback.Run(result);
}

void QuicProxyClientSocket::OnIOComplete(int result) {
  DCHECK_NE(STATE_DISCONNECTED, next_state_);
  int rv = DoLoop(result);
  if (rv != ERR_IO_PENDING) {
    // Connect() finished (successfully or unsuccessfully)?
    DCHECK(!connect_callback_.is_null());
    CompletionCallback c = connect_callback_;
    connect_callback_.Reset();
    c.Run(rv);
  }
}

int QuicProxyClientSocket::DoLoop(int last_io_result) {
  DCHECK_NE(next_state_, STATE_DISCONNECTED);
  int rv = last_io_result;
  do {
    State state = next_state_;
    next_state_ = STATE_DISCONNECTED;
    switch (state) {
      case STATE_GENERATE_AUTH_TOKEN:
        DCHECK_EQ(OK, rv);
        rv = DoGenerateAuthToken();
        break;
      case STATE_GENERATE_AUTH_TOKEN_COMPLETE:
        rv = DoGenerateAuthTokenComplete(rv);
        break;
      case STATE_SEND_REQUEST:
        DCHECK_EQ(OK, rv);
        net_log_.BeginEvent(
            NetLogEventType::HTTP_TRANSACTION_TUNNEL_SEND_REQUEST);
        rv = DoSendRequest();
        break;
      case STATE_SEND_REQUEST_COMPLETE:
        net_log_.EndEventWithNetErrorCode(
            NetLogEventType::HTTP_TRANSACTION_TUNNEL_SEND_REQUEST, rv);
        rv = DoSendRequestComplete(rv);
        if (rv >= 0 || rv == ERR_IO_PENDING) {
          // Emit extra event so can use the same events as
          // HttpProxyClientSocket.
          net_log_.BeginEvent(
              NetLogEventType::HTTP_TRANSACTION_TUNNEL_READ_HEADERS);
        }
        break;
      case STATE_READ_REPLY:
        rv = DoReadReply();
        break;
      case STATE_READ_REPLY_COMPLETE:
        rv = DoReadReplyComplete(rv);
        net_log_.EndEventWithNetErrorCode(
            NetLogEventType::HTTP_TRANSACTION_TUNNEL_READ_HEADERS, rv);
        break;
      default:
        NOTREACHED() << "bad state";
        rv = ERR_UNEXPECTED;
        break;
    }
  } while (rv != ERR_IO_PENDING && next_state_ != STATE_DISCONNECTED &&
           next_state_ != STATE_CONNECT_COMPLETE);
  return rv;
}

int QuicProxyClientSocket::DoGenerateAuthToken() {
  next_state_ = STATE_GENERATE_AUTH_TOKEN_COMPLETE;
  return auth_->MaybeGenerateAuthToken(
      &request_,
      base::Bind(&QuicProxyClientSocket::OnIOComplete,
                 weak_factory_.GetWeakPtr()),
      net_log_);
}

int QuicProxyClientSocket::DoGenerateAuthTokenComplete(int result) {
  DCHECK_NE(ERR_IO_PENDING, result);
  if (result == OK)
    next_state_ = STATE_SEND_REQUEST;
  return result;
}

int QuicProxyClientSocket::DoSendRequest() {
  next_state_ = STATE_SEND_REQUEST_COMPLETE;

  // Add Proxy-Authentication header if necessary.
  HttpRequestHeaders authorization_headers;
  if (auth_->HaveAuth()) {
    auth_->AddAuthorizationHeader(&authorization_headers);
  }

  std::string request_line;
  BuildTunnelRequest(endpoint_, authorization_headers, user_agent_,
                     &request_line, &request_.extra_headers);

  net_log_.AddEvent(
      NetLogEventType::HTTP_TRANSACTION_SEND_TUNNEL_HEADERS,
      base::Bind(&HttpRequestHeaders::NetLogCallback,
                 base::Unretained(&request_.extra_headers), &request_line));

  SpdyHeaderBlock headers;
  CreateSpdyHeadersFromHttpRequest(request_, request_.extra_headers, true,
                                   &headers);

  /*return spdy_stream_->SendRequestHeaders(std::move(headers),
                                          MORE_DATA_TO_SEND);*/
  
/*printf("\n\nQuicProxyClientSocket::DoSendRequest()");
printf("%s\n", headers.DebugString().c_str());*/

  return stream_->WriteHeaders(std::move(headers), false, nullptr);
}

int QuicProxyClientSocket::DoSendRequestComplete(int result) {
  if (result < 0)
    return result;

  // Wait for HEADERS frame from the server
  next_state_ = STATE_READ_REPLY;   //STATE_READ_REPLY_COMPLETE;
  return OK;
}


int QuicProxyClientSocket::DoReadReply() {
  next_state_ = STATE_READ_REPLY_COMPLETE;
  
  int rv = stream_->ReadInitialHeaders(&response_header_block_,
      base::Bind(&QuicProxyClientSocket::OnReadResponseHeadersComplete,
                 weak_factory_.GetWeakPtr()));
  if (rv == ERR_IO_PENDING) {
    return ERR_IO_PENDING;
  }
  if (rv < 0)
    return rv;

  return ProcessResponseHeaders(response_header_block_);
}


int QuicProxyClientSocket::DoReadReplyComplete(int result) {
  // We enter this method directly from DoSendRequestComplete, since
  // we are notified by a callback when the HEADERS frame arrives.
  // IS THIS TRUE STILL?????? prolly not?

  if (result < 0)
    return result;

  // Require the "HTTP/1.x" status line for SSL CONNECT.
  if (response_.headers->GetHttpVersion() < HttpVersion(1, 0))
    return ERR_TUNNEL_CONNECTION_FAILED;

  net_log_.AddEvent(
      NetLogEventType::HTTP_TRANSACTION_READ_TUNNEL_RESPONSE_HEADERS,
      base::Bind(&HttpResponseHeaders::NetLogCallback, response_.headers));

  switch (response_.headers->response_code()) {
    case 200:  // OK
      next_state_ = STATE_CONNECT_COMPLETE;
      return OK;

    case 302:  // Found / Moved Temporarily
      // Try to return a sanitized response so we can follow auth redirects.
      // If we can't, fail the tunnel connection.
      if (!SanitizeProxyRedirect(&response_)) {
        LogBlockedTunnelResponse();
        return ERR_TUNNEL_CONNECTION_FAILED;
      }

      redirect_has_load_timing_info_ =
          GetLoadTimingInfo(&redirect_load_timing_info_);
          /*spdy_stream_->GetLoadTimingInfo(&redirect_load_timing_info_);*/
      // Note that this triggers a ERROR_CODE_CANCEL.
      //spdy_stream_->DetachDelegate();
      /*stream_->SetDelegate(nullptr);*/
      next_state_ = STATE_DISCONNECTED;
      return ERR_HTTPS_PROXY_TUNNEL_RESPONSE;

    case 407:  // Proxy Authentication Required
      next_state_ = STATE_CONNECT_COMPLETE;
      if (!SanitizeProxyAuth(&response_)) {
        LogBlockedTunnelResponse();
        return ERR_TUNNEL_CONNECTION_FAILED;
      }
      return HandleProxyAuthChallenge(auth_.get(), &response_, net_log_);

    default:
      // Ignore response to avoid letting the proxy impersonate the target
      // server.  (See http://crbug.com/137891.)
      LogBlockedTunnelResponse();
      return ERR_TUNNEL_CONNECTION_FAILED;
  }
}

/*void QuicProxyClientSocket::OnCryptoHandshakeConfirmed() {
  was_handshake_confirmed_ = true;
}*/

/*void QuicProxyClientSocket::OnSuccessfulVersionNegotiation(
    const QuicVersion& version) {

}*/
/*
void QuicProxyClientSocket::OnSessionClosed(int error,
                                            bool port_migration_detected) {
  session_error_ = error;
  SaveResponseStatus();

  Disconnect();
  session_.reset();
}*/

/*void QuicProxyClientSocket::OnHeadersAvailable(
    const SpdyHeaderBlock& headers, size_t frame_len) {
  // If we've already received the reply, existing headers are too late.
  // TODO(mbelshe): figure out a way to make HEADERS frames useful after the
  //                initial response.
  if (next_state_ != STATE_READ_REPLY_COMPLETE)
    return;

  // Save the response
  const bool headers_valid =
      SpdyHeadersToHttpResponse(headers, &response_);
  DCHECK(headers_valid);

  connect_timing_ = session_->GetConnectTiming();

  OnIOComplete(OK);
}*/

void QuicProxyClientSocket::OnReadResponseHeadersComplete(int result) {
  // Convert the now-populated SpdyHeaderBlock to HttpResponseInfo
  if (result > 0) {
    result = ProcessResponseHeaders(response_header_block_);
  }

  if (result != ERR_IO_PENDING) {
    OnIOComplete(result);
  }
}

int QuicProxyClientSocket::ProcessResponseHeaders(
    const SpdyHeaderBlock& headers) {
  if (!SpdyHeadersToHttpResponse(headers, &response_)) {
    DLOG(WARNING) << "Invalid headers";
    return ERR_QUIC_PROTOCOL_ERROR;
  }
  // Populate |connect_timing_| when response headers are received. This
  // should take care of 0-RTT where request is sent before handshake is
  // confirmed.
  connect_timing_ = session_->GetConnectTiming();
  return OK;  
}

/*// Called when data is received or on EOF (if |buffer| is NULL).
void QuicProxyClientSocket::OnDataReceived(std::unique_ptr<SpdyBuffer> buffer) {
  if (buffer) {
    net_log_.AddByteTransferEvent(NetLogEventType::SOCKET_BYTES_RECEIVED,
                                  buffer->GetRemainingSize(),
                                  buffer->GetRemainingData());
    read_buffer_queue_.Enqueue(std::move(buffer));
  } else {
    net_log_.AddByteTransferEvent(NetLogEventType::SOCKET_BYTES_RECEIVED, 0,
                                  NULL);
  }

  if (!read_callback_.is_null()) {
    int rv = PopulateUserReadBuffer(user_buffer_->data(), user_buffer_len_);
    CompletionCallback c = read_callback_;
    read_callback_.Reset();
    user_buffer_ = NULL;
    user_buffer_len_ = 0;
    c.Run(rv);
  }
}*/
/*void QuicProxyClientSocket::OnDataAvailable() {
  struct iovec iov;
  bool enqueued_data = false;
  while (true) {
    if (stream_->sequencer()->GetReadableRegions(&iov, 1) != 1) {
      // No more data to read.
      break;
    }
    std::unique_ptr<SpdyBuffer> buffer(
        new SpdyBuffer(static_cast<char*>(iov.iov_base), iov.iov_len));
    net_log_.AddByteTransferEvent(NetLogEventType::SOCKET_BYTES_RECEIVED,
                                  buffer->GetRemainingSize(),
                                  buffer->GetRemainingData());
    read_buffer_queue_.Enqueue(std::move(buffer));
    enqueued_data = true;
  }
  if (!enqueued_data) {
    net_log_.AddByteTransferEvent(NetLogEventType::SOCKET_BYTES_RECEIVED, 0,
                                  nullptr);
  }

  if (!read_callback_.is_null()) {
    int rv = PopulateUserReadBuffer(user_buffer_->data(), user_buffer_len_);
    CompletionCallback c = read_callback_;
    read_callback_.Reset();
    user_buffer_ = NULL;
    user_buffer_len_ = 0;
    c.Run(rv);
  }
}*/
#if 0
void QuicProxyClientSocket::OnClose() {
  /*quic_connection_error_ = stream_->connection_error();*/
  /*quic_stream_error_ = stream_->stream_error();*/
  SaveResponseStatus();

  OnCloseInner(GetResponseStatus());
}

void QuicProxyClientSocket::OnError(int error) {
  /*session_error_ = error;*/
  SaveResponseStatus();

  OnCloseInner(GetResponseStatus());
}

bool QuicProxyClientSocket::HasSendHeadersComplete() {
  return next_state_ > STATE_SEND_REQUEST_COMPLETE;
}
#endif

bool QuicProxyClientSocket::GetLoadTimingInfo(
    LoadTimingInfo* load_timing_info) const {
  bool is_first_stream = stream_->IsFirstStream();
  if (stream_)
    is_first_stream = stream_->IsFirstStream();
  if (is_first_stream) {
    load_timing_info->socket_reused = false;
    load_timing_info->connect_timing = connect_timing_;
  } else {
    load_timing_info->socket_reused = true;
  }
  return true;
}

/*void QuicProxyClientSocket::OnHeadersSent() {
  DCHECK_EQ(next_state_, STATE_SEND_REQUEST_COMPLETE);

  OnIOComplete(OK);
}*/

/*void QuicProxyClientSocket::OnDataSent(int result)  {
  DCHECK(!write_callback_.is_null());

  // Proxy write callbacks result in deep callback chains. Post to allow the
  // stream's write callback chain to unwind (see crbug.com/355511).
  base::ThreadTaskRunnerHandle::Get()->PostTask(
      FROM_HERE, base::Bind(&QuicProxyClientSocket::RunCallback,
                            write_callback_weak_factory_.GetWeakPtr(),
                            base::ResetAndReturn(&write_callback_), result));
}*/

#if 0
void QuicProxyClientSocket::OnCloseInner(int status)  {
  /*was_ever_used_ = session_->WasEverUsed();
  closed_is_first_stream_ = stream_->IsFirstStream();
  stream_ = nullptr;*/

  bool connecting = next_state_ != STATE_DISCONNECTED &&
      next_state_ < STATE_CONNECT_COMPLETE;
  if (next_state_ == STATE_CONNECT_COMPLETE)
    next_state_ = STATE_CLOSED;
  else
    next_state_ = STATE_DISCONNECTED;

  base::WeakPtr<QuicProxyClientSocket> weak_ptr = weak_factory_.GetWeakPtr();
  /*CompletionCallback write_callback = write_callback_;
  write_callback_.Reset();*/
  /*write_buffer_len_ = 0;*/

  // If we're in the middle of connecting, we need to make sure
  // we invoke the connect callback.
  if (connecting) {
    DCHECK(!connect_callback_.is_null());
    CompletionCallback connect_callback = connect_callback_;
    connect_callback_.Reset();
    connect_callback.Run(status);
// just return connection_closed here?
  }/* else if (!read_callback_.is_null()) {
    // If we have a read_callback_, the we need to make sure we call it back.
    OnDataReceived(std::unique_ptr<SpdyBuffer>());
    int rv = PopulateUserReadBuffer(user_buffer_->data(), user_buffer_len_);
    CompletionCallback read_callback = read_callback_;
    read_callback_.Reset();
    user_buffer_ = NULL;
    user_buffer_len_ = 0;
    read_callback.Run(rv);
  }*/
  // This may have been deleted by read_callback_, so check first.
  /*if (weak_ptr.get() && !write_callback.is_null())
    write_callback.Run(ERR_CONNECTION_CLOSED);*/
}
#endif

/*void QuicProxyClientSocket::OnClose(int status) {
  next_state_ = (next_state_ == STATE_CONNECT_COMPLETE) ? STATE_CLOSED : STATE_DISCONNECTED;

  // If we're in the middle of connecting, we need to make sure we iknvoke the
  // connect callback.
  bool connecting = next_state_ != STATE_DISCONNECTED &&
                    next_state_ < STATE_CONNECT_COMPLETE;
  if (connecting) {
    DCHECK(!connect_callback_.is_null());
    CompletionCallback connect_callback = connect_callback_;
    connect_callback_.Reset();
    connect_callback.Run(status);
  }
}*/

#if 0
int QuicProxyClientSocket::GetResponseStatus() {
  SaveResponseStatus();
  return response_status_;
}

void QuicProxyClientSocket::SaveResponseStatus() {
  if (!has_response_status_)
    SetResponseStatus(ComputeResponseStatus());
}

void QuicProxyClientSocket::SetResponseStatus(int response_status) {
  has_response_status_ = true;
  response_status_ = response_status;
}

// Copied from QuicHttpStream
int QuicProxyClientSocket::ComputeResponseStatus() const {
  DCHECK(!has_response_status_);
  
  if (!session_->IsCryptoHandshakeConfirmed())
    return ERR_QUIC_HANDSHAKE_FAILED;

  // If the session was aborted by a higher layer, simply use that error code.
  if (session_->GetError() != ERR_UNEXPECTED)
    return session_->GetError();

  /*// A request has not yet been sent.
  if (next_state_ != STATE_DISCONNECTED && next_state_ < STATE_SEND_REQUEST_COMPLETE)
    return ERR_CONNECTION_CLOSED;*/

  // Explicit stream errors are always fatal.
  if (stream_->stream_error() != QUIC_STREAM_NO_ERROR &&
      stream_->stream_error() != QUIC_STREAM_CONNECTION_ERROR) {
    return ERR_QUIC_PROTOCOL_ERROR;
  }

  DCHECK_NE(QUIC_HANDSHAKE_TIMEOUT, stream_->connection_error());

  return ERR_QUIC_PROTOCOL_ERROR;
}
#endif

}  // namespace net
