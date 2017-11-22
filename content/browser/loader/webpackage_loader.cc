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

class HTTPErrorLoader : public mojom::URLLoader {
 public:
  HTTPErrorLoader(mojom::URLLoaderClientPtr loader_client,
                  int status_code,
                  const std::string& status_text) {
    std::string buf(base::StringPrintf("HTTP/1.1 %d %s\r\n", status_code,
                                       status_text.c_str()));
    ResourceResponseHead response_head;
    response_head.headers = new net::HttpResponseHeaders(
        net::HttpUtil::AssembleRawHeaders(buf.c_str(), buf.size()));
    loader_client->OnReceiveResponse(response_head, base::nullopt,
                                     nullptr /* download_file */);
    loader_client->OnComplete(network::URLLoaderCompletionStatus(net::OK));
  }

  void FollowRedirect() override {}
  void SetPriority(net::RequestPriority priority,
                   int32_t intra_priority_value) override {}
  void PauseReadingBodyFromNet() override {}
  void ResumeReadingBodyFromNet() override {}
};

}  // namespace

//----------------------------------------------------------------------------

WebPackageRequestHandler::WebPackageRequestHandler(
    const ResourceRequest& resource_request,
    const scoped_refptr<URLLoaderFactoryGetter>& url_loader_factory_getter)
    : resource_request_(resource_request),
      url_loader_factory_getter_(url_loader_factory_getter) {}

WebPackageRequestHandler::~WebPackageRequestHandler() = default;

void WebPackageRequestHandler::MaybeCreateLoader(
    const ResourceRequest& resource_request,
    ResourceContext* resource_context,
    LoaderCallback callback) {
  if (webpackage_loader_) {
    // Second load. This must be for the package's start_url.
    DCHECK(webpackage_loader_->IsWebPackageRequest(resource_request));

    // Create the subresource loader factory before we run the callback,
    // This is synchronously taken in MaybeCreateSubresourceLoaderParams
    // during the callback.
    subresource_loader_factory_ =
        webpackage_loader_->CreateSubresourceLoaderFactory();

    // Run the callback to asynchronously create the main resource loader.
    std::move(callback).Run(
        base::BindOnce(&WebPackageLoader::CreateMainResourceLoader,
                       webpackage_loader_->GetWeakPtr()));

    // |webpackage_loader_| will self-destruct after this point.
    webpackage_loader_.release();
    return;
  }
  // XXX quick hack.
  base::FilePath::StringType extension =
      base::FilePath(resource_request.url.PathForRequest()).Extension();
  if (extension != FILE_PATH_LITERAL(".wpkg")) {
    // Shouldn't be a wpkg, let it go.
    std::move(callback).Run(StartLoaderCallback());
    return;
  }
  DCHECK(!webpackage_loader_);
  DCHECK(url_loader_factory_getter_);
  LOG(ERROR) << "** Creating WebPackgaeLoader";
  webpackage_loader_ =
      std::make_unique<WebPackageLoader>(std::move(callback), resource_request,
                                         std::move(url_loader_factory_getter_));
}

base::Optional<SubresourceLoaderParams>
WebPackageRequestHandler::MaybeCreateSubresourceLoaderParams() {
  if (!subresource_loader_factory_)
    return base::nullopt;

  SubresourceLoaderParams params;
  params.loader_factory_info = subresource_loader_factory_.PassInterface();
  return params;
}

bool WebPackageRequestHandler::MaybeCreateLoaderForResponse(
    const ResourceResponseHead& response,
    mojom::URLLoaderPtr* loader,
    mojom::URLLoaderClientRequest* client_request) {
  // For now we don't use this code.
  return false;
}

//----------------------------------------------------------------------------

class WebPackageLoader::MainResourceLoader : public mojom::URLLoader {
 public:
  MainResourceLoader(WebPackageLoader* webpackage_loader,
                     mojom::URLLoaderRequest request,
                     mojom::URLLoaderClientPtr client)
      : url_loader_client_(std::move(client)),
        binding_(this, std::move(request)),
        weak_factory_(this) {
    // |webpackage_loader| owns |this|, and bindign_ is owned by |this|.
    binding_.set_connection_error_handler(
        base::BindOnce(&WebPackageLoader::OnFinishMainResourceLoad,
                       base::Unretained(webpackage_loader)));

    // TODO(kinuko): implement.
  }

 private:
  void FollowRedirect() override {}
  void SetPriority(net::RequestPriority priority,
                   int32_t intra_priority_value) override {}
  void PauseReadingBodyFromNet() override {}
  void ResumeReadingBodyFromNet() override {}

  // Pointer to the URLLoaderClient (e.g. NavigationURLLoader).
  mojom::URLLoaderClientPtr url_loader_client_;
  mojo::Binding<mojom::URLLoader> binding_;
  base::WeakPtrFactory<MainResourceLoader> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(MainResourceLoader);
};

class WebPackageLoader::SubresourceLoaderFactory
    : public mojom::URLLoaderFactory {
 public:
  explicit SubresourceLoaderFactory(WebPackageLoader* webpackage_loader)
      : webpackage_loader_(webpackage_loader), weak_factory_(this) {
    // |webpackage_loader| owns |this|, and |bindings_| is owned by |this|.
    bindings_.set_connection_error_handler(
        base::Bind(&WebPackageLoader::OnFinishSubresourceLoad,
                   base::Unretained(webpackage_loader)));
  }

  ~SubresourceLoaderFactory() override = default;

  // mojom::URLLoaderFactory implementation.
  void CreateLoaderAndStart(mojom::URLLoaderRequest loader_request,
                            int32_t routing_id,
                            int32_t request_id,
                            uint32_t options,
                            const ResourceRequest& resource_request,
                            mojom::URLLoaderClientPtr loader_client,
                            const net::MutableNetworkTrafficAnnotationTag&
                                traffic_annotation) override {
    webpackage_loader_->CreateSubresourceLoader(
        std::move(loader_request), routing_id, request_id, options,
        resource_request, std::move(loader_client), traffic_annotation);
  }

  void Clone(mojom::URLLoaderFactoryRequest request) override {
    bindings_.AddBinding(this, std::move(request));
  }

 private:
  WebPackageLoader* webpackage_loader_;  // Owns this.
  mojo::BindingSet<mojom::URLLoaderFactory> bindings_;
  base::WeakPtrFactory<SubresourceLoaderFactory> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(SubresourceLoaderFactory);
};

//----------------------------------------------------------------------------

WebPackageLoader::WebPackageLoader(
    LoaderCallback loader_callback,
    const ResourceRequest& resource_request,
    scoped_refptr<URLLoaderFactoryGetter> url_loader_factory_getter)
    : loader_callback_(std::move(loader_callback)),
      webpackage_request_(resource_request),
      url_loader_factory_getter_(url_loader_factory_getter),
      network_client_binding_(this),
      reader_(
          std::make_unique<WebPackageReaderAdapter>(this,
                                                    resource_request.url,
                                                    url_loader_factory_getter)),
      weak_factory_(this) {
  // Start the network loading.
  mojom::URLLoaderClientPtr loader_client;
  network_client_binding_.Bind(mojo::MakeRequest(&loader_client));
  url_loader_factory_getter->GetNetworkFactory()->get()->CreateLoaderAndStart(
      mojo::MakeRequest(&network_loader_), 0 /* routing_id */,
      0 /* request_id */, mojom::kURLLoadOptionNone /* no sniffing for now */,
      resource_request, std::move(loader_client),
      net::MutableNetworkTrafficAnnotationTag(
          kWebPackageNavigationTrafficAnnotation));
}

WebPackageLoader::~WebPackageLoader() {
  DCHECK(!loader_callback_);
}

void WebPackageLoader::DestructIfNecessary() {
  if (!main_resource_loader_ || !subresource_loader_factory_)
    return;
  delete this;
}

void WebPackageLoader::CreateMainResourceLoader(
    mojom::URLLoaderRequest loader_request,
    mojom::URLLoaderClientPtr loader_client) {
  LOG(ERROR) << "** Creating WebPackgaeLoader's MainResourceLoader";
  DCHECK(!main_resource_loader_);

  main_resource_loader_ = std::make_unique<MainResourceLoader>(
      this, std::move(loader_request), std::move(loader_client));
}

mojom::URLLoaderFactoryPtr WebPackageLoader::CreateSubresourceLoaderFactory() {
  mojom::URLLoaderFactoryPtr factory;
  DCHECK(!subresource_loader_factory_);
  subresource_loader_factory_ =
      std::make_unique<SubresourceLoaderFactory>(this);
  subresource_loader_factory_->Clone(mojo::MakeRequest(&factory));
  return factory;
}

void WebPackageLoader::CreateSubresourceLoader(
    mojom::URLLoaderRequest loader_request,
    int32_t routing_id,
    int32_t request_id,
    uint32_t options,
    const ResourceRequest& resource_request,
    mojom::URLLoaderClientPtr loader_client,
    const net::MutableNetworkTrafficAnnotationTag& traffic_annotation,
    bool allow_fallback) {
  DCHECK(reader_);
  DCHECK(subresource_loader_factory_);
  LOG(ERROR) << "** CreateSubresourceLoader: " << resource_request.url;
  /*
  // TODO(kinuko): implement.
  if (reader_->GetResource(&resource_request, base::BindOnce(&...)))
    return;
  */
  if (allow_fallback) {
    url_loader_factory_getter_->GetNetworkFactory()
        ->get()
        ->CreateLoaderAndStart(std::move(loader_request), routing_id,
                               request_id, options, resource_request,
                               std::move(loader_client), traffic_annotation);
    return;
  }
  mojo::MakeStrongBinding(std::make_unique<HTTPErrorLoader>(
                              std::move(loader_client), 404, "Not Found"),
                          std::move(loader_request));
}

void WebPackageLoader::OnFoundManifest(const WebPackageManifest& manifest) {
  DCHECK(loader_callback_);
  DCHECK(!main_resource_loader_);
  webpackage_start_url_ = manifest.start_url;
  std::move(loader_callback_)
      .Run(base::BindOnce(&WebPackageLoader::StartRedirectResponse,
                          weak_factory_.GetWeakPtr()));
}

void WebPackageLoader::OnFoundResource(const ResourceRequest& request) {
  // TODO: Push the request to the renderer asap.
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
  LOG(ERROR) << "*** StartLoadingResponseBody";
  DCHECK(!main_resource_loader_);
  DCHECK(reader_);
  reader_->Consume(std::move(body));
}

void WebPackageLoader::OnComplete(
    const network::URLLoaderCompletionStatus& status) {
  // TODO(kinuko): Handle failure cases.
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
}

void WebPackageLoader::OnFinishMainResourceLoad() {
  main_resource_loader_.reset();
  DestructIfNecessary();
}

void WebPackageLoader::OnFinishSubresourceLoad() {
  subresource_loader_factory_.reset();
  DestructIfNecessary();
}

}  // namespace content
