// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/renderer/mime_handler_view/mime_handler_view_manager.h"

#include "base/lazy_instance.h"
#include "content/public/renderer/render_frame.h"
#include "extensions/common/constants.h"
#include "extensions/renderer/mime_handler_view/mime_handler_view_frame_controller.h"
#include "ipc/ipc_message.h"
#include "third_party/WebKit/public/platform/WebData.h"
#include "third_party/WebKit/public/web/WebDocument.h"
#include "third_party/WebKit/public/web/WebElement.h"
#include "third_party/WebKit/public/web/WebFrame.h"
#include "third_party/WebKit/public/web/WebLocalFrame.h"
#include "third_party/WebKit/public/web/WebRemoteFrame.h"

namespace extensions {

namespace {

const char kMimeTypeApplicationPdf[] = "application/pdf";
const char kMimeTypeTextPdf[] = "text/pdf";
const char kEmbedTagTemplate[] =
    "<embed width=\"100%%\" height=\"100%%\" name=\"plugin\" id=\"plugin\""
    "src=\"%s\" type=\"%s\">";
const char kHTMLPageTemplate[] =
    "<html><body style=\"background-color: rgb(38,38,38); height: 100%%;"
    "width: 100%%; overflow: hidden; margin: 0\">%s</body></html>";

base::LazyInstance<std::unordered_map<int32_t, MimeHandlerViewManager*>>::
    DestructorAtExit g_web_local_frame_map_ = LAZY_INSTANCE_INITIALIZER;
}  // namespace

// static
MimeHandlerViewManager* MimeHandlerViewManager::Get(
    content::RenderFrame* render_frame) {
  int32_t routing_id = render_frame->GetRoutingID();
  if (!g_web_local_frame_map_.Get().count(routing_id))
    g_web_local_frame_map_.Get()[routing_id] =
        new MimeHandlerViewManager(render_frame);
  return g_web_local_frame_map_.Get()[routing_id];
}

// static
void MimeHandlerViewManager::CreateInterface(
    int32_t render_frame_routing_id,
    mime_handler_view::MimeHandlerViewManagerRequest request) {
  content::RenderFrame* render_frame =
      content::RenderFrame::FromRoutingID(render_frame_routing_id);
  if (!render_frame)
    return;

  Get(render_frame)->binding_.Bind(std::move(request));
}

bool MimeHandlerViewManager::OverrideNavigationForMimeHandlerView(
    content::RenderFrame* render_frame,
    const GURL& url,
    const std::string& actual_mime_type) {
  if (actual_mime_type != "application/pdf" && actual_mime_type != "text/pdf")
    return false;
  std::string html_str = base::StringPrintf(
      kHTMLPageTemplate,
      base::StringPrintf(kEmbedTagTemplate, url.spec().c_str(),
                         actual_mime_type.c_str())
          .c_str());
  VLOG(2) << "Loading HTML page inside frame: " << html_str;
  render_frame->GetWebFrame()->LoadHTMLString(
      blink::WebData(html_str.c_str(), html_str.length()), url);
  return true;
}

MimeHandlerViewManager::MimeHandlerViewManager(
    content::RenderFrame* render_frame)
    : content::RenderFrameObserver(render_frame),
      render_frame_routing_id_(render_frame->GetRoutingID()),
      binding_(this) {}

MimeHandlerViewManager::~MimeHandlerViewManager() {
  g_web_local_frame_map_.Get().erase(render_frame_routing_id_);
  if (binding_.is_bound())
    binding_.Close();
}

void MimeHandlerViewManager::DidCommitProvisionalLoad(
    bool unused_is_new_navigation,
    bool unused_is_same_document_navigation) {
  delete this;
}

void MimeHandlerViewManager::DidFinishLoad() {
  for (auto& controller : frame_controllers_) {
    if (!controller->did_request_resource())
      controller->DidFinishLoading(0.0f);
  }
}

void MimeHandlerViewManager::OnDestruct() {
  delete this;
}

void MimeHandlerViewManager::DocumentLoaded(
    int32_t original_render_frame_routing_id) {
  for (auto& controller : frame_controllers_) {
    if (controller->original_render_frame_routing_id() ==
        original_render_frame_routing_id) {
      return controller->DocumentLoaded();
    }
  }
  NOTREACHED();
  return;
}

void MimeHandlerViewManager::OnStreamReleased(
    int32_t original_render_frame_routing_id) {
  if (auto* controller = GetFrameController(original_render_frame_routing_id)) {
    controller->StartLoading();
  }
}

void MimeHandlerViewManager::FrameRemoved(
    int32_t original_render_frame_routing_id) {
  frame_controllers_.remove_if(
      [&original_render_frame_routing_id](
          const std::unique_ptr<MimeHandlerViewFrameController>& controller) {
        return controller->original_render_frame_routing_id() ==
               original_render_frame_routing_id;
      });
}

bool MimeHandlerViewManager::CreatePluginController(
    const blink::WebElement& owner,
    const GURL& resource_url,
    const std::string& mime_type) {
  if (mime_type != kMimeTypeTextPdf && mime_type != kMimeTypeApplicationPdf)
    return false;
  if (GURL(owner.GetDocument().Url()).host_piece() ==
      extension_misc::kPdfExtensionId) {
    return false;
  }

  if (auto* old_controller = GetFrameController(owner))
    FrameRemoved(old_controller->original_render_frame_routing_id());

  MimeHandlerViewFrameController* controller =
      new MimeHandlerViewFrameController(this, owner, resource_url);
  frame_controllers_.emplace_back(controller);

  bool in_plugin_document =
      render_frame()->GetWebFrame()->GetDocument().IsPluginDocument();
  if (!in_plugin_document || did_create_controller_for_plugin_document_)
    controller->RequestResource();

  if (!did_create_controller_for_plugin_document_ && in_plugin_document)
    did_create_controller_for_plugin_document_ = in_plugin_document;
  return true;
}

v8::Local<v8::Object> MimeHandlerViewManager::CreateV8ScriptableObject(
    const blink::WebElement& owner,
    v8::Isolate* isolate) {
  MimeHandlerViewFrameController* controller = GetFrameController(owner);
  DCHECK(controller);
  return controller->GetV8ScriptableObject(isolate);
}

void MimeHandlerViewManager::DidReceiveDataInPluginDocument(
    const blink::WebElement& owner,
    const char* data,
    size_t data_length) {
  MimeHandlerViewFrameController* controller = GetFrameController(owner);
  DCHECK(controller);
  controller->DidReceiveData(data, data_length);
}

void MimeHandlerViewManager::DidReceiveViewId(
    MimeHandlerViewFrameController* controller) {
  DCHECK_EQ(GetFrameController(controller->owner_element()), controller);
  if (auto* interface = GetBrowserInterface()) {
    interface->ReleaseStream(controller->original_render_frame_routing_id(),
                             controller->view_id());
  }
}

void MimeHandlerViewManager::DidFailLoadingResource(
    MimeHandlerViewFrameController* controller) {
  DCHECK_EQ(GetFrameController(controller->owner_element()), controller);
  frame_controllers_.remove_if(
      [&controller](
          const std::unique_ptr<MimeHandlerViewFrameController>& item) {
        return item.get() == controller;
      });
}

mime_handler_view::MimeHandlerViewManagerHost*
MimeHandlerViewManager::GetBrowserInterface() {
  DCHECK(render_frame());
  if (!browser_interface_.get()) {
    render_frame()->GetRemoteInterfaces()->GetInterface(&browser_interface_);
  }
  return browser_interface_.get();
}

MimeHandlerViewFrameController* MimeHandlerViewManager::GetFrameController(
    const blink::WebElement& owner) {
  for (auto& controller : frame_controllers_) {
    if (controller->owner_element().Equals(owner))
      return controller.get();
  }
  return nullptr;
}

MimeHandlerViewFrameController* MimeHandlerViewManager::GetFrameController(
    int32_t original_render_frame_routing_id) {
  for (auto& controller : frame_controllers_) {
    if (controller->original_render_frame_routing_id() ==
        original_render_frame_routing_id) {
      return controller.get();
    }
  }
  return nullptr;
}

}  // namespace extensions
