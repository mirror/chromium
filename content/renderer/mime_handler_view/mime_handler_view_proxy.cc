// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/mime_handler_view/mime_handler_view_proxy.h"

#include "content/common/frame_messages.h"
#include "content/public/common/content_client.h"
#include "content/public/common/content_features.h"
#include "content/public/common/webplugininfo.h"
#include "content/public/renderer/content_renderer_client.h"
#include "content/renderer/render_frame_impl.h"
#include "content/renderer/render_frame_proxy.h"
#include "third_party/WebKit/public/platform/WebString.h"
#include "third_party/WebKit/public/platform/WebURL.h"
#include "third_party/WebKit/public/web/WebFrame.h"
#include "third_party/WebKit/public/web/WebLocalFrame.h"

namespace content {
MimeHandlerViewProxy::MimeHandlerViewProxy(RenderFrameImpl* render_frame_impl)
    : render_frame_impl_(render_frame_impl) {}

MimeHandlerViewProxy::~MimeHandlerViewProxy() {}

// static
MimeHandlerViewProxy* MimeHandlerViewProxy::FromWebFrame(
    blink::WebFrame* frame) {
  if (!frame || !frame->IsWebLocalFrame())
    return nullptr;
  return static_cast<RenderFrameImpl*>(RenderFrame::FromWebFrame(frame))
      ->mime_handler_view_proxy();
}

// static
void MimeHandlerViewProxy::OverrrideURLForPDFEmbed(
    const blink::WebURL& url,
    const blink::WebString& original_mime_type,
    blink::WebURL* overriden_url) {
  if (!base::FeatureList::IsEnabled(features::kWebAccessiblePdfExtension))
    return;

  if (original_mime_type.Utf8() == "application/x-google-chrome-pdf") {
    // TODO(ekaramad): Is this the best way to verify we are not dealing with
    // PDF plugin here? Maybe use a constant at the very least?
    return;
  }

#if BUILDFLAG(ENABLE_PLUGINS)
  WebPluginInfo info;
  std::string mime_type;
  bool found = false;
  render_frame_impl_->Send(new FrameHostMsg_GetPluginInfo(
      render_frame_impl_->GetRoutingID(), url,
      render_frame_impl_->GetWebFrame()->Top()->GetSecurityOrigin(), "", &found,
      &info, &mime_type));
  if (!found)
    return;

  *overriden_url =
      GetContentClient()->renderer()->OverridePDFEmbedWithHTML(url, mime_type);
#endif
}

void MimeHandlerViewProxy::MaybeRequestPDFResource(
    RenderFrameImpl* navigating_frame,
    const GURL& complete_url) {
  if (!base::FeatureList::IsEnabled(features::kWebAccessiblePdfExtension))
    return;

  GURL url(complete_url);

  if (GetContentClient()->renderer()->MaybeRequestPDFResource(navigating_frame,
                                                              complete_url)) {
    routing_id_map_[navigating_frame->GetRoutingID()] = MSG_ROUTING_NONE;
  }
}

v8::Local<v8::Object> MimeHandlerViewProxy::GetV8ScriptableObjectForPluginFrame(
    v8::Isolate* isolate,
    blink::WebFrame* frame) {
  int original_frame_routing_id = MSG_ROUTING_NONE;
  if (frame->IsWebLocalFrame()) {
    original_frame_routing_id =
        RenderFrameImpl::FromWebFrame(frame)->GetRoutingID();
  } else {
    original_frame_routing_id = GetOriginalFrameRoutingIDFromProxy(
        RenderFrameProxy::FromWebFrame(frame->ToWebRemoteFrame())
            ->routing_id());
  }
  CHECK(routing_id_map_.count(original_frame_routing_id));
  return GetContentClient()->renderer()->GetV8ScriptableObjectForPluginFrame(
      isolate, frame, original_frame_routing_id);
}

void MimeHandlerViewProxy::FrameSwappedWithProxy(
    int32_t render_frame_routing_id,
    int32_t render_frame_proxy_routing_id) {
  if (!routing_id_map_.count(render_frame_routing_id))
    return;
  CHECK_EQ(routing_id_map_[render_frame_routing_id], MSG_ROUTING_NONE);
  routing_id_map_[render_frame_routing_id] = render_frame_proxy_routing_id;
}

void MimeHandlerViewProxy::FrameDetached(int32_t routing_id) {
  if (routing_id_map_.count(routing_id)) {
    if (routing_id_map_[routing_id] != MSG_ROUTING_NONE) {
      // The frame is detached because it has swapped out with a proxy and we
      // already know about it. This is expected.
      return;
    }
    routing_id_map_.erase(routing_id);
  }
  {
    int original_routing_id = GetOriginalFrameRoutingIDFromProxy(routing_id);
    if (original_routing_id == MSG_ROUTING_NONE)
      return;
    // This is a proxy being detached. We should now clear the state.
    routing_id_map_.erase(original_routing_id);
  }
}

int32_t MimeHandlerViewProxy::GetOriginalFrameRoutingIDFromProxy(int proxy_id) {
  for (const auto& pair : routing_id_map_) {
    if (pair.second == proxy_id)
      return pair.first;
  }
  return MSG_ROUTING_NONE;
}

}  // namespace content
