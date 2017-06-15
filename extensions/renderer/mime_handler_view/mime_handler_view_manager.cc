// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/renderer/mime_handler_view/mime_handler_view_manager.h"

#include <unordered_map>

#include "base/lazy_instance.h"
#include "base/macros.h"
#include "base/strings/stringprintf.h"
#include "content/public/common/content_features.h"
#include "content/public/renderer/render_frame.h"
#include "extensions/common/constants.h"
#include "mojo/public/cpp/bindings/interface_request.h"
#include "services/service_manager/public/cpp/interface_provider.h"
#include "third_party/WebKit/public/web/WebAssociatedURLLoader.h"
#include "third_party/WebKit/public/web/WebAssociatedURLLoaderOptions.h"
#include "third_party/WebKit/public/web/WebDocument.h"
#include "third_party/WebKit/public/web/WebLocalFrame.h"

namespace extensions {

namespace {
using Callback = base::Callback<void(int, const std::string&)>;

// Maps from content::RenderFrame to the set of MimeHandlerViewManagers within
// it.
base::LazyInstance<std::unordered_map<int, MimeHandlerViewManager*>>::
    DestructorAtExit g_mime_handler_view_manager_map =
        LAZY_INSTANCE_INITIALIZER;

const int kPluginDocumentRequestId = 1;

int32_t NextUniqueId() {
  static int32_t next_mime_handler_id = kPluginDocumentRequestId + 1;
  return next_mime_handler_id++;
}

}  // namespace

// static
MimeHandlerViewManager* MimeHandlerViewManager::FromRenderFrame(
    content::RenderFrame* render_frame) {
  int routing_id = render_frame->GetRoutingID();
  if (!g_mime_handler_view_manager_map.Get().count(routing_id)) {
    g_mime_handler_view_manager_map.Get().insert(
        {routing_id, new MimeHandlerViewManager(render_frame)});
  }
  return g_mime_handler_view_manager_map.Get().at(routing_id);
}

// static
GURL MimeHandlerViewManager::GetExtensionURL(const GURL& resource_url,
                                             const std::string& mime_type) {
  if (!base::FeatureList::IsEnabled(features::kWebAccessiblePdfExtension))
    return GURL();

  if (mime_type != "application/pdf")
    return GURL();

  return GURL(base::StringPrintf(
      "%s://%s/index.html?%s", extensions::kExtensionScheme,
      extension_misc::kPdfExtensionId, resource_url.spec().c_str()));
}

MimeHandlerViewManager::MimeHandlerViewManager(
    content::RenderFrame* render_frame)
    : content::RenderFrameObserver(render_frame),
      is_plugin_document_(
          render_frame->GetWebFrame()->GetDocument().IsPluginDocument()),
      browser_interface_(nullptr),
      weak_factory_(this) {}

MimeHandlerViewManager::~MimeHandlerViewManager() {
  for (auto* client : pending_requests_)
    delete client;
}

void MimeHandlerViewManager::OnDestruct() {
  g_mime_handler_view_manager_map.Get().erase(render_frame()->GetRoutingID());
  delete this;
}

void MimeHandlerViewManager::MaybeRequestResource(
    content::RenderFrame* navigating_frame,
    const GURL& url) {
  DCHECK_EQ(render_frame()->GetWebFrame(),
            navigating_frame->GetWebFrame()->Parent());
  DCHECK(GetBrowserInterface());

  std::string pdf_url = url.query();
  GetBrowserInterface()->RegisterMimeHandlerViewFrame(
      navigating_frame->GetRoutingID(), pdf_url, is_plugin_document_,
      base::Bind(&MimeHandlerViewManager::OnRegisteredMimeHandlerViewFrame,
                 weak_factory_.GetWeakPtr(), pdf_url,
                 navigating_frame->GetRoutingID()));
}

void MimeHandlerViewManager::OnRegisteredMimeHandlerViewFrame(
    const std::string& pdf_url,
    int navigating_frame_id,
    bool request_resource) {
  if (request_resource) {
    content::RenderFrame* navigating_frame =
        content::RenderFrame::FromRoutingID(navigating_frame_id);
    if (!navigating_frame)
      return;

    pending_requests_.insert(
        new ResourceRequestClient(this, navigating_frame, GURL(pdf_url)));
  }
}

void MimeHandlerViewManager::DidFinishLoadingResource(
    ResourceRequestClient* client) {
  DCHECK(pending_requests_.count(client));
  delete client;
  pending_requests_.erase(client);
}

mime_handler_view::MimeHandlerViewService*
MimeHandlerViewManager::GetBrowserInterface() {
  if (!render_frame())
    return nullptr;

  if (!browser_interface_) {
    render_frame()->GetRemoteInterfaces()->GetInterface(
        mojo::MakeRequest(&browser_interface_));
  }
  return browser_interface_.get();
}

MimeHandlerViewManager::ResourceRequestClient::ResourceRequestClient(
    MimeHandlerViewManager* manager,
    content::RenderFrame* render_frame,
    const blink::WebURL& url)
    : manager_(manager) {
  blink::WebLocalFrame* frame = render_frame->GetWebFrame();
  blink::WebAssociatedURLLoaderOptions options;
  // The embedded plugin is allowed to be cross-origin and we should always
  // send credentials/cookies with the request.
  options.fetch_request_mode = blink::WebURLRequest::kFetchRequestModeNoCORS;
  options.allow_credentials = true;
  DCHECK(!loader_);
  loader_.reset(frame->CreateAssociatedURLLoader(options));

  blink::WebURLRequest request(url);
  request.SetRequestContext(blink::WebURLRequest::kRequestContextObject);
  loader_->LoadAsynchronously(request, this);
}

MimeHandlerViewManager::ResourceRequestClient::~ResourceRequestClient() {
  if (loader_) {
    loader_->Cancel();
    loader_.reset();
  }
}

void MimeHandlerViewManager::ResourceRequestClient::DidFinishLoading(
    double time) {
  manager_->DidFinishLoadingResource(this);
}

}  // namespace extensions
