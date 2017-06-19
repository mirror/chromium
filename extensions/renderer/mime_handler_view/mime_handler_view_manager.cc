// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/renderer/mime_handler_view/mime_handler_view_manager.h"

#include <unordered_map>

#include "base/lazy_instance.h"
#include "base/macros.h"
#include "base/strings/stringprintf.h"
#include "content/public/common/associated_interface_provider.h"
#include "content/public/common/content_features.h"
#include "content/public/renderer/render_frame.h"
#include "extensions/common/constants.h"
#include "extensions/renderer/mime_handler_view/mime_handler_view_client.h"
#include "mojo/public/cpp/bindings/interface_request.h"
#include "services/service_manager/public/cpp/binder_registry.h"
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
      owner_frame_routing_id_(render_frame->GetRoutingID()),
      browser_interface_(nullptr),
      weak_factory_(this) {}

MimeHandlerViewManager::~MimeHandlerViewManager() {}

void MimeHandlerViewManager::Bind(
    const service_manager::BindSourceInfo& source_info,
    mime_handler_view::MimeHandlerViewManagerRequest request) {
  CHECK(!binding_);
  binding_.reset(new mojo::Binding<mime_handler_view::MimeHandlerViewManager>(
      this, std::move(request)));
}

void MimeHandlerViewManager::OnDestruct() {
  g_mime_handler_view_manager_map.Get().erase(owner_frame_routing_id_);
  delete this;
}

void MimeHandlerViewManager::MaybeRequestPDFResource(
    content::RenderFrame* navigating_frame,
    const GURL& url) {
  DCHECK_EQ(render_frame()->GetWebFrame(),
            navigating_frame->GetWebFrame()->Parent());
  DCHECK(GetBrowserInterface());

  std::string pdf_url = url.query();
  GetBrowserInterface()->RegisterMimeHandlerViewFrame(
      navigating_frame->GetRoutingID(), pdf_url, is_plugin_document_,
      base::Bind(&MimeHandlerViewManager::OnRegisteredMimeHandlerViewFrame,
                 weak_factory_.GetWeakPtr(), navigating_frame->GetRoutingID(),
                 pdf_url));
}

void MimeHandlerViewManager::ClientRenderFrameGone(int32_t id) {
  DCHECK(clients_.count(id));
  // TODO(ekaramad): RenderFrame is destroyed in the following scenarios:
  // 1- Frame swaps out since we are navigating to the extension
  // 2- Frame is detached because the <embed> is removed.
  // Case (1) is expected behavior during navigation but case (2) should lead to
  // destroying MimeHandlerViewClient. We should find a way to distinguish the
  // two cases and delete MimeHandlerViewClient is necessary.
}

MimeHandlerViewClient* MimeHandlerViewManager::GetClient(int32_t id) {
  DCHECK(clients_.count(id));
  return clients_[id].get();
}

void MimeHandlerViewManager::OnRegisteredMimeHandlerViewFrame(
    int navigating_frame_id,
    const std::string& pdf_url,
    bool request_resource) {
  content::RenderFrame* render_frame =
      content::RenderFrame::FromRoutingID(navigating_frame_id);
  if (!render_frame)
    return;

  if (!did_register_interface_) {
    render_frame->GetInterfaceRegistry()->AddInterface(
        base::Bind(&MimeHandlerViewManager::Bind, weak_factory_.GetWeakPtr()));
    did_register_interface_ = true;
  }

  clients_[navigating_frame_id] = base::MakeUnique<MimeHandlerViewClient>(
      render_frame, GURL(pdf_url), request_resource);
}

mime_handler_view::MimeHandlerViewService*
MimeHandlerViewManager::GetBrowserInterface() {
  if (!render_frame())
    return nullptr;

  if (!browser_interface_.get()) {
    render_frame()->GetRemoteInterfaces()->GetInterface(&browser_interface_);
  }
  return browser_interface_.get();
}

}  // namespace extensions
