// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/cronet/cronet_url_request.h"

#include <limits>
#include <utility>
#include <vector>

#include "base/bind.h"
#include "base/location.h"
#include "base/logging.h"
#include "components/cronet/cronet_url_request_context.h"
#include "net/base/load_flags.h"
#include "net/base/load_states.h"
#include "net/base/net_errors.h"
#include "net/base/request_priority.h"
#include "net/cert/cert_status_flags.h"
#include "net/http/http_response_headers.h"
#include "net/http/http_status_code.h"
#include "net/http/http_util.h"
#include "net/proxy/proxy_server.h"
#include "net/quic/core/quic_packets.h"
#include "net/ssl/ssl_info.h"
#include "net/url_request/redirect_info.h"
#include "net/url_request/url_request_context.h"

namespace cronet {

namespace {

// Returns the string representation of the HostPortPair of the proxy server
// that was used to fetch the response.
std::string GetProxy(const net::HttpResponseInfo& info) {
  if (!info.proxy_server.is_valid() || info.proxy_server.is_direct())
    return net::HostPortPair().ToString();
  return info.proxy_server.host_port_pair().ToString();
}

}  // namespace

CronetURLRequest::CronetURLRequest(CronetURLRequestContext* context,
                                   Delegate* delegate,
                                   const GURL& url,
                                   net::RequestPriority priority,
                                   bool disable_cache,
                                   bool disable_connection_migration,
                                   bool enable_metrics)
    : context_(context),
      delegate_(delegate),
      initial_url_(url),
      initial_priority_(priority),
      initial_method_("GET"),
      load_flags_(net::LOAD_NORMAL),
      enable_metrics_(enable_metrics),
      metrics_reported_(false) {
  DCHECK(!context_->IsOnNetworkThread());
  if (disable_cache)
    load_flags_ |= net::LOAD_DISABLE_CACHE;
  if (disable_connection_migration)
    load_flags_ |= net::LOAD_DISABLE_CONNECTION_MIGRATION;
}

CronetURLRequest::~CronetURLRequest() {
  DCHECK(context_->IsOnNetworkThread());
}

bool CronetURLRequest::SetHttpMethod(const std::string& method) {
  DCHECK(!context_->IsOnNetworkThread());
  // Http method is a token, just as header name.
  if (!net::HttpUtil::IsValidHeaderName(method))
    return false;
  initial_method_ = method;
  return true;
}

bool CronetURLRequest::AddRequestHeader(const std::string& name,
                                        const std::string& value) {
  DCHECK(!context_->IsOnNetworkThread());
  if (!net::HttpUtil::IsValidHeaderName(name) ||
      !net::HttpUtil::IsValidHeaderValue(value)) {
    return false;
  }
  initial_request_headers_.SetHeader(name, value);
  return true;
}

void CronetURLRequest::SetUpload(
    std::unique_ptr<net::UploadDataStream> upload) {
  DCHECK(!context_->IsOnNetworkThread());
  DCHECK(!upload_);
  upload_ = std::move(upload);
}

void CronetURLRequest::Start() {
  DCHECK(!context_->IsOnNetworkThread());
  context_->PostTaskToNetworkThread(
      FROM_HERE, base::Bind(&CronetURLRequest::StartOnNetworkThread,
                            base::Unretained(this)));
}

void CronetURLRequest::GetStatus() const {
  context_->PostTaskToNetworkThread(
      FROM_HERE, base::Bind(&CronetURLRequest::GetStatusOnNetworkThread,
                            base::Unretained(this)));
}

void CronetURLRequest::FollowDeferredRedirect() {
  context_->PostTaskToNetworkThread(
      FROM_HERE,
      base::Bind(&CronetURLRequest::FollowDeferredRedirectOnNetworkThread,
                 base::Unretained(this)));
}

bool CronetURLRequest::ReadData(net::IOBuffer* raw_read_buffer,
                                int max_size) {
  scoped_refptr<net::IOBuffer> read_buffer(raw_read_buffer);
  context_->PostTaskToNetworkThread(
      FROM_HERE, base::Bind(&CronetURLRequest::ReadDataOnNetworkThread,
                            base::Unretained(this), read_buffer, max_size));
  return true;
}

void CronetURLRequest::Destroy(bool send_on_canceled) {
  // Destroy could be called from any thread, including network thread (if
  // posting task to executor throws an exception), but is posted, so |this|
  // is valid until calling task is complete. Destroy() is always called from
  // within a synchronized java block that guarantees no future posts to the
  // network thread with the adapter pointer.
  context_->PostTaskToNetworkThread(
      FROM_HERE, base::Bind(&CronetURLRequest::DestroyOnNetworkThread,
                            base::Unretained(this), send_on_canceled));
}

void CronetURLRequest::OnReceivedRedirect(
    net::URLRequest* request,
    const net::RedirectInfo& redirect_info,
    bool* defer_redirect) {
  DCHECK(context_->IsOnNetworkThread());
  delegate_->OnReceivedRedirect(
      redirect_info.new_url.spec(), redirect_info.status_code,
      request->response_headers()->GetStatusText(), request->response_headers(),
      request->response_info().was_cached,
      request->response_info().alpn_negotiated_protocol,
      GetProxy(request->response_info()), request->GetTotalReceivedBytes());
  *defer_redirect = true;
}

void CronetURLRequest::OnCertificateRequested(
    net::URLRequest* request,
    net::SSLCertRequestInfo* cert_request_info) {
  DCHECK(context_->IsOnNetworkThread());
  // Cronet does not support client certificates.
  request->ContinueWithCertificate(nullptr, nullptr);
}

void CronetURLRequest::OnSSLCertificateError(net::URLRequest* request,
                                             const net::SSLInfo& ssl_info,
                                             bool fatal) {
  DCHECK(context_->IsOnNetworkThread());
  request->Cancel();
  int net_error = net::MapCertStatusToNetError(ssl_info.cert_status);
  ReportError(request, net_error);
}

void CronetURLRequest::OnResponseStarted(net::URLRequest* request,
                                         int net_error) {
  DCHECK_NE(net::ERR_IO_PENDING, net_error);
  DCHECK(context_->IsOnNetworkThread());

  if (net_error != net::OK) {
    ReportError(request, net_error);
    return;
  }
  delegate_->OnResponseStarted(
      request->GetResponseCode(), request->response_headers()->GetStatusText(),
      request->response_headers(), request->response_info().was_cached,
      request->response_info().alpn_negotiated_protocol,
      GetProxy(request->response_info()));
}

void CronetURLRequest::OnReadCompleted(net::URLRequest* request,
                                       int bytes_read) {
  DCHECK(context_->IsOnNetworkThread());

  if (bytes_read < 0) {
    ReportError(request, bytes_read);
    return;
  }

  if (bytes_read == 0) {
    MaybeReportMetrics();
    delegate_->OnSucceeded(url_request_->GetTotalReceivedBytes());
  } else {
    delegate_->OnReadCompleted(read_buffer_, bytes_read,
                               request->GetTotalReceivedBytes());
  }
  // Free the read buffer.
  read_buffer_ = nullptr;
}

void CronetURLRequest::StartOnNetworkThread() {
  DCHECK(context_->IsOnNetworkThread());
  VLOG(1) << "Starting chromium request: "
          << initial_url_.possibly_invalid_spec().c_str()
          << " priority: " << RequestPriorityToString(initial_priority_);
  url_request_ = context_->GetURLRequestContext()->CreateRequest(
      initial_url_, net::DEFAULT_PRIORITY, this);
  url_request_->SetLoadFlags(load_flags_ | context_->default_load_flags());
  url_request_->set_method(initial_method_);
  url_request_->SetExtraRequestHeaders(initial_request_headers_);
  url_request_->SetPriority(initial_priority_);
  if (upload_)
    url_request_->set_upload(std::move(upload_));
  url_request_->Start();
}

void CronetURLRequest::GetStatusOnNetworkThread() const {
  DCHECK(context_->IsOnNetworkThread());
  int status = net::LOAD_STATE_IDLE;
  // |url_request_| is initialized in StartOnNetworkThread, and it is
  // never nulled. If it is null, it must be that StartOnNetworkThread
  // has not been called, pretend that we are in LOAD_STATE_IDLE.
  // See https://crbug.com/606872.
  if (url_request_)
    status = url_request_->GetLoadState().state;
  delegate_->OnStatus(status);
}

void CronetURLRequest::FollowDeferredRedirectOnNetworkThread() {
  DCHECK(context_->IsOnNetworkThread());
  url_request_->FollowDeferredRedirect();
}

void CronetURLRequest::ReadDataOnNetworkThread(
    scoped_refptr<net::IOBuffer> read_buffer,
    int buffer_size) {
  DCHECK(context_->IsOnNetworkThread());
  DCHECK(read_buffer);
  DCHECK(!read_buffer_);

  read_buffer_ = read_buffer;

  int result = url_request_->Read(read_buffer_.get(), buffer_size);
  // If IO is pending, wait for the URLRequest to call OnReadCompleted.
  if (result == net::ERR_IO_PENDING)
    return;

  OnReadCompleted(url_request_.get(), result);
}

void CronetURLRequest::DestroyOnNetworkThread(bool send_on_canceled) {
  DCHECK(context_->IsOnNetworkThread());
  MaybeReportMetrics();
  // Free the read buffer.
  // read_buffer_ = nullptr;
  if (send_on_canceled)
    delegate_->OnCanceled();
  delegate_->OnDestroyed();
  // delete this;
}

void CronetURLRequest::ReportError(net::URLRequest* request, int net_error) {
  DCHECK_NE(net::ERR_IO_PENDING, net_error);
  DCHECK_LT(net_error, 0);
  DCHECK_EQ(request, url_request_.get());
  // Free the read buffer.
  // read_buffer_ = nullptr;
  net::NetErrorDetails net_error_details;
  url_request_->PopulateNetErrorDetails(&net_error_details);
  VLOG(1) << "Error " << net::ErrorToString(net_error)
          << " on chromium request: " << initial_url_.possibly_invalid_spec();
  delegate_->OnError(net_error, net_error_details.quic_connection_error,
                     net::ErrorToString(net_error),
                     request->GetTotalReceivedBytes());
}

void CronetURLRequest::MaybeReportMetrics() {
  // If there was an exception while starting the CronetUrlRequest, there won't
  // be a native URLRequest. In this case, the caller gets the exception
  // immediately, and the onFailed callback isn't called, so don't report
  // metrics either.
  if (!enable_metrics_ || metrics_reported_ || !url_request_) {
    return;
  }
  metrics_reported_ = true;
  net::LoadTimingInfo metrics;
  url_request_->GetLoadTimingInfo(&metrics);
  delegate_->OnMetricsCollected(
      metrics.request_start_time, metrics.request_start,
      metrics.connect_timing.dns_start, metrics.connect_timing.dns_end,
      metrics.connect_timing.connect_start, metrics.connect_timing.connect_end,
      metrics.connect_timing.ssl_start, metrics.connect_timing.ssl_end,
      metrics.send_start, metrics.send_end, metrics.push_start,
      metrics.push_end, metrics.receive_headers_end, base::TimeTicks::Now(),
      metrics.socket_reused, url_request_->GetTotalSentBytes(),
      url_request_->GetTotalReceivedBytes());
}

}  // namespace cronet
