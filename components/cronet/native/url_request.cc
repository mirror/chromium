// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/cronet/native/url_request.h"

#include <utility>

#include "base/logging.h"
#include "base/macros.h"
#include "components/cronet/native/engine.h"
#include "components/cronet/native/generated/cronet.idl_impl_struct.h"
#include "components/cronet/native/include/cronet_c.h"
#include "components/cronet/native/runnables.h"
#include "net/base/io_buffer.h"

namespace {

net::RequestPriority ConvertRequestPriority(
    Cronet_UrlRequestParams_REQUEST_PRIORITY priority) {
  switch (priority) {
    case Cronet_UrlRequestParams_REQUEST_PRIORITY_REQUEST_PRIORITY_IDLE:
      return net::IDLE;
    case Cronet_UrlRequestParams_REQUEST_PRIORITY_REQUEST_PRIORITY_LOWEST:
      return net::LOWEST;
    case Cronet_UrlRequestParams_REQUEST_PRIORITY_REQUEST_PRIORITY_LOW:
      return net::LOW;
    case Cronet_UrlRequestParams_REQUEST_PRIORITY_REQUEST_PRIORITY_MEDIUM:
      return net::MEDIUM;
    case Cronet_UrlRequestParams_REQUEST_PRIORITY_REQUEST_PRIORITY_HIGHEST:
      return net::HIGHEST;
  }
  return net::DEFAULT_PRIORITY;
}

std::unique_ptr<Cronet_UrlResponseInfo> ConvertCronet_UrlResponseInfo(
    int http_status_code,
    const std::string& http_status_text,
    const net::HttpResponseHeaders* headers,
    bool was_cached,
    const std::string& negotiated_protocol,
    const std::string& proxy_server) {
  auto respone_info = std::make_unique<Cronet_UrlResponseInfo>();
  respone_info->url = "";
  respone_info->urlChain.push_back(respone_info->url);
  respone_info->httpStatusCode = http_status_code;
  // |headers| could be nullptr.
  if (headers != nullptr) {
    size_t iter = 0;
    std::string header_name;
    std::string header_value;
    while (headers->EnumerateHeaderLines(&iter, &header_name, &header_value)) {
      auto header = std::make_unique<Cronet_HttpHeader>();
      header->name = header_name;
      header->value = header_value;
      respone_info->allHeadersList.push_back(std::move(header));
    }
  }
  respone_info->wasCached = was_cached;
  respone_info->negotiatedProtocol = negotiated_protocol;
  respone_info->proxyServer = proxy_server;
  return respone_info;
}

Cronet_Error_ERROR_CODE NetErrorToCronetErrorCode(int net_error) {
  switch (net_error) {
    case net::ERR_NAME_NOT_RESOLVED:
      return Cronet_Error_ERROR_CODE_ERROR_HOSTNAME_NOT_RESOLVED;
    case net::ERR_INTERNET_DISCONNECTED:
      return Cronet_Error_ERROR_CODE_ERROR_INTERNET_DISCONNECTED;
    case net::ERR_NETWORK_CHANGED:
      return Cronet_Error_ERROR_CODE_ERROR_NETWORK_CHANGED;
    case net::ERR_TIMED_OUT:
      return Cronet_Error_ERROR_CODE_ERROR_TIMED_OUT;
    case net::ERR_CONNECTION_CLOSED:
      return Cronet_Error_ERROR_CODE_ERROR_CONNECTION_CLOSED;
    case net::ERR_CONNECTION_TIMED_OUT:
      return Cronet_Error_ERROR_CODE_ERROR_CONNECTION_TIMED_OUT;
    case net::ERR_CONNECTION_REFUSED:
      return Cronet_Error_ERROR_CODE_ERROR_CONNECTION_REFUSED;
    case net::ERR_CONNECTION_RESET:
      return Cronet_Error_ERROR_CODE_ERROR_CONNECTION_RESET;
    case net::ERR_ADDRESS_UNREACHABLE:
      return Cronet_Error_ERROR_CODE_ERROR_ADDRESS_UNREACHABLE;
    case net::ERR_QUIC_PROTOCOL_ERROR:
      return Cronet_Error_ERROR_CODE_ERROR_QUIC_PROTOCOL_FAILED;
    default:
      return Cronet_Error_ERROR_CODE_ERROR_OTHER;
  }
}

bool IsCronetErrorImmediatelyRetryable(Cronet_Error_ERROR_CODE error_code) {
  switch (error_code) {
    case Cronet_Error_ERROR_CODE_ERROR_HOSTNAME_NOT_RESOLVED:
    case Cronet_Error_ERROR_CODE_ERROR_INTERNET_DISCONNECTED:
    case Cronet_Error_ERROR_CODE_ERROR_CONNECTION_REFUSED:
    case Cronet_Error_ERROR_CODE_ERROR_ADDRESS_UNREACHABLE:
    case Cronet_Error_ERROR_CODE_ERROR_OTHER:
    default:
      return false;
    case Cronet_Error_ERROR_CODE_ERROR_NETWORK_CHANGED:
    case Cronet_Error_ERROR_CODE_ERROR_TIMED_OUT:
    case Cronet_Error_ERROR_CODE_ERROR_CONNECTION_CLOSED:
    case Cronet_Error_ERROR_CODE_ERROR_CONNECTION_TIMED_OUT:
    case Cronet_Error_ERROR_CODE_ERROR_CONNECTION_RESET:
      return true;
  }
}

std::unique_ptr<Cronet_Error> ConvertCronet_Error(
    int net_error,
    int quic_error,
    const std::string& error_string) {
  auto error = std::make_unique<Cronet_Error>();
  error->errorCode = NetErrorToCronetErrorCode(net_error);
  error->message = error_string;
  error->internalErrorCode = net_error;
  error->quicDetailedErrorCode = quic_error;
  error->immediatelyRetryable =
      IsCronetErrorImmediatelyRetryable(error->errorCode);
  return error;
}

}  // namespace

namespace cronet {

Cronet_UrlRequestImpl::Cronet_UrlRequestImpl() {}

Cronet_UrlRequestImpl::~Cronet_UrlRequestImpl() {
  base::AutoLock lock(request_lock_);
  // Request may already be destroyed if it hasn't started or got canceled.
  if (request_)
    request_->Destroy(false);
}

void Cronet_UrlRequestImpl::SetContext(Cronet_UrlRequestContext context) {
  url_request_context_ = context;
}

Cronet_UrlRequestContext Cronet_UrlRequestImpl::GetContext() {
  return url_request_context_;
}

void Cronet_UrlRequestImpl::InitWithParams(
    Cronet_EnginePtr engine,
    CharString url,
    Cronet_UrlRequestParamsPtr params,
    Cronet_UrlRequestCallbackPtr callback,
    Cronet_ExecutorPtr executor) {
  CHECK(engine);
  CHECK(callback);
  CHECK(executor);
  Cronet_EngineImpl* engine_impl = static_cast<Cronet_EngineImpl*>(engine);
  GURL gurl(url);
  VLOG(1) << "New Cronet_UrlRequest: " << gurl.possibly_invalid_spec();
  request_ = new CronetURLRequest(
      engine_impl->cronet_url_request_context(),
      std::make_unique<Callback>(this, callback, executor), gurl,
      ConvertRequestPriority(params->priority), params->disableCache,
      true /* params->disableConnectionMigration */,
      false /* params->enableMetrics */);

  if (!params->httpMethod.empty())
    request_->SetHttpMethod(params->httpMethod);

  for (const auto& request_header : params->requestHeaders) {
    request_->AddRequestHeader(request_header->name, request_header->value);
  }
}

void Cronet_UrlRequestImpl::Start() {
  base::AutoLock lock(request_lock_);
  CHECK(request_);
  request_->Start();
}
void Cronet_UrlRequestImpl::FollowRedirect() {
  base::AutoLock lock(request_lock_);
  CHECK(request_);
  request_->FollowDeferredRedirect();
}

void Cronet_UrlRequestImpl::Read(Cronet_BufferPtr buffer) {
  base::AutoLock lock(request_lock_);
  read_buffer_ = buffer;
  net::IOBuffer* io_buffer = new net::WrappedIOBuffer(
      reinterpret_cast<char*>(Cronet_Buffer_GetData(buffer)));
  request_->ReadData(io_buffer, Cronet_Buffer_GetSize(buffer));
}

void Cronet_UrlRequestImpl::Cancel() {
  base::AutoLock lock(request_lock_);
  CHECK(request_);
  request_->Destroy(true);
  // Request can no longer be used.
  request_ = nullptr;
}

bool Cronet_UrlRequestImpl::IsDone() {
  return false;
}

void Cronet_UrlRequestImpl::GetStatus(
    Cronet_UrlRequestStatusListenerPtr listener) {}

Cronet_UrlRequestImpl::Callback::Callback(Cronet_UrlRequestImpl* url_request,
                                          Cronet_UrlRequestCallbackPtr callback,
                                          Cronet_ExecutorPtr executor)
    : url_request_(url_request), callback_(callback), executor_(executor) {
  DETACH_FROM_THREAD(network_thread_checker_);
}

Cronet_UrlRequestImpl::Callback::~Callback() {
  Cronet_Executor_Destroy(executor_);
  Cronet_UrlRequestCallback_Destroy(callback_);
}

// CronetURLRequest::Callback implementations:
void Cronet_UrlRequestImpl::Callback::OnReceivedRedirect(
    const std::string& new_location,
    int http_status_code,
    const std::string& http_status_text,
    const net::HttpResponseHeaders* headers,
    bool was_cached,
    const std::string& negotiated_protocol,
    const std::string& proxy_server,
    int64_t received_byte_count) {
  DCHECK_CALLED_ON_VALID_THREAD(network_thread_checker_);
}

void Cronet_UrlRequestImpl::Callback::OnResponseStarted(
    int http_status_code,
    const std::string& http_status_text,
    const net::HttpResponseHeaders* headers,
    bool was_cached,
    const std::string& negotiated_protocol,
    const std::string& proxy_server) {
  DCHECK_CALLED_ON_VALID_THREAD(network_thread_checker_);
  response_info_ = ConvertCronet_UrlResponseInfo(
      http_status_code, http_status_text, headers, was_cached,
      negotiated_protocol, proxy_server);
  // Invoke Cronet_UrlRequestCallback_OnResponseStarted using OnceClosure
  Cronet_RunnablePtr runnable = new cronet::OnceClosureRunnable(
      base::BindOnce(Cronet_UrlRequestCallback_OnResponseStarted, callback_,
                     url_request_, response_info_.get()));
  // |runnable| is passed to executor, which destroys it after execution.
  Cronet_Executor_Execute(executor_, runnable);
}

void Cronet_UrlRequestImpl::Callback::OnReadCompleted(
    scoped_refptr<net::IOBuffer> buffer,
    int bytes_read,
    int64_t received_byte_count) {
  DCHECK_CALLED_ON_VALID_THREAD(network_thread_checker_);
  base::AutoLock lock(url_request_->request_lock_);
  response_info_->receivedByteCount = received_byte_count;
  Cronet_Buffer_SetPosition(url_request_->read_buffer_, bytes_read);
  // Invoke Cronet_UrlRequestCallback_OnReadCompleted using OnceClosure
  Cronet_RunnablePtr runnable = new cronet::OnceClosureRunnable(base::BindOnce(
      Cronet_UrlRequestCallback_OnReadCompleted, callback_, url_request_,
      response_info_.get(), url_request_->read_buffer_));
  url_request_->read_buffer_ = nullptr;
  // |runnable| is passed to executor, which destroys it after execution.
  Cronet_Executor_Execute(executor_, runnable);
}

void Cronet_UrlRequestImpl::Callback::OnSucceeded(int64_t received_byte_count) {
  DCHECK_CALLED_ON_VALID_THREAD(network_thread_checker_);
  response_info_->receivedByteCount = received_byte_count;
  // Invoke Cronet_UrlRequestCallback_OnSucceeded using OnceClosure
  Cronet_RunnablePtr runnable = new cronet::OnceClosureRunnable(
      base::BindOnce(Cronet_UrlRequestCallback_OnSucceeded, callback_,
                     url_request_, response_info_.get()));
  if (url_request_->read_buffer_) {
    Cronet_Buffer_Destroy(url_request_->read_buffer_);
    url_request_->read_buffer_ = nullptr;
  }
  // |runnable| is passed to executor, which destroys it after execution.
  Cronet_Executor_Execute(executor_, runnable);
}

void Cronet_UrlRequestImpl::Callback::OnError(int net_error,
                                              int quic_error,
                                              const std::string& error_string,
                                              int64_t received_byte_count) {
  DCHECK_CALLED_ON_VALID_THREAD(network_thread_checker_);
  if (response_info_)
    response_info_->receivedByteCount = received_byte_count;
  error_ = ConvertCronet_Error(net_error, quic_error, error_string);
  // Invoke Cronet_UrlRequestCallback_OnFailed on client executor.
  Cronet_RunnablePtr runnable = new cronet::OnceClosureRunnable(
      base::BindOnce(Cronet_UrlRequestCallback_OnFailed, callback_,
                     url_request_, response_info_.get(), error_.get()));
  if (url_request_->read_buffer_) {
    Cronet_Buffer_Destroy(url_request_->read_buffer_);
    url_request_->read_buffer_ = nullptr;
  }
  // |runnable| is passed to executor, which destroys it after execution.
  Cronet_Executor_Execute(executor_, runnable);
}

void Cronet_UrlRequestImpl::Callback::OnCanceled() {
  DCHECK_CALLED_ON_VALID_THREAD(network_thread_checker_);
  // Invoke Cronet_UrlRequestCallback_OnCanceled on client executor.
  Cronet_RunnablePtr runnable = new cronet::OnceClosureRunnable(
      base::BindOnce(Cronet_UrlRequestCallback_OnCanceled, callback_,
                     url_request_, response_info_.get()));
  if (url_request_->read_buffer_) {
    Cronet_Buffer_Destroy(url_request_->read_buffer_);
    url_request_->read_buffer_ = nullptr;
  }
  // |runnable| is passed to executor, which destroys it after execution.
  Cronet_Executor_Execute(executor_, runnable);
}

void Cronet_UrlRequestImpl::Callback::OnDestroyed() {
  DCHECK_CALLED_ON_VALID_THREAD(network_thread_checker_);
  DCHECK(url_request_);
}

void Cronet_UrlRequestImpl::Callback::OnMetricsCollected(
    const base::Time& request_start_time,
    const base::TimeTicks& request_start,
    const base::TimeTicks& dns_start,
    const base::TimeTicks& dns_end,
    const base::TimeTicks& connect_start,
    const base::TimeTicks& connect_end,
    const base::TimeTicks& ssl_start,
    const base::TimeTicks& ssl_end,
    const base::TimeTicks& send_start,
    const base::TimeTicks& send_end,
    const base::TimeTicks& push_start,
    const base::TimeTicks& push_end,
    const base::TimeTicks& receive_headers_end,
    const base::TimeTicks& request_end,
    bool socket_reused,
    int64_t sent_bytes_count,
    int64_t received_bytes_count) {
  DCHECK_CALLED_ON_VALID_THREAD(network_thread_checker_);
}

};  // namespace cronet

CRONET_EXPORT Cronet_UrlRequestPtr Cronet_UrlRequest_Create() {
  return new cronet::Cronet_UrlRequestImpl();
}
