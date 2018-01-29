// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/loader/signed_exchange_prefetch_service_impl.h"

#include "base/feature_list.h"
#include "content/browser/loader/web_package_request_handler.h"
#include "content/public/browser/storage_partition.h"
#include "content/public/common/content_features.h"
#include "mojo/public/cpp/bindings/strong_binding.h"
#include "services/network/public/interfaces/network_service.mojom.h"

namespace content {

namespace {

class SignedExchangePrefetchLoaderImpl
    : public blink::mojom::SignedExchangePrefetchLoader,
      public network::mojom::URLLoaderClient {
 public:
  SignedExchangePrefetchLoaderImpl(
      network::mojom::URLLoaderFactoryPtr* network_loader_factory,
      blink::mojom::SignedExchangePrefetchLoaderClientPtr
          prefetch_loader_client,
      const GURL& url)
      : prefetch_loader_client_(std::move(prefetch_loader_client)),
        url_loader_client_binding_(this) {
    DCHECK(base::FeatureList::IsEnabled(features::kSignedHTTPExchange));
    network::ResourceRequest request;
    request.url = url;

    network::mojom::URLLoaderClientPtr url_loader_client_ptr_;
    url_loader_client_binding_.Bind(mojo::MakeRequest(&url_loader_client_ptr_));

    (*network_loader_factory)
        ->CreateLoaderAndStart(
            mojo::MakeRequest(&url_loader_), 0 /* routing_id */,
            0 /* request_id */, network::mojom::kURLLoadOptionNone, request,
            std::move(url_loader_client_ptr_),
            net::MutableNetworkTrafficAnnotationTag(
                net::DefineNetworkTrafficAnnotation(
                    "test",
                    "Traffic annotation for unit, browser and other tests")));
    LOG(ERROR)
        << "SignedExchangePrefetchLoaderImpl::SignedExchangePrefetchLoaderImpl "
        << this << " " << url;
  }
  ~SignedExchangePrefetchLoaderImpl() override {
    LOG(ERROR) << "SignedExchangePrefetchLoaderImpl::~"
                  "SignedExchangePrefetchLoaderImpl "
               << this;
  }

  // blink::mojom::SignedExchangePrefetchLoader overrides:
  void PrefetchSubresources(const std::vector<GURL>& urls) override {
    LOG(ERROR) << "SignedExchangePrefetchLoaderImpl::PrefetchSubresources "
               << this;
    for (auto& url : urls) {
      LOG(ERROR) << "  " << url;
    }
    // TODO: prefetch.
  }

  void Abort() override {
    LOG(ERROR) << "SignedExchangePrefetchLoaderImpl::Abort " << this;
  }

  // network::mojom::URLLoaderClient overrides:
  void OnReceiveResponse(
      const network::ResourceResponseHead& head,
      const base::Optional<net::SSLInfo>& ssl_info,
      network::mojom::DownloadedTempFilePtr downloaded_file) override {
    LOG(ERROR) << "SignedExchangePrefetchLoaderImpl::OnReceiveResponse";
    LOG(ERROR) << "  mime_type " << head.mime_type;
    std::string link_header_value;
    if (WebPackageRequestHandler::IsSupportedMimeType(head.mime_type)) {
      head.headers->GetNormalizedHeader("link", &link_header_value);
    }
    prefetch_loader_client_->OnReceiveResponse(link_header_value);
  }
  void OnReceiveRedirect(const net::RedirectInfo& redirect_info,
                         const network::ResourceResponseHead& head) override {
    LOG(ERROR) << "SignedExchangePrefetchLoaderImpl::OnReceiveRedirect";
  }
  void OnDataDownloaded(int64_t data_length, int64_t encoded_length) override {
    LOG(ERROR) << "SignedExchangePrefetchLoaderImpl::OnDataDownloaded";
  }
  void OnUploadProgress(int64_t current_position,
                        int64_t total_size,
                        base::OnceCallback<void()> callback) override {
    LOG(ERROR) << "SignedExchangePrefetchLoaderImpl::OnUploadProgress";
  }
  void OnReceiveCachedMetadata(const std::vector<uint8_t>& data) override {
    LOG(ERROR) << "SignedExchangePrefetchLoaderImpl::OnReceiveCachedMetadata";
  }
  void OnTransferSizeUpdated(int32_t transfer_size_diff) override {
    LOG(ERROR) << "SignedExchangePrefetchLoaderImpl::OnTransferSizeUpdated";
  }
  void OnStartLoadingResponseBody(
      mojo::ScopedDataPipeConsumerHandle body) override {
    // Need to consume the body???
    LOG(ERROR)
        << "SignedExchangePrefetchLoaderImpl::OnStartLoadingResponseBody";
  }
  void OnComplete(const network::URLLoaderCompletionStatus& status) override {
    LOG(ERROR) << "SignedExchangePrefetchLoaderImpl::OnComplete";
  }

 private:
  blink::mojom::SignedExchangePrefetchLoaderClientPtr prefetch_loader_client_;
  network::mojom::URLLoaderPtr url_loader_;
  mojo::Binding<network::mojom::URLLoaderClient> url_loader_client_binding_;
};

}  // namespace

SignedExchangePrefetchServiceImpl::SignedExchangePrefetchServiceImpl(
    network::mojom::NetworkContext* network_context,
    int process_id) {
  DCHECK(base::FeatureList::IsEnabled(features::kSignedHTTPExchange));
  network_context->CreateURLLoaderFactory(
      mojo::MakeRequest(&url_loader_factory_), process_id);
}

SignedExchangePrefetchServiceImpl::~SignedExchangePrefetchServiceImpl() =
    default;

// static
void SignedExchangePrefetchServiceImpl::CreateMojoService(
    StoragePartition* storage_partition,
    int process_id,
    blink::mojom::SignedExchangePrefetchServiceRequest request) {
  LOG(ERROR) << "SignedExchangePrefetchServiceImpl::CreateMojoService";
  mojo::MakeStrongBinding(
      std::make_unique<SignedExchangePrefetchServiceImpl>(
          storage_partition->GetNetworkContext(), process_id),
      std::move(request));
}

void SignedExchangePrefetchServiceImpl::StartPrefetch(
    blink::mojom::SignedExchangePrefetchLoaderRequest prefetch_loader_request,
    blink::mojom::SignedExchangePrefetchLoaderClientPtr prefetch_loader_client,
    const GURL& url) {
  LOG(ERROR) << "SignedExchangePrefetchServiceImpl::StartPrefetch " << url;

  std::unique_ptr<SignedExchangePrefetchLoaderImpl> loader =
      base::MakeUnique<SignedExchangePrefetchLoaderImpl>(
          &url_loader_factory_, std::move(prefetch_loader_client), url);

  mojo::MakeStrongBinding(std::move(loader),
                          std::move(prefetch_loader_request));
}

}  // namespace content
