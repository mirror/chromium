// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/http/http_basic_stream.h"

#include "base/format_macros.h"
#include "base/metrics/histogram.h"
#include "base/stringprintf.h"
#include "net/base/io_buffer.h"
#include "net/base/net_errors.h"
#include "net/http/http_request_headers.h"
#include "net/http/http_request_info.h"
#include "net/http/http_stream_parser.h"
#include "net/http/http_util.h"
#include "net/socket/client_socket_handle.h"
#include "net/socket/client_socket_pool_base.h"

namespace net {

HttpBasicStream::HttpBasicStream(ClientSocketHandle* connection,
                                 HttpStreamParser* parser,
                                 bool using_proxy)
    : read_buf_(new GrowableIOBuffer()),
      parser_(parser),
      connection_(connection),
      using_proxy_(using_proxy),
      request_info_(NULL),
      response_(NULL),
      bytes_read_offset_(0) {
}

HttpBasicStream::~HttpBasicStream() {}

int HttpBasicStream::InitializeStream(const HttpRequestInfo* request_info,
                                      const BoundNetLog& net_log,
                                      CompletionCallback* callback) {
  DCHECK(!parser_.get());
  request_info_ = request_info;
  parser_.reset(new HttpStreamParser(connection_.get(), request_info,
                                     read_buf_, net_log));
  bytes_read_offset_ = connection_->socket()->NumBytesRead();
  return OK;
}


int HttpBasicStream::SendRequest(const HttpRequestHeaders& headers,
                                 UploadDataStream* request_body,
                                 HttpResponseInfo* response,
                                 CompletionCallback* callback) {
  DCHECK(parser_.get());
  DCHECK(request_info_);
  const std::string path = using_proxy_ ?
                           HttpUtil::SpecForRequest(request_info_->url) :
                           HttpUtil::PathForRequest(request_info_->url);
  request_line_ = base::StringPrintf("%s %s HTTP/1.1\r\n",
                                     request_info_->method.c_str(),
                                     path.c_str());
  response_ = response;
  return parser_->SendRequest(request_line_, headers, request_body, response,
                              callback);
}

uint64 HttpBasicStream::GetUploadProgress() const {
  return parser_->GetUploadProgress();
}

int HttpBasicStream::ReadResponseHeaders(CompletionCallback* callback) {
  return parser_->ReadResponseHeaders(callback);
}

const HttpResponseInfo* HttpBasicStream::GetResponseInfo() const {
  return parser_->GetResponseInfo();
}

int HttpBasicStream::ReadResponseBody(IOBuffer* buf, int buf_len,
                                      CompletionCallback* callback) {
  return parser_->ReadResponseBody(buf, buf_len, callback);
}

void HttpBasicStream::Close(bool not_reusable) {
  parser_->Close(not_reusable);
}

HttpStream* HttpBasicStream::RenewStreamForAuth() {
  DCHECK(IsResponseBodyComplete());
  DCHECK(!IsMoreDataBuffered());
  parser_.reset();
  return new HttpBasicStream(connection_.release(), NULL, using_proxy_);
}

bool HttpBasicStream::IsResponseBodyComplete() const {
  return parser_->IsResponseBodyComplete();
}

bool HttpBasicStream::CanFindEndOfResponse() const {
  return parser_->CanFindEndOfResponse();
}

bool HttpBasicStream::IsMoreDataBuffered() const {
  return parser_->IsMoreDataBuffered();
}

bool HttpBasicStream::IsConnectionReused() const {
  return parser_->IsConnectionReused();
}

void HttpBasicStream::SetConnectionReused() {
  parser_->SetConnectionReused();
}

bool HttpBasicStream::IsConnectionReusable() const {
  return parser_->IsConnectionReusable();
}

void HttpBasicStream::GetSSLInfo(SSLInfo* ssl_info) {
  parser_->GetSSLInfo(ssl_info);
}

void HttpBasicStream::GetSSLCertRequestInfo(
    SSLCertRequestInfo* cert_request_info) {
  parser_->GetSSLCertRequestInfo(cert_request_info);
}

bool HttpBasicStream::IsSpdyHttpStream() const {
  return false;
}

void HttpBasicStream::LogNumRttVsBytesMetrics() const {
  int socket_reuse_policy = GetSocketReusePolicy();
  if (socket_reuse_policy > 2 || socket_reuse_policy < 0) {
    return;
  }

  int64 total_bytes_read = connection_->socket()->NumBytesRead();
  int64 bytes_received = total_bytes_read - bytes_read_offset_;
  int64 num_kb = bytes_received / 1024;
  double rtt = connection_->socket()->GetConnectTimeMicros().ToInternalValue();
  rtt /= 1000.0;

  if (num_kb < 1024 && rtt > 0) {  // Ignore responses > 1MB
    base::TimeDelta duration = base::Time::Now() -
                               response_->request_time;
    double num_rtt = static_cast<double>(duration.InMilliseconds()) / rtt;
    int64 num_rtt_scaled = (4 * num_rtt);

    static const char* const kGroups[] = {
      "warmest_socket", "warm_socket", "last_accessed_socket"
    };
    int bucket = (num_kb / 5) * 5;
    const std::string histogram(StringPrintf("Net.Num_RTT_vs_KB_%s_%dKB",
                                             kGroups[socket_reuse_policy],
                                             bucket));
    base::Histogram* counter = base::Histogram::FactoryGet(
        histogram, 0, 1000, 2, base::Histogram::kUmaTargetedHistogramFlag);
    DCHECK_EQ(histogram, counter->histogram_name());
    counter->Add(num_rtt_scaled);

    VLOG(2) << StringPrintf("%s\nrtt = %f\tnum_rtt = %f\t"
                            "num_kb = %" PRId64 "\t"
                            "total bytes = %" PRId64 "\t"
                            "histogram = %s",
                            request_line_.data(),
                            rtt, num_rtt, num_kb, total_bytes_read,
                            histogram.data());
  }
}

}  // namespace net
