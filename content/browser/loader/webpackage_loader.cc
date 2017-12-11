// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/loader/webpackage_loader.h"

#include "base/command_line.h"
#include "base/strings/stringprintf.h"
#include "content/browser/loader/webpackage_subresource_manager_host.h"
#include "content/browser/url_loader_factory_getter.h"
#include "content/common/navigation_subresource_loader_params.h"
#include "content/common/webpackage_subresource_loader.h"
#include "content/public/common/resource_request.h"
#include "content/public/common/resource_response.h"
#include "content/public/common/url_loader_factory.mojom.h"
#include "mojo/public/cpp/bindings/binding_set.h"
#include "mojo/public/cpp/bindings/strong_binding.h"
#include "net/http/http_util.h"
#include "net/url_request/redirect_info.h"
#include "net/url_request/url_request_job.h"
#include "services/network/public/cpp/url_loader_completion_status.h"
#include "url/gurl.h"

namespace content {

bool WebPackageStreamResourcesEnabled() {
  static bool enable_streaming =
      base::CommandLine::ForCurrentProcess()->HasSwitch(
          "enable-webpackage-streaming");
  if (enable_streaming)
    LOG(ERROR) << "** WebPackageStreaming enabled";
  return enable_streaming;
}

namespace {

const net::NetworkTrafficAnnotationTag kWebPackageNavigationTrafficAnnotation =
    net::DefineNetworkTrafficAnnotation("webpackage_loader", R"(
      semantics {
        sender: "WebPackage Loader"
        description:
          "This request is issued by a main frame navigation to fetch the "
          "webpackage content that includes the page data that is being "
          "navigated to."
        trigger:
          "Navigating Chrome (by clicking on a link, bookmark, history item, "
          "using session restore, etc)."
        data:
          "Arbitrary site-controlled data can be included in the URL, HTTP "
          "headers, and request body. Requests may include cookies and "
          "site-specific credentials."
        destination: WEBSITE
      }
      policy {
        cookies_allowed: YES
        cookies_store: "user"
        setting: "This feature cannot be disabled."
        chrome_policy {
          URLBlacklist {
            URLBlacklist: { entries: '*' }
          }
        }
        chrome_policy {
          URLWhitelist {
            URLWhitelist { }
          }
        }
      }
      comments:
        "Chrome would be unable to navigate to websites without this type of "
        "request. Using either URLBlacklist or URLWhitelist policies (or a "
        "combination of both) limits the scope of these requests."
      )");


void ForwardRedirectResponse(
    const net::RedirectInfo& redirect_info,
    const ResourceResponseHead& response_head,
    mojom::URLLoaderRequest request,
    mojom::URLLoaderClientPtr client) {
  client->OnReceiveRedirect(redirect_info, response_head);
  client->OnComplete(network::URLLoaderCompletionStatus(net::OK));
}

}  // namespace

//----------------------------------------------------------------------------

// This class is destructed when the Mojo pipe bound to this object is
// dropped.
class MainResourceLoader : public mojom::URLLoader {
 public:
  MainResourceLoader(
      const GURL& url,
      scoped_refptr<WebPackageReaderAdapter> reader,
      mojom::URLLoaderClientPtr client,
      base::WeakPtr<WebPackageSubresourceManagerHost> subresource_manager_host)
      : loader_client_(std::move(client)),
        reader_(std::move(reader)),
        subresource_manager_host_(std::move(subresource_manager_host)),
        weak_factory_(this) {
    // LOG(ERROR) << "MainResourceLoader " << this << " " << url;
    if (url.path() == "/") {
      // No particular path is specified; try to render the first resource.
      reader_->GetFirstResource(
          base::BindOnce(&MainResourceLoader::OnFoundResource,
                         weak_factory_.GetWeakPtr()),
          base::BindOnce(&MainResourceLoader::OnComplete,
                         weak_factory_.GetWeakPtr()));
      return;
    }
    ResourceRequest resource_request;
    resource_request.url = url;
    resource_request.method = "GET";
    reader_->GetResource(resource_request,
                         base::BindOnce(&MainResourceLoader::OnFoundResource,
                                        weak_factory_.GetWeakPtr()),
                         base::BindOnce(&MainResourceLoader::OnComplete,
                                        weak_factory_.GetWeakPtr()));
  }

 private:
  void OnFoundResource(int error_code,
                       const ResourceResponseHead& response_head,
                       mojo::ScopedDataPipeConsumerHandle body) {
    // LOG(ERROR) << "MainResourceLoader::OnFoundResource " << this
    //            << " error_code: " << error_code;
    if (error_code != net::OK) {
      loader_client_->OnComplete(
          network::URLLoaderCompletionStatus(error_code));
      return;
    }
    if (WebPackageStreamResourcesEnabled()) {
      // This still should be around.
      DCHECK(subresource_manager_host_);
      // After this point it's fine to switch to the push mode.
      reader_->StartPushResources(std::move(subresource_manager_host_));
    }

    loader_client_->OnReceiveResponse(response_head, base::nullopt,
                                      nullptr /* download_file */);
    loader_client_->OnStartLoadingResponseBody(std::move(body));
  }
  void OnComplete(const network::URLLoaderCompletionStatus& status) {
    // LOG(ERROR) << "MainResourceLoader::OnComplete " << this
    //           << " error_code: " << status.error_code;
    loader_client_->OnComplete(status);
  }

  void FollowRedirect() override {}
  void SetPriority(net::RequestPriority priority,
                   int32_t intra_priority_value) override {}
  void PauseReadingBodyFromNet() override {}
  void ResumeReadingBodyFromNet() override {}

  mojom::URLLoaderClientPtr loader_client_;
  scoped_refptr<WebPackageReaderAdapter> reader_;
  base::WeakPtr<WebPackageSubresourceManagerHost> subresource_manager_host_;

  base::WeakPtrFactory<MainResourceLoader> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(MainResourceLoader);
};

//----------------------------------------------------------------------------

WebPackageRequestHandler::WebPackageRequestHandler(
    const ResourceRequest& resource_request,
    scoped_refptr<URLLoaderFactoryGetter> loader_factory_getter)
    : resource_request_(resource_request),
      loader_factory_getter_(std::move(loader_factory_getter)),
      weak_factory_(this) {}

WebPackageRequestHandler::~WebPackageRequestHandler() = default;

void WebPackageRequestHandler::MaybeCreateLoader(
    const ResourceRequest& resource_request,
    ResourceContext* resource_context,
    LoaderCallback callback) {
  if (webpackage_loader_ &&
      webpackage_start_url_ == resource_request.url) {
    // Second load. This must be for the package's start_url.
    DCHECK_EQ("GET", resource_request.method);
    DCHECK(webpackage_reader_);

    base::WeakPtr<WebPackageSubresourceManagerHost> subresource_manager_host;

    if (WebPackageStreamResourcesEnabled()) {
      // Create the subresource manager host before we run the callback,
      // This is synchronously taken by MaybeCreateSubresourceLoaderParams
      // during the callback.
      //
      // WebPackageSubresourceManagerHost's lifetime is tied to that of
      // |subresource_manager_request_|.
      mojom::WebPackageSubresourceManagerPtr subresource_manager_ptr;
      subresource_manager_request_ =
          mojo::MakeRequest(&subresource_manager_ptr);
      auto* manager_host = new WebPackageSubresourceManagerHost(
          webpackage_reader_, std::move(subresource_manager_ptr));
      subresource_manager_host = manager_host->GetWeakPtr();
    } else {
      // The factory destroys itself when |factory_ptr| is dropped.
      auto* factory = new WebPackageSubresourceLoaderFactory(
          base::Bind(&WebPackageReaderAdapter::GetResource, webpackage_reader_),
          base::Bind(&URLLoaderFactoryGetter::GetNetworkFactory,
                     loader_factory_getter_));
      factory->Clone(mojo::MakeRequest(&subresource_loader_factory_));
    }

    // Run the callback to asynchronously create the main resource loader.
    std::move(callback).Run(base::BindOnce(
        &WebPackageRequestHandler::CreateMainResourceLoader,
        weak_factory_.GetWeakPtr(), std::move(subresource_manager_host)));

    // After this point |webpackage_loader_| will destruct itself when
    // loading the package data is finished.
    webpackage_loader_.release()->DetachFromRequestHandler();
    return;
  }
  // XXX quick hack.
  base::FilePath::StringType extension =
      base::FilePath(resource_request.url.PathForRequest()).Extension();
  if (extension != FILE_PATH_LITERAL(".wpk")) {
    LOG(ERROR) << "--- Not WPK " << resource_request.url;
    // Shouldn't be a web package, let it go.
    std::move(callback).Run(StartLoaderCallback());
    return;
  }

  // It's possible that we come here multiple times due to redirects
  // before we start to receive the actual package data.
  if (webpackage_loader_)
    webpackage_loader_.reset();

  DCHECK(loader_factory_getter_);
  LOG(ERROR) << "** Creating WebPackgaeLoader";

  webpackage_loader_ =
      loader_factory_getter_->GetWebPackageLoaderManager()->TakeMatchingLoader(
          resource_request);
  if (!webpackage_loader_) {
    webpackage_loader_ = std::make_unique<WebPackageLoader>(
        resource_request, loader_factory_getter_.get());
  }
  webpackage_loader_->AttachToRequestHandler(this, std::move(callback));
}

base::Optional<SubresourceLoaderParams>
WebPackageRequestHandler::MaybeCreateSubresourceLoaderParams() {
  SubresourceLoaderParams params;
  if (!subresource_manager_request_.is_pending() &&
      !subresource_loader_factory_) {
    return base::nullopt;
  }

  params.webpackage_subresource_info = mojom::WebPackageSubresourceInfo::New();
  webpackage_reader_->PopulateRequests(
      params.webpackage_subresource_info.get());

  if (WebPackageStreamResourcesEnabled()) {
    params.webpackage_subresource_info->manager_request =
        std::move(subresource_manager_request_);
    LOG(INFO) << "** Returning subresource manager";
  } else {
    params.loader_factory_info = subresource_loader_factory_.PassInterface();
    LOG(INFO) << "** Returning subresource loader factory";
  }
  return params;
}

bool WebPackageRequestHandler::MaybeCreateLoaderForResponse(
    const ResourceResponseHead& response,
    mojom::URLLoaderPtr* loader,
    mojom::URLLoaderClientRequest* client_request) {
  // For now we don't use this code.
  return false;
}

void WebPackageRequestHandler::OnRedirectedToMainResource(
    const GURL& webpackage_start_url,
    scoped_refptr<WebPackageReaderAdapter> reader) {
  webpackage_start_url_ = webpackage_start_url;
  webpackage_reader_ = std::move(reader);
}

void WebPackageRequestHandler::CreateMainResourceLoader(
    base::WeakPtr<WebPackageSubresourceManagerHost> subresource_manager_host,
    mojom::URLLoaderRequest loader_request,
    mojom::URLLoaderClientPtr loader_client) {
  DCHECK(webpackage_reader_);
  LOG(INFO) << "** Creating WebPackgaeLoader's MainResourceLoader";
  url::Replacements<char> remove_query;
  remove_query.ClearQuery();
  mojo::MakeStrongBinding(
      std::make_unique<MainResourceLoader>(
          webpackage_start_url_.ReplaceComponents(remove_query),
          webpackage_reader_, std::move(loader_client),
          std::move(subresource_manager_host)),
      std::move(loader_request));
}

//----------------------------------------------------------------------------

WebPackageLoader::WebPackageLoader(
    const ResourceRequest& resource_request,
    URLLoaderFactoryGetter* loader_factory_getter)
    : webpackage_request_(resource_request),
      network_client_binding_(this),
      weak_factory_(this) {
  // Start the network loading.
  mojom::URLLoaderClientPtr loader_client;
  network_client_binding_.Bind(mojo::MakeRequest(&loader_client));
  loader_factory_getter->GetNetworkFactory()->CreateLoaderAndStart(
      mojo::MakeRequest(&network_loader_), 0 /* routing_id */,
      0 /* request_id */, mojom::kURLLoadOptionNone /* no sniffing for now */,
      resource_request, std::move(loader_client),
      net::MutableNetworkTrafficAnnotationTag(
          kWebPackageNavigationTrafficAnnotation));
}

WebPackageLoader::~WebPackageLoader() {
  // DCHECK(!loader_callback_);
}

void WebPackageLoader::DetachFromRequestHandler() {
  detached_from_request_handler_ = true;
  MaybeDestruct();
}

void WebPackageLoader::AttachToRequestHandler(
    WebPackageRequestHandler* request_handler,
    LoaderCallback loader_callback) {
  request_handler_ = request_handler;
  loader_callback_ = std::move(loader_callback);
  MaybeRunLoaderCallback();
}

void WebPackageLoader::MaybeDestruct() {
  if (finished_reading_package_ && detached_from_request_handler_)
    base::ThreadTaskRunnerHandle::Get()->DeleteSoon(FROM_HERE, this);
}

void WebPackageLoader::MaybeRunLoaderCallback() {
  if (!loader_callback_ || !webpackage_start_url_.is_valid())
    return;
  std::move(loader_callback_)
      .Run(base::BindOnce(&WebPackageLoader::StartRedirectResponse,
                          weak_factory_.GetWeakPtr()));
}

void WebPackageLoader::OnFoundManifest(const WebPackageManifest& manifest) {
  DCHECK(manifest.start_url.is_valid());
  webpackage_start_url_ = manifest.start_url;
  MaybeRunLoaderCallback();
}

void WebPackageLoader::OnFinishedPackage() {
  finished_reading_package_ = true;
  MaybeDestruct();
}

//------------------------------------------------------------------------
// mojom::URLLoader overrides.

void WebPackageLoader::FollowRedirect() {
  network_loader_->FollowRedirect();
}

void WebPackageLoader::SetPriority(net::RequestPriority priority,
                                   int32_t intra_priority_value) {
  network_loader_->SetPriority(priority, intra_priority_value);
}

void WebPackageLoader::PauseReadingBodyFromNet() {
  network_loader_->PauseReadingBodyFromNet();
}

void WebPackageLoader::ResumeReadingBodyFromNet() {
  network_loader_->ResumeReadingBodyFromNet();
}

//-------------------------------------------------------------------------
// mojom::URLLoaderClient for reading data from the network.

void WebPackageLoader::OnReceiveResponse(
    const ResourceResponseHead& response_head,
    const base::Optional<net::SSLInfo>& ssl_info,
    mojom::DownloadedTempFilePtr downloaded_file) {
  DCHECK(!downloaded_file);
}

void WebPackageLoader::OnReceiveRedirect(
    const net::RedirectInfo& redirect_info,
    const ResourceResponseHead& response_head) {
  std::move(loader_callback_)
      .Run(base::BindOnce(&ForwardRedirectResponse,
                          redirect_info, response_head));
  network_client_binding_.Close();
  network_loader_.reset();
}

void WebPackageLoader::OnDataDownloaded(int64_t data_len,
                                        int64_t encoded_data_len) {
  // Unexpected.
  NOTREACHED();
}

void WebPackageLoader::OnUploadProgress(int64_t current_position,
                                        int64_t total_size,
                                        OnUploadProgressCallback ack_callback) {
  NOTIMPLEMENTED();
}

void WebPackageLoader::OnReceiveCachedMetadata(
    const std::vector<uint8_t>& data) {
  NOTIMPLEMENTED();
}

void WebPackageLoader::OnTransferSizeUpdated(int32_t transfer_size_diff) {
  NOTIMPLEMENTED();
}

void WebPackageLoader::OnStartLoadingResponseBody(
    mojo::ScopedDataPipeConsumerHandle body) {
  DCHECK(!reader_);
  reader_ =
      base::MakeRefCounted<WebPackageReaderAdapter>(this, std::move(body));
}

void WebPackageLoader::OnComplete(
    const network::URLLoaderCompletionStatus& status) {
  // TODO(kinuko): Handle failure cases?
}

void WebPackageLoader::StartRedirectResponse(mojom::URLLoaderRequest request,
                                             mojom::URLLoaderClientPtr client) {
  LOG(INFO) << "*** StartRedirectResponse: " << webpackage_start_url_;
  net::RedirectInfo redirect_info;
  redirect_info.new_url = webpackage_start_url_;
  redirect_info.new_method = "GET";
  redirect_info.status_code = 302;
  redirect_info.new_site_for_cookies = redirect_info.new_url;
  ResourceResponseHead response_head;
  response_head.encoded_data_length = 0;
  std::string buf(base::StringPrintf("HTTP/1.1 %d %s\r\n", 302, "Found"));
  response_head.headers = new net::HttpResponseHeaders(
      net::HttpUtil::AssembleRawHeaders(buf.c_str(), buf.size()));
  response_head.encoded_data_length = 0;
  client->OnReceiveRedirect(redirect_info, response_head);
  client->OnComplete(network::URLLoaderCompletionStatus(net::OK));

  redirect_to_mainresource_happened_ = true;
  request_handler_->OnRedirectedToMainResource(webpackage_start_url_, reader_);
}

}  // namespace content
