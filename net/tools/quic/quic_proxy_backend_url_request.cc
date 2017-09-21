// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/base/elements_upload_data_stream.h"
#include "net/base/net_errors.h"
#include "net/base/request_priority.h"
#include "net/base/upload_bytes_element_reader.h"
#include "net/cert/cert_status_flags.h"
#include "net/http/http_response_headers.h"
#include "net/http/http_status_code.h"
#include "net/http/http_util.h"
#include "net/quic/platform/api/quic_text_utils.h"
#include "net/ssl/ssl_info.h"
#include "net/url_request/redirect_info.h"
#include "net/url_request/url_request_context.h"

#include "net/tools/quic/quic_proxy_backend_url_request.h"

namespace net {

// This is the Size of the buffer that consumes the response from the Backend
// The response is consumed upto 64KB at a time to avoid a large response
// from hogging resources from smaller responses.
const int QuicProxyBackendUrlRequest::kBufferSize = 64000;
/*502 Bad Gateway
  The server was acting as a gateway or proxy and received an
  invalid response from the upstream server.*/
const int QuicProxyBackendUrlRequest::kProxyHttpBackendError = 502;
// Hop-by-hop headers (small-caps). These are removed when sent to the backend.
// http://www.w3.org/Protocols/rfc2616/rfc2616-sec13.html
// not Trailers per URL above;
// http://www.rfc-editor.org/errata_search.php?eid=4522
const std::set<std::string> QuicProxyBackendUrlRequest::kHopHeaders = {
    "connection",
    "proxy-connection",  // non-standard but still sent by libcurl and rejected
                         // by e.g. google
    "keep-alive", "proxy-authenticate", "proxy-authorization",
    "te",       // canonicalized version of "TE"
    "trailer",  // not Trailers per URL above;
                // http://www.rfc-editor.org/errata_search.php?eid=4522
    "transfer-encoding", "upgrade",
};
const std::string QuicProxyBackendUrlRequest::kDefaultQuicPeerIP = "Unknown";

QuicProxyBackendUrlRequest::QuicProxyBackendUrlRequest(
    QuicHttpResponseProxy* proxy_context)
    : proxy_context_(proxy_context),
      delegate_(nullptr),
      quic_peer_ip_(kDefaultQuicPeerIP),
      url_request_(nullptr),
      buf_(new IOBuffer(kBufferSize)),
      response_completed_(false),
      quic_response_(new QuicProxyBackendUrlRequest::Response()),
      weak_factory_(this) {}

QuicProxyBackendUrlRequest::~QuicProxyBackendUrlRequest() {}

void QuicProxyBackendUrlRequest::Initialize(
    net::QuicConnectionId quic_connection_id,
    net::QuicStreamId quic_stream_id,
    std::string quic_peer_ip) {
  quic_connection_id_ = quic_connection_id;
  quic_stream_id_ = quic_stream_id;
  quic_peer_ip_ = quic_peer_ip;
  if (!quic_proxy_task_runner_.get()) {
    quic_proxy_task_runner_ = proxy_context_->GetProxyTaskRunner();
  } else {
    DCHECK_EQ(quic_proxy_task_runner_, proxy_context_->GetProxyTaskRunner());
  }
}

void QuicProxyBackendUrlRequest::set_delegate(QuicProxyDelegate* delegate) {
  delegate_ = delegate;
  delegate_task_runner_ = base::SequencedTaskRunnerHandle::Get();
}

bool QuicProxyBackendUrlRequest::SendRequestToBackend(
    SpdyHeaderBlock* incoming_request_headers,
    const std::string& incoming_body) {
  DCHECK(proxy_context_->Initialized())
      << " The quic-backend-proxy-context should be initialized";
  std::string url = proxy_context_->backend_url();
  // Get Path From the Incoming Header Block
  SpdyHeaderBlock::const_iterator it = incoming_request_headers->find(":path");
  if (it != incoming_request_headers->end()) {
    url.append(it->second.as_string());
  }
  url_ = GURL(url.c_str());
  QUIC_DVLOG(1) << "QUIC Proxy Making a request to the Backed URL: " + url;

  // Set the Method From the Incoming Header Block
  std::string method = "";
  it = incoming_request_headers->find(":method");
  if (it != incoming_request_headers->end()) {
    method.append(it->second.as_string());
  }
  if (ValidateHttpMethod(method) != true) {
    QUIC_DVLOG(1) << "Unknown Request Type received from QUIC client "
                  << method;
    return false;
  }
  CopyHeaders(incoming_request_headers);
  if (method_type_ == "POST" || method_type_ == "PUT" ||
      method_type_ == "PATCH") {
    // Upload content must be set
    if (!incoming_body.empty()) {
      std::unique_ptr<UploadElementReader> reader(new UploadBytesElementReader(
          incoming_body.data(), incoming_body.size()));
      SetUpload(
          ElementsUploadDataStream::CreateWithReader(std::move(reader), 0));
    }
  }
  // Start the request on the backend thread
  bool posted = quic_proxy_task_runner_->PostTask(
      FROM_HERE,
      base::Bind(&QuicProxyBackendUrlRequest::SendRequestOnBackendThread,
                 weak_factory_.GetWeakPtr()));
  return posted;
}

void QuicProxyBackendUrlRequest::CopyHeaders(
    SpdyHeaderBlock* incoming_request_headers) {
  // Set all the request headers
  // Add or append the X-Forwarded-For Header and X-Real-IP
  for (SpdyHeaderBlock::const_iterator it = incoming_request_headers->begin();
       it != incoming_request_headers->end(); ++it) {
    std::string key = it->first.as_string();
    std::string value = it->second.as_string();
    // Ignore the spdy headers
    if (!key.empty() && key[0] != ':') {
      // Remove hop-by-hop headers
      if (QuicContainsKey(kHopHeaders, key)) {
        QUIC_DVLOG(1) << "QUIC Proxy Ignoring Hop-by-hop Request Header: "
                      << key << ":" << value;
      } else {
        QUIC_DVLOG(1) << "QUIC Proxy Copying to backend Request Header: " << key
                      << ":" << value;
        AddRequestHeader(key, value);
      }
    }
  }
  // ToDo append proxy ip when x_forwarded_for header already present
  AddRequestHeader("X-Forwarded-For", quic_peer_ip_);
}

bool QuicProxyBackendUrlRequest::ValidateHttpMethod(std::string method) {
  // Http method is a token, just as header name.
  if (!net::HttpUtil::IsValidHeaderName(method))
    return false;
  method_type_ = method;
  return true;
}

bool QuicProxyBackendUrlRequest::AddRequestHeader(std::string name,
                                                  std::string value) {
  if (!net::HttpUtil::IsValidHeaderName(name) ||
      !net::HttpUtil::IsValidHeaderValue(value)) {
    return false;
  }
  request_headers_.SetHeader(name, value);
  return true;
}

void QuicProxyBackendUrlRequest::SetUpload(
    std::unique_ptr<net::UploadDataStream> upload) {
  DCHECK(!upload_);
  upload_ = std::move(upload);
}

void QuicProxyBackendUrlRequest::SendRequestOnBackendThread() {
  DCHECK(quic_proxy_task_runner_->BelongsToCurrentThread());
  url_request_ = proxy_context_->GetURLRequestContext()->CreateRequest(
      url_, net::DEFAULT_PRIORITY, this);
  url_request_->set_method(method_type_);
  url_request_->SetExtraRequestHeaders(request_headers_);
  if (upload_) {
    url_request_->set_upload(std::move(upload_));
  }
  url_request_->Start();
  VLOG(1) << "Quic Proxy Sending Request to Backend for quic_conn_id: "
          << quic_connection_id_ << " quic_stream_id: " << quic_stream_id_
          << " backend_req_id: " << url_request_->identifier()
          << " url: " << url_;
}

void QuicProxyBackendUrlRequest::OnReceivedRedirect(
    net::URLRequest* request,
    const net::RedirectInfo& redirect_info,
    bool* defer_redirect) {
  DCHECK_EQ(request, url_request_.get());
  DCHECK(quic_proxy_task_runner_->BelongsToCurrentThread());
  // Do not defer redirect, retry again from the proxy with the new url
  *defer_redirect = false;
  QUIC_LOG(ERROR) << "Received Redirect from Backend "
                  << " BackendReqId: " << request->identifier()
                  << " redirectUrl: "
                  << redirect_info.new_url.possibly_invalid_spec().c_str()
                  << " RespCode " << request->GetResponseCode();
}

void QuicProxyBackendUrlRequest::OnCertificateRequested(
    net::URLRequest* request,
    net::SSLCertRequestInfo* cert_request_info) {
  DCHECK_EQ(request, url_request_.get());
  DCHECK(quic_proxy_task_runner_->BelongsToCurrentThread());
  // Continue the SSL handshake without a client certificate.
  request->ContinueWithCertificate(nullptr, nullptr);
}

void QuicProxyBackendUrlRequest::OnSSLCertificateError(
    net::URLRequest* request,
    const net::SSLInfo& ssl_info,
    bool fatal) {
  request->Cancel();
  OnResponseCompleted();
}

void QuicProxyBackendUrlRequest::OnResponseStarted(net::URLRequest* request,
                                                   int net_error) {
  DCHECK_EQ(request, url_request_.get());
  DCHECK(quic_proxy_task_runner_->BelongsToCurrentThread());
  // It doesn't make sense for the request to have IO pending at this point.
  DCHECK_NE(net::ERR_IO_PENDING, net_error);
  if (net_error != net::OK) {
    QUIC_LOG(ERROR) << "OnResponseStarted Error from Backend "
                    << url_request_->identifier() << " url: "
                    << url_request_->url().possibly_invalid_spec().c_str()
                    << " RespError " << net::ErrorToString(net_error);
    OnResponseCompleted();
    return;
  }
  // Initialite the first read
  ReadOnceTask();
}

void QuicProxyBackendUrlRequest::ReadOnceTask() {
  // Initiate a read for a max of kBufferSize
  // This avoids a request with a large response from starving
  // requests with smaller responses
  int bytes_read = url_request_->Read(buf_.get(), kBufferSize);
  OnReadCompleted(url_request_.get(), bytes_read);
}

// In the case of net::ERR_IO_PENDING,
// OnReadCompleted callback will be called by URLRequest
void QuicProxyBackendUrlRequest::OnReadCompleted(net::URLRequest* unused,
                                                 int bytes_read) {
  DCHECK_EQ(url_request_.get(), unused);
  QUIC_DVLOG(1) << "OnReadCompleted Backend with"
                << " ReqId: " << url_request_->identifier() << " RespCode "
                << url_request_->GetResponseCode() << " RcvdBytesCount "
                << bytes_read << " RcvdTotalBytes " << data_received_.size();

  if (bytes_read > 0) {
    data_received_.append(buf_->data(), bytes_read);
    // More data to be read, send a task to self
    quic_proxy_task_runner_->PostTask(
        FROM_HERE, base::Bind(&QuicProxyBackendUrlRequest::ReadOnceTask,
                              weak_factory_.GetWeakPtr()));
  } else if (bytes_read != net::ERR_IO_PENDING) {
    quic_response_->set_response_status(SUCCESSFUL_RESPONSE);
    OnResponseCompleted();
  }
}

/* Response from Backend complete, send the last chunk of data with fin=true to
 * the corresponding quic stream */
void QuicProxyBackendUrlRequest::OnResponseCompleted() {
  DCHECK(!response_completed_);
  VLOG(1) << "Quic Proxy Received Response from Backend for quic_conn_id: "
          << quic_connection_id_ << " quic_stream_id: " << quic_stream_id_
          << " backend_req_id: " << url_request_->identifier()
          << " url: " << url_;

  // ToDo Stream the response
  if (quic_response_->response_status() != BACKEND_ERR_RESPONSE) {
    quic_response_->set_response_code(url_request_->GetResponseCode());
    quic_response_->set_headers(url_request_->response_headers(),
                                data_received_.size());
    quic_response_->set_body(std::move(data_received_));
  } else {
    quic_response_->set_response_code(kProxyHttpBackendError);
  }
  response_completed_ = true;
  ReleaseRequest();

  // Send the response back to the quic client on the quic/main thread
  if (delegate_ != nullptr) {
    delegate_task_runner_->PostTask(
        FROM_HERE,
        base::Bind(&QuicProxyBackendUrlRequest::SendResponseOnDelegateThread,
                   base::Unretained(this)));
  }
}

void QuicProxyBackendUrlRequest::SendResponseOnDelegateThread() {
  DCHECK(delegate_ != nullptr);
  delegate_->OnResponseBackendComplete();
}

void QuicProxyBackendUrlRequest::CancelRequest() {
  DCHECK(quic_proxy_task_runner_->BelongsToCurrentThread());
  if (quic_proxy_task_runner_.get())
    DCHECK(quic_proxy_task_runner_->BelongsToCurrentThread());
  delegate_ = nullptr;
  if (url_request_.get()) {
    url_request_->CancelWithError(ERR_ABORTED);
    ReleaseRequest();
  }
}

void QuicProxyBackendUrlRequest::ReleaseRequest() {
  url_request_.reset();
  buf_ = nullptr;
}

QuicProxyBackendUrlRequest::Response* QuicProxyBackendUrlRequest::GetResponse()
    const {
  return quic_response_.get();
}

// Response back to quic client
QuicProxyBackendUrlRequest::Response::Response()
    : response_status_(BACKEND_ERR_RESPONSE),
      response_code_(kProxyHttpBackendError),
      headers_set_(false) {}

QuicProxyBackendUrlRequest::Response::~Response() {}

void QuicProxyBackendUrlRequest::Response::set_headers(
    net::HttpResponseHeaders* resp_headers,
    uint64_t response_decoded_body_size) {
  DCHECK(!headers_set_);
  bool response_body_encoded = false;
  // Add spdy headers: Status, version need : before the header
  headers_[":status"] = QuicTextUtils::Uint64ToString(response_code_);
  headers_set_ = true;
  // Returns an empty array if |headers| is nullptr.
  if (resp_headers != nullptr) {
    size_t iter = 0;
    std::string header_name;
    std::string header_value;
    while (resp_headers->EnumerateHeaderLines(&iter, &header_name,
                                              &header_value)) {
      header_name = QuicTextUtils::ToLower(header_name);
      // Do not copy status again since status needs a ":" before the header
      // name
      if (header_name.compare("status") != 0) {
        if (header_name.compare("content-encoding") != 0) {
          // Remove hop-by-hop headers
          if (QuicContainsKey(kHopHeaders, header_name)) {
            QUIC_DVLOG(1) << "Quic Proxy Ignoring Hop-by-hop Response Header: "
                          << header_name << ":" << header_value;
          } else {
            QUIC_DVLOG(1) << " Quic Proxy Copying Response Header: "
                          << header_name << ":" << header_value;
            headers_.AppendValueOrAddHeader(header_name, header_value);
          }
        } else {
          response_body_encoded = true;
        }
      }
    }  // while
    // Currently URLRequest class does not support ability to disable decoding,
    // response body (gzip, deflate etc. )
    // Instead of re-encoding the body, we send decode body to the quic client
    // and re-write the content length to the original body size
    if (response_body_encoded) {
      QUIC_DVLOG(1) << " Quic Proxy Rewriting the Content-Length Header since "
                       "the response was encoded : "
                    << response_decoded_body_size;
      headers_["content-length"] =
          QuicTextUtils::Uint64ToString(response_decoded_body_size);
    }
  }
}
}  // namespace net
