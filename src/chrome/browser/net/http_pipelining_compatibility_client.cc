// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/net/http_pipelining_compatibility_client.h"

#include "base/metrics/histogram.h"
#include "base/stringprintf.h"
#include "net/base/load_flags.h"
#include "net/disk_cache/histogram_macros.h"
#include "net/http/http_response_headers.h"
#include "net/http/http_version.h"

namespace chrome_browser_net {

HttpPipeliningCompatibilityClient::HttpPipeliningCompatibilityClient()
    : num_finished_(0) {
}

HttpPipeliningCompatibilityClient::~HttpPipeliningCompatibilityClient() {
}

void HttpPipeliningCompatibilityClient::Start(
    const std::string& base_url,
    std::vector<RequestInfo>& requests,
    const net::CompletionCallback& callback,
    net::URLRequestContext* url_request_context) {
  finished_callback_ = callback;
  for (size_t i = 0; i < requests.size(); ++i) {
    requests_.push_back(new Request(i, base_url, requests[i], this,
                                    url_request_context));
  }
}

void HttpPipeliningCompatibilityClient::OnRequestFinished(int request_id,
                                                          Status status) {
  // The CACHE_HISTOGRAM_* macros are used, because they allow dynamic metric
  // names.
  CACHE_HISTOGRAM_ENUMERATION(GetMetricName(request_id, "Status"),
                              status, STATUS_MAX);
  ++num_finished_;
  if (num_finished_ == requests_.size()) {
    finished_callback_.Run(0);
  }
}

void HttpPipeliningCompatibilityClient::ReportNetworkError(int request_id,
                                                           int error_code) {
  CACHE_HISTOGRAM_ENUMERATION(GetMetricName(request_id, "NetworkError"),
                              -error_code, 900);
}

void HttpPipeliningCompatibilityClient::ReportResponseCode(int request_id,
                                                           int response_code) {
  CACHE_HISTOGRAM_ENUMERATION(GetMetricName(request_id, "ResponseCode"),
                              response_code, 600);
}

std::string HttpPipeliningCompatibilityClient::GetMetricName(
    int request_id, const char* description) {
  return base::StringPrintf("NetConnectivity.Pipeline.%d.%s",
                            request_id, description);
}

HttpPipeliningCompatibilityClient::Request::Request(
    int request_id,
    const std::string& base_url,
    const RequestInfo& info,
    HttpPipeliningCompatibilityClient* client,
    net::URLRequestContext* url_request_context)
    : request_id_(request_id),
      request_(GURL(base_url + info.filename), this),
      info_(info),
      client_(client),
      finished_(false) {
  request_.set_context(url_request_context);
  // TODO(simonjam): Force pipelining.
  request_.set_load_flags(net::LOAD_BYPASS_CACHE |
                          net::LOAD_DISABLE_CACHE |
                          net::LOAD_DO_NOT_SAVE_COOKIES |
                          net::LOAD_DO_NOT_SEND_COOKIES |
                          net::LOAD_DO_NOT_PROMPT_FOR_LOGIN |
                          net::LOAD_DO_NOT_SEND_AUTH_DATA);
  request_.Start();
}

HttpPipeliningCompatibilityClient::Request::~Request() {
}

void HttpPipeliningCompatibilityClient::Request::OnReceivedRedirect(
    net::URLRequest* request,
    const GURL& new_url,
    bool* defer_redirect) {
  *defer_redirect = true;
  request->Cancel();
  Finished(REDIRECTED);
}

void HttpPipeliningCompatibilityClient::Request::OnSSLCertificateError(
    net::URLRequest* request,
    const net::SSLInfo& ssl_info,
    bool fatal) {
  Finished(CERT_ERROR);
}

void HttpPipeliningCompatibilityClient::Request::OnResponseStarted(
    net::URLRequest* request) {
  if (finished_) {
    return;
  }
  int response_code = request->GetResponseCode();
  if (response_code > 0) {
    client_->ReportResponseCode(request_id_, response_code);
  }
  if (response_code == 200) {
    const net::HttpVersion required_version(1, 1);
    if (request->response_info().headers->GetParsedHttpVersion() <
        required_version) {
      Finished(BAD_HTTP_VERSION);
    } else {
      read_buffer_ = new net::IOBuffer(info_.expected_response.length());
      DoRead();
    }
  } else {
    Finished(BAD_RESPONSE_CODE);
  }
}

void HttpPipeliningCompatibilityClient::Request::OnReadCompleted(
    net::URLRequest* request,
    int bytes_read) {
  if (bytes_read == 0) {
    DoReadFinished();
  } else if (bytes_read < 0) {
    Finished(NETWORK_ERROR);
  } else {
    response_.append(read_buffer_->data(), bytes_read);
    if (response_.length() <= info_.expected_response.length()) {
      DoRead();
    } else if (response_.find(info_.expected_response) == 0) {
      Finished(TOO_LARGE);
    } else {
      Finished(CONTENT_MISMATCH);
    }
  }
}

void HttpPipeliningCompatibilityClient::Request::DoRead() {
  int bytes_read = 0;
  if (request_.Read(read_buffer_.get(), info_.expected_response.length(),
                    &bytes_read)) {
    OnReadCompleted(&request_, bytes_read);
  }
}

void HttpPipeliningCompatibilityClient::Request::DoReadFinished() {
  if (response_.length() != info_.expected_response.length()) {
    if (info_.expected_response.find(response_) == 0) {
      Finished(TOO_SMALL);
    } else {
      Finished(CONTENT_MISMATCH);
    }
  } else if (response_ == info_.expected_response) {
    Finished(SUCCESS);
  } else {
    Finished(CONTENT_MISMATCH);
  }
}

void HttpPipeliningCompatibilityClient::Request::Finished(Status result) {
  if (finished_) {
    return;
  }
  finished_ = true;
  const net::URLRequestStatus& status = request_.status();
  if (status.status() == net::URLRequestStatus::FAILED) {
    // Network errors trump all other status codes, because network errors can
    // be detected by the network stack even with real content. If we determine
    // that all pipelining errors can be detected by the network stack, then we
    // don't need to worry about broken proxies.
    client_->ReportNetworkError(request_id_, status.error());
    client_->OnRequestFinished(request_id_, NETWORK_ERROR);
    return;
  }
  client_->OnRequestFinished(request_id_, result);
}

}  // namespace chrome_browser_net
