// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/loader/web_package_loader.h"

#include "base/feature_list.h"
#include "base/strings/stringprintf.h"
#include "content/browser/loader/signed_exchange_handler.h"
#include "content/public/common/content_features.h"
#include "net/http/http_util.h"

namespace content {

class WebPackageLoader::ResponseTimingInfo {
 public:
  explicit ResponseTimingInfo(const network::ResourceResponseHead& response)
      : request_start_(response.request_start),
        response_start_(response.response_start),
        request_time_(response.request_time),
        response_time_(response.response_time),
        load_timing_(response.load_timing) {}

  void CopyTo(network::ResourceResponseHead* response) const {
    response->request_start = request_start_;
    response->response_start = response_start_;
    response->request_time = request_time_;
    response->response_time = response_time_;
    response->load_timing = load_timing_;
  }

 private:
  const base::TimeTicks request_start_;
  const base::TimeTicks response_start_;
  const base::Time request_time_;
  const base::Time response_time_;
  const net::LoadTimingInfo load_timing_;

  DISALLOW_COPY_AND_ASSIGN(ResponseTimingInfo);
};

WebPackageLoader::WebPackageLoader(
    const network::ResourceResponseHead& original_response,
    network::mojom::URLLoaderClientPtr forwarding_client,
    network::mojom::URLLoaderClientEndpointsPtr endpoints)
    : original_response_timing_info_(
          base::MakeUnique<ResponseTimingInfo>(original_response)),
      forwarding_client_(std::move(forwarding_client)),
      url_loader_client_binding_(this),
      weak_factory_(this) {
  url_loader_.Bind(std::move(endpoints->url_loader));
  if (!base::FeatureList::IsEnabled(features::kNetworkService))
    url_loader_->ProceedWithResponse();
  url_loader_client_binding_.Bind(std::move(endpoints->url_loader_client));

  pending_client_request_ = mojo::MakeRequest(&client_);
}

WebPackageLoader::~WebPackageLoader() {}

void WebPackageLoader::OnReceiveResponse(
    const network::ResourceResponseHead& response_head,
    const base::Optional<net::SSLInfo>& ssl_info,
    network::mojom::DownloadedTempFilePtr downloaded_file) {
  // Must not be caused because this WebPackageLoader and the client endpoints
  // were bound after OnReceiveResponse() had been called.
  NOTREACHED();
}

void WebPackageLoader::OnReceiveRedirect(
    const net::RedirectInfo& redirect_info,
    const network::ResourceResponseHead& response_head) {
  // Must not be caused because this WebPackageLoader and the client endpoints
  // were bound after OnReceiveResponse() had been called.
  NOTREACHED();
}

void WebPackageLoader::OnDataDownloaded(int64_t data_len,
                                        int64_t encoded_data_len) {
  // Must not be caused because this WebPackageLoader and the client endpoints
  // were bound after OnReceiveResponse() had been called.
  NOTREACHED();
}

void WebPackageLoader::OnUploadProgress(int64_t current_position,
                                        int64_t total_size,
                                        OnUploadProgressCallback ack_callback) {
  // Must not be caused because this WebPackageLoader and the client endpoints
  // were bound after OnReceiveResponse() had been called.
  NOTREACHED();
}

void WebPackageLoader::OnReceiveCachedMetadata(
    const std::vector<uint8_t>& data) {
  // Curerntly CachedMetadata for WebPackage is not supported.
  NOTREACHED();
}

void WebPackageLoader::OnTransferSizeUpdated(int32_t transfer_size_diff) {
  // Must not be caused because this WebPackageLoader and the client endpoints
  // were bound after OnReceiveResponse() had been called.
  NOTREACHED();
}

void WebPackageLoader::OnStartLoadingResponseBody(
    mojo::ScopedDataPipeConsumerHandle body) {
  signed_exchange_handler_ =
      base::MakeUnique<SignedExchangeHandler>(std::move(body));
  signed_exchange_handler_->GetHTTPExchange(
      base::BindOnce(&WebPackageLoader::OnHTTPExchangeFound,
                     weak_factory_.GetWeakPtr()),
      base::BindOnce(&WebPackageLoader::OnHTTPExchangeFinished,
                     weak_factory_.GetWeakPtr()));
}

void WebPackageLoader::OnComplete(
    const network::URLLoaderCompletionStatus& status) {}

void WebPackageLoader::ConnectToClient(
    network::mojom::URLLoaderClientPtr client) {
  mojo::FuseInterface(std::move(pending_client_request_),
                      client.PassInterface());
}

// static
std::tuple<net::RedirectInfo, network::ResourceResponseHead>
WebPackageLoader::CreateRedirectResponse(
    const GURL& new_url,
    std::unique_ptr<const WebPackageLoader::ResponseTimingInfo> timing_info) {
  net::RedirectInfo redirect_info;
  redirect_info.new_url = new_url;
  redirect_info.new_method = "GET";
  redirect_info.status_code = 302;
  redirect_info.new_site_for_cookies = redirect_info.new_url;
  network::ResourceResponseHead response_head;
  response_head.encoded_data_length = 0;
  std::string buf(base::StringPrintf("HTTP/1.1 %d %s\r\n", 302, "Found"));
  response_head.headers = new net::HttpResponseHeaders(
      net::HttpUtil::AssembleRawHeaders(buf.c_str(), buf.size()));
  response_head.encoded_data_length = 0;
  DCHECK(timing_info);
  std::move(timing_info)->CopyTo(&response_head);
  return std::tie(redirect_info, response_head);
}

void WebPackageLoader::OnHTTPExchangeFound(
    const GURL& request_url,
    const network::ResourceResponseHead& resource_response,
    mojo::ScopedDataPipeConsumerHandle body) {
  DCHECK(original_response_timing_info_);
  net::RedirectInfo redirect_info;
  network::ResourceResponseHead response_head;
  std::tie(redirect_info, response_head) = CreateRedirectResponse(
      request_url, std::move(original_response_timing_info_));
  forwarding_client_->OnReceiveRedirect(redirect_info, response_head);
  forwarding_client_->OnComplete(network::URLLoaderCompletionStatus(net::OK));
  forwarding_client_.reset();

  client_->OnReceiveResponse(resource_response,
                             base::Optional<net::SSLInfo>() /* TODO */,
                             nullptr /* downloaded_file */);
  client_->OnStartLoadingResponseBody(std::move(body));
}

void WebPackageLoader::OnHTTPExchangeFinished(
    const network::URLLoaderCompletionStatus& status) {
  client_->OnComplete(status);
}

}  // namespace content
