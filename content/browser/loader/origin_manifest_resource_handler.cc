// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/loader/origin_manifest_resource_handler.h"

#include "base/bind.h"
#include "base/strings/string_util.h"
#include "base/threading/platform_thread.h"
#include "content/browser/loader/resource_controller.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/origin_manifest_store.h"
#include "content/public/browser/resource_request_info.h"
#include "content/public/browser/storage_partition.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/resource_response.h"
#include "net/url_request/redirect_info.h"
#include "net/url_request/url_fetcher.h"
#include "net/url_request/url_request_context.h"
#include "net/url_request/url_request_context_getter.h"

namespace content {

OriginManifestResourceHandler::OriginManifestResourceHandler(
    net::URLRequest* request,
    std::unique_ptr<ResourceHandler> next_handler)
    : LayeredResourceHandler(request, std::move(next_handler)) {}

OriginManifestResourceHandler::~OriginManifestResourceHandler() {}

void OriginManifestResourceHandler::Run(
    const ResourceRequestInfo::WebContentsGetter& web_contents_getter) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  WebContents* web_contents = web_contents_getter.Run();
  BrowserContext* browser_context = nullptr;
  if (web_contents)
    browser_context = web_contents->GetBrowserContext();

  scoped_refptr<net::URLRequestContextGetter> getter(nullptr);
  if (browser_context)
    getter = BrowserContext::GetDefaultStoragePartition(browser_context)
                 ->GetURLRequestContext();

  BrowserThread::PostTask(
      BrowserThread::IO, FROM_HERE,
      base::BindOnce(&OriginManifestResourceHandler::OnWillStartContinue,
                     base::Unretained(this), browser_context,
                     std::move(getter)));
}

void OriginManifestResourceHandler::OnWillStart(
    const GURL& url,
    std::unique_ptr<ResourceController> controller) {
  url_ = &url;
  HoldController(std::move(controller));

  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  BrowserThread::PostTask(
      BrowserThread::UI, FROM_HERE,
      base::BindOnce(&OriginManifestResourceHandler::Run,
                     base::Unretained(this),
                     ResourceRequestInfo::ForRequest(request())
                         ->GetWebContentsGetterForRequest()));
}

void OriginManifestResourceHandler::OnWillStartContinue(
    BrowserContext* browser_context,
    scoped_refptr<net::URLRequestContextGetter> getter) {
  if (!browser_context) {
    OnWillStartFinish(mojom::OriginManifestPtr(nullptr));
    return;
  }

  browser_context->GetOriginManifestStore()->BindRequest(
      mojo::MakeRequest(&store_), getter.get());
  origin_ = url_->GetOrigin().spec();
  store_->Get(origin_,
              base::BindOnce(&OriginManifestResourceHandler::OnWillStartFinish,
                             base::Unretained(this)));
}

void OriginManifestResourceHandler::OnWillStartFinish(
    mojom::OriginManifestPtr om) {
  std::string version = (om.Equals(nullptr)) ? "1" : om->version;

  request()->SetExtraRequestHeaderByName("Sec-Origin-Manifest", version, true);

  DCHECK(next_handler_.get());
  next_handler_->OnWillStart(*url_, ReleaseController());
}

void OriginManifestResourceHandler::OnRequestRedirected(
    const net::RedirectInfo& redirect_info,
    ResourceResponse* response,
    std::unique_ptr<ResourceController> controller) {
  DCHECK(next_handler_.get());
  DCHECK(response);

  redirect_info_ = base::MakeUnique<net::RedirectInfo>(redirect_info);
  response_ = response;
  HoldController(std::move(controller));

  if (!store_.is_bound()) {
    OriginManifestResourceHandler::OnRequestRedirectedContinue(
        mojom::OriginManifestPtr(nullptr));
    return;
  }

  ProcessOriginManifestHeaderValue(base::BindOnce(
      &OriginManifestResourceHandler::OnRequestRedirectedContinue,
      base::Unretained(this)));
}

void OriginManifestResourceHandler::OnRequestRedirectedContinue(
    mojom::OriginManifestPtr om) {
  next_handler_->OnRequestRedirected(*redirect_info_, response_.get(),
                                     ReleaseController());
  response_ = nullptr;
  redirect_info_ = nullptr;
}

void OriginManifestResourceHandler::OnResponseStarted(
    ResourceResponse* response,
    std::unique_ptr<ResourceController> controller) {
  DCHECK(next_handler_.get());
  DCHECK(response);

  response_ = response;
  HoldController(std::move(controller));

  if (!store_.is_bound()) {
    OriginManifestResourceHandler::OnResponseStartedContinue(
        mojom::OriginManifestPtr(nullptr));
    return;
  }

  ProcessOriginManifestHeaderValue(
      base::BindOnce(&OriginManifestResourceHandler::OnResponseStartedContinue,
                     base::Unretained(this)));
}

void OriginManifestResourceHandler::ProcessOriginManifestHeaderValue(
    base::OnceCallback<void(mojom::OriginManifestPtr)> callback) {
  DCHECK(response_);

  std::string version;
  if (!response_->head.headers->EnumerateHeader(nullptr, "Sec-Origin-Manifest",
                                                &version)) {
    // There's no header. But we can still try to work with what we have already
    store_->Get(origin_, std::move(callback));
    return;
  }

  if (version == "0") {
    store_->Remove(origin_, base::BindOnce(std::move(callback),
                                           mojom::OriginManifestPtr(nullptr)));
    return;
  }

  base::TrimString(version, "\"", &version);
  store_->GetOrFetch(origin_, version, std::move(callback));
}

void OriginManifestResourceHandler::OnResponseStartedContinue(
    mojom::OriginManifestPtr om) {
  // TODO(dhausknecht): we are not doing anything with the |om|. And honestly, I
  // don't think this is the right place to do it anyways.
  next_handler_->OnResponseStarted(response_.get(), ReleaseController());
  response_ = nullptr;
}

}  // namespace content
