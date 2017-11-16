// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/download/resource_downloader.h"

#include "content/browser/blob_storage/blob_url_loader_factory.h"
#include "content/browser/download/download_utils.h"
#include "content/common/throttling_url_loader.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"
#include "storage/browser/fileapi/file_system_context.h"

namespace content {

// This object monitors the URLLoaderStatus change when ResourceDownloader is
// asking |delegate_| whether download can proceed.
class URLLoaderStatusMonitor : public mojom::URLLoaderClient {
 public:
  using URLLoaderStatusChangeCallback =
      base::OnceCallback<void(const network::URLLoaderStatus&)>;
  explicit URLLoaderStatusMonitor(URLLoaderStatusChangeCallback callback);
  ~URLLoaderStatusMonitor() override = default;

  // mojom::URLLoaderClient
  void OnReceiveResponse(
      const ResourceResponseHead& head,
      const base::Optional<net::SSLInfo>& ssl_info,
      mojom::DownloadedTempFilePtr downloaded_file) override {}
  void OnReceiveRedirect(const net::RedirectInfo& redirect_info,
                         const ResourceResponseHead& head) override {}
  void OnDataDownloaded(int64_t data_length, int64_t encoded_length) override {}
  void OnUploadProgress(int64_t current_position,
                        int64_t total_size,
                        OnUploadProgressCallback callback) override {}
  void OnReceiveCachedMetadata(const std::vector<uint8_t>& data) override {}
  void OnTransferSizeUpdated(int32_t transfer_size_diff) override {}
  void OnStartLoadingResponseBody(
      mojo::ScopedDataPipeConsumerHandle body) override {}
  void OnComplete(const network::URLLoaderStatus& status) override;

 private:
  URLLoaderStatusChangeCallback callback_;
  DISALLOW_COPY_AND_ASSIGN(URLLoaderStatusMonitor);
};

URLLoaderStatusMonitor::URLLoaderStatusMonitor(
    URLLoaderStatusChangeCallback callback)
    : callback_(std::move(callback)) {}

void URLLoaderStatusMonitor::OnComplete(
    const network::URLLoaderStatus& status) {
  std::move(callback_).Run(status);
}

// This class is only used for providing the WebContents to DownloadItemImpl.
class RequestHandle : public DownloadRequestHandleInterface {
 public:
  explicit RequestHandle(const ResourceRequestInfo::WebContentsGetter& getter)
      : web_contents_getter_(getter) {}
  RequestHandle(RequestHandle&& other)
      : web_contents_getter_(other.web_contents_getter_) {}

  // DownloadRequestHandleInterface
  WebContents* GetWebContents() const override {
    return web_contents_getter_.Run();
  }

  DownloadManager* GetDownloadManager() const override { return nullptr; }
  void PauseRequest() const override {}
  void ResumeRequest() const override {}
  void CancelRequest(bool user_cancel) const override {}

 private:
  ResourceRequestInfo::WebContentsGetter web_contents_getter_;

  DISALLOW_COPY_AND_ASSIGN(RequestHandle);
};

// static
std::unique_ptr<ResourceDownloader> ResourceDownloader::BeginDownload(
    base::WeakPtr<UrlDownloadHandler::Delegate> delegate,
    std::unique_ptr<DownloadUrlParameters> params,
    std::unique_ptr<ResourceRequest> request,
    scoped_refptr<URLLoaderFactoryGetter> url_loader_factory_getter,
    scoped_refptr<storage::FileSystemContext> file_system_context,
    const ResourceRequestInfo::WebContentsGetter& web_contents_getter,
    uint32_t download_id,
    bool is_parallel_request) {
  mojom::URLLoaderFactoryPtr* factory =
      params->url().SchemeIs(url::kBlobScheme)
          ? url_loader_factory_getter->GetBlobFactory()
          : url_loader_factory_getter->GetNetworkFactory();
  auto downloader = std::make_unique<ResourceDownloader>(
      delegate, std::move(request), web_contents_getter, download_id,
      params->guid());

  downloader->Start(factory, file_system_context, std::move(params),
                    is_parallel_request);
  return downloader;
}

// static
std::unique_ptr<ResourceDownloader> ResourceDownloader::CreateWithURLLoader(
    base::WeakPtr<UrlDownloadHandler::Delegate> delegate,
    std::unique_ptr<ResourceRequest> resource_request,
    const ResourceRequestInfo::WebContentsGetter& web_contents_getter,
    std::unique_ptr<ThrottlingURLLoader> url_loader,
    base::Optional<network::URLLoaderStatus> status) {
  auto downloader = std::make_unique<ResourceDownloader>(
      delegate, std::move(resource_request), web_contents_getter,
      DownloadItem::kInvalidId, std::string());
  downloader->InitializeURLLoader(std::move(url_loader), std::move(status));
  return downloader;
}

ResourceDownloader::ResourceDownloader(
    base::WeakPtr<UrlDownloadHandler::Delegate> delegate,
    std::unique_ptr<ResourceRequest> resource_request,
    const ResourceRequestInfo::WebContentsGetter& web_contents_getter,
    uint32_t download_id,
    std::string guid)
    : delegate_(delegate),
      resource_request_(std::move(resource_request)),
      download_id_(download_id),
      guid_(guid),
      web_contents_getter_(web_contents_getter),
      weak_ptr_factory_(this) {}

ResourceDownloader::~ResourceDownloader() = default;

void ResourceDownloader::Start(
    mojom::URLLoaderFactoryPtr* factory,
    scoped_refptr<storage::FileSystemContext> file_system_context,
    std::unique_ptr<DownloadUrlParameters> download_url_parameters,
    bool is_parallel_request) {
  callback_ = download_url_parameters->callback();
  url_loader_client_ = base::MakeUnique<DownloadResponseHandler>(
      resource_request_.get(), this,
      std::make_unique<DownloadSaveInfo>(
          download_url_parameters->GetSaveInfo()),
      is_parallel_request, download_url_parameters->is_transient(),
      download_url_parameters->fetch_error_body(),
      std::vector<GURL>(1, resource_request_->url));

  if (download_url_parameters->url().SchemeIs(url::kBlobScheme)) {
    mojom::URLLoaderRequest url_loader_request;
    mojom::URLLoaderClientPtr client;
    blob_client_binding_ =
        base::MakeUnique<mojo::Binding<mojom::URLLoaderClient>>(
            url_loader_client_.get());
    blob_client_binding_->Bind(mojo::MakeRequest(&client));
    BlobURLLoaderFactory::CreateLoaderAndStart(
        std::move(url_loader_request), *(resource_request_.get()),
        std::move(client), download_url_parameters->GetBlobDataHandle(),
        file_system_context.get());
  } else {
    url_loader_ = ThrottlingURLLoader::CreateLoaderAndStart(
        factory->get(), std::vector<std::unique_ptr<URLLoaderThrottle>>(),
        0,  // routing_id
        0,  // request_id
        mojom::kURLLoadOptionSendSSLInfo | mojom::kURLLoadOptionSniffMimeType,
        *(resource_request_.get()), url_loader_client_.get(),
        download_url_parameters->GetNetworkTrafficAnnotation());
    url_loader_->SetPriority(net::RequestPriority::IDLE,
                             0 /* intra_priority_value */);
  }
}

void ResourceDownloader::InitializeURLLoader(
    std::unique_ptr<ThrottlingURLLoader> url_loader,
    base::Optional<network::URLLoaderStatus> status) {
  url_loader_status_ = std::move(status);
  url_loader_ = std::move(url_loader);
  url_loader_client_ = base::MakeUnique<URLLoaderStatusMonitor>(
      base::Bind(&ResourceDownloader::OnURLLoaderStatusChanged,
                 weak_ptr_factory_.GetWeakPtr()));
  url_loader_->set_forwarding_client(url_loader_client_.get());
}

void ResourceDownloader::OnURLLoaderStatusChanged(
    const network::URLLoaderStatus& status) {
  DCHECK(!url_loader_status_);
  url_loader_status_ = status;
}

void ResourceDownloader::StartNavigationInterception(
    const scoped_refptr<ResourceResponse>& response,
    mojo::ScopedDataPipeConsumerHandle consumer_handle,
    const SSLStatus& ssl_status,
    std::vector<GURL> url_chain) {
  url_loader_client_ = base::MakeUnique<DownloadResponseHandler>(
      resource_request_.get(), this, std::make_unique<DownloadSaveInfo>(),
      false, false, false, url_chain);
  url_loader_->set_forwarding_client(url_loader_client_.get());
  net::SSLInfo info;
  info.cert_status = ssl_status.cert_status;
  url_loader_client_->OnReceiveResponse(response->head,
                                        base::Optional<net::SSLInfo>(info),
                                        mojom::DownloadedTempFilePtr());
  url_loader_client_->OnStartLoadingResponseBody(std::move(consumer_handle));
  if (url_loader_status_)
    url_loader_client_->OnComplete(url_loader_status_.value());
}

void ResourceDownloader::OnResponseStarted(
    std::unique_ptr<DownloadCreateInfo> download_create_info,
    mojom::DownloadStreamHandlePtr stream_handle) {
  download_create_info->download_id = download_id_;
  download_create_info->guid = guid_;
  download_create_info->request_handle.reset(
      new RequestHandle(web_contents_getter_));

  BrowserThread::PostTask(
      BrowserThread::UI, FROM_HERE,
      base::BindOnce(&UrlDownloadHandler::Delegate::OnUrlDownloadStarted,
                     delegate_, std::move(download_create_info),
                     std::make_unique<DownloadManager::InputStream>(
                         std::move(stream_handle)),
                     callback_));
}

void ResourceDownloader::OnReceiveRedirect() {
  url_loader_->FollowRedirect();
}

}  // namespace content
