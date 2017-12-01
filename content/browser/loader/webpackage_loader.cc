// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/loader/webpackage_loader.h"

#include "base/strings/stringprintf.h"
#include "content/browser/url_loader_factory_getter.h"
#include "content/common/navigation_subresource_loader_params.h"
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

}  // namespace

//----------------------------------------------------------------------------

// This class is destructed when the Mojo pipe bound to this object is
// dropped.
class MainResourceLoader : public mojom::URLLoader {
 public:
  MainResourceLoader(const GURL& url,
                     scoped_refptr<WebPackageReaderAdapter> reader,
                     mojom::URLLoaderClientPtr client)
      : loader_client_(std::move(client)),
        reader_(std::move(reader)),
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

  base::WeakPtrFactory<MainResourceLoader> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(MainResourceLoader);
};

// This class destroys itself when the Mojo pipe bound to this object
// is dropped, or the Mojo request is passed over to the network service.
class SubresourceLoader : public mojom::URLLoader {
 public:
  SubresourceLoader(
      mojom::URLLoaderRequest loader_request,
      scoped_refptr<WebPackageReaderAdapter> reader,
      scoped_refptr<URLLoaderFactoryGetter> loader_factory_getter,
      int32_t routing_id,
      int32_t request_id,
      uint32_t options,
      const ResourceRequest& resource_request,
      mojom::URLLoaderClientPtr loader_client,
      const net::MutableNetworkTrafficAnnotationTag& traffic_annotation,
      bool allow_fallback)
      : binding_(this, std::move(loader_request)),
        loader_client_(std::move(loader_client)),
        reader_(std::move(reader)),
        loader_factory_getter_(std::move(loader_factory_getter)),
        weak_factory_(this) {
    // LOG(ERROR) << "SubresourceeLoader " << this << " " <<
    // resource_request.url;
    binding_.set_connection_error_handler(base::Bind(
        &SubresourceLoader::OnConnectionError, base::Unretained(this)));
    reader_->GetResource(resource_request,
                         base::BindOnce(&SubresourceLoader::OnFoundResource,
                                        weak_factory_.GetWeakPtr(), routing_id,
                                        request_id, options, resource_request,
                                        traffic_annotation, allow_fallback),
                         base::BindOnce(&SubresourceLoader::OnComplete,
                                        weak_factory_.GetWeakPtr()));
  }

 private:
  void OnFoundResource(
      int32_t routing_id,
      int32_t request_id,
      uint32_t options,
      const ResourceRequest& resource_request,
      const net::MutableNetworkTrafficAnnotationTag& traffic_annotation,
      bool allow_fallback,
      int error_code,
      const ResourceResponseHead& response_head,
      mojo::ScopedDataPipeConsumerHandle body) {
    if (error_code == net::ERR_FILE_NOT_FOUND) {
      // LOG(ERROR) << "SubresourceLoader NotFound allow_fallback: "
      //           << allow_fallback << " url: " << resource_request.url;
      if (allow_fallback) {
        // Let the NetworkFactory handle it.
        loader_factory_getter_->GetNetworkFactory()->CreateLoaderAndStart(
            binding_.Unbind(), routing_id, request_id, options,
            resource_request, std::move(loader_client_), traffic_annotation);
        base::ThreadTaskRunnerHandle::Get()->DeleteSoon(FROM_HERE, this);
        return;
      }
      // TODO(kinuko): Maybe we don't want to generate 404 not found here,
      // we may want auto-reload on navigation.
      std::string buf("HTTP/1.1 404 Not Found\r\n");
      ResourceResponseHead response_head;
      response_head.headers = new net::HttpResponseHeaders(
          net::HttpUtil::AssembleRawHeaders(buf.c_str(), buf.size()));
      loader_client_->OnReceiveResponse(response_head, base::nullopt,
                                        nullptr /* download_file */);
      loader_client_->OnComplete(network::URLLoaderCompletionStatus(net::OK));
      return;
    }
    if (error_code != net::OK) {
      // LOG(ERROR) << "SubresourceLoader OnFoundResource  error_code: "
      //            << error_code << " url: " << resource_request.url;
      loader_client_->OnComplete(
          network::URLLoaderCompletionStatus(error_code));
      return;
    }
    // LOG(ERROR) << "SubresourceLoader OnFoundResource " << this
    //            << " url: " << resource_request.url;
    loader_client_->OnReceiveResponse(response_head, base::nullopt,
                                      nullptr /* download_file */);
    loader_client_->OnStartLoadingResponseBody(std::move(body));
  }
  void OnComplete(const network::URLLoaderCompletionStatus& status) {
    // LOG(ERROR) << "SubresourceLoader OnComplete " << this << " "
    //            << status.error_code;
    loader_client_->OnComplete(status);
  }
  void OnConnectionError() {
    base::ThreadTaskRunnerHandle::Get()->DeleteSoon(FROM_HERE, this);
  }

  void FollowRedirect() override {}
  void SetPriority(net::RequestPriority priority,
                   int32_t intra_priority_value) override {}
  void PauseReadingBodyFromNet() override {}
  void ResumeReadingBodyFromNet() override {}

  mojo::Binding<mojom::URLLoader> binding_;
  mojom::URLLoaderClientPtr loader_client_;

  scoped_refptr<WebPackageReaderAdapter> reader_;
  scoped_refptr<URLLoaderFactoryGetter> loader_factory_getter_;

  base::WeakPtrFactory<SubresourceLoader> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(SubresourceLoader);
};

// This class is self-destruct, when all the bindings are gone this deletes
// itself.
class SubresourceLoaderFactory : public mojom::URLLoaderFactory {
 public:
  SubresourceLoaderFactory(
      scoped_refptr<WebPackageReaderAdapter> reader,
      scoped_refptr<URLLoaderFactoryGetter> loader_factory_getter)
      : reader_(std::move(reader)),
        loader_factory_getter_(std::move(loader_factory_getter)),
        weak_factory_(this) {
    bindings_.set_connection_error_handler(base::Bind(
        &SubresourceLoaderFactory::OnConnectionError, base::Unretained(this)));
  }

  ~SubresourceLoaderFactory() override {
    LOG(ERROR) << "** SubresourceLoaderFactory:: DTOR";
  }

  // mojom::URLLoaderFactory implementation.
  void CreateLoaderAndStart(mojom::URLLoaderRequest loader_request,
                            int32_t routing_id,
                            int32_t request_id,
                            uint32_t options,
                            const ResourceRequest& resource_request,
                            mojom::URLLoaderClientPtr loader_client,
                            const net::MutableNetworkTrafficAnnotationTag&
                                traffic_annotation) override {
    LOG(ERROR) << "** CreateSubresourceLoader: " << resource_request.url;
    // This class is self-destruct.
    new SubresourceLoader(std::move(loader_request), reader_,
                          loader_factory_getter_, routing_id, request_id,
                          options, resource_request, std::move(loader_client),
                          traffic_annotation, allow_fallback_);
  }

  void Clone(mojom::URLLoaderFactoryRequest request) override {
    bindings_.AddBinding(this, std::move(request));
  }

  void set_allow_fallback(bool flag) { allow_fallback_ = flag; }

 private:
  void OnConnectionError() {
    if (bindings_.empty())
      delete this;
  }

  scoped_refptr<WebPackageReaderAdapter> reader_;
  scoped_refptr<URLLoaderFactoryGetter> loader_factory_getter_;
  mojo::BindingSet<mojom::URLLoaderFactory> bindings_;
  bool allow_fallback_ = false;

  base::WeakPtrFactory<SubresourceLoaderFactory> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(SubresourceLoaderFactory);
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
  if (webpackage_loader_) {
    // Second load. This must be for the package's start_url.
    DCHECK_EQ("GET", resource_request.method);
    DCHECK_EQ(webpackage_start_url_, resource_request.url);
    DCHECK(webpackage_reader_);

    // Create the subresource loader factory before we run the callback,
    // This is synchronously taken by MaybeCreateSubresourceLoaderParams
    // during the callback.
    DCHECK(loader_factory_getter_);
    // The factory destroys itself when |factory_ptr| is dropped.
    auto* factory = new SubresourceLoaderFactory(
        webpackage_reader_, std::move(loader_factory_getter_));
    factory->Clone(mojo::MakeRequest(&subresource_loader_factory_));

    // Run the callback to asynchronously create the main resource loader.
    std::move(callback).Run(
        base::BindOnce(&WebPackageRequestHandler::CreateMainResourceLoader,
                       weak_factory_.GetWeakPtr()));

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
  DCHECK(!webpackage_loader_);
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
  if (!subresource_loader_factory_)
    return base::nullopt;

  SubresourceLoaderParams params;
  params.loader_factory_info = subresource_loader_factory_.PassInterface();
  LOG(ERROR) << "** Returning subresource loader factory";
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
    mojom::URLLoaderRequest loader_request,
    mojom::URLLoaderClientPtr loader_client) {
  DCHECK(webpackage_reader_);
  LOG(ERROR) << "** Creating WebPackgaeLoader's MainResourceLoader";
  url::Replacements<char> remove_query;
  remove_query.ClearQuery();
  mojo::MakeStrongBinding(
      std::make_unique<MainResourceLoader>(
          webpackage_start_url_.ReplaceComponents(remove_query),
          webpackage_reader_, std::move(loader_client)),
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
  DCHECK(!loader_callback_);
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

void WebPackageLoader::OnFoundRequest(const ResourceRequest& request) {
  // TODO: Push the request to the renderer asap.
}

void WebPackageLoader::OnFinishedPackage() {
  finished_reading_package_ = true;
  MaybeDestruct();
}

//------------------------------------------------------------------------
// mojom::URLLoader overrides.

void WebPackageLoader::FollowRedirect() {
  NOTREACHED();
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
  // Unexpected.
  NOTREACHED();
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
  LOG(ERROR) << "*** StartRedirectResponse: " << webpackage_start_url_;
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

  request_handler_->OnRedirectedToMainResource(webpackage_start_url_, reader_);
}

}  // namespace content
