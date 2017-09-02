// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/exported/WebUnhandledTapInfo.h"
#include "core/frame/LocalFrame.h"
#include "mojo/public/cpp/bindings/strong_binding.h"
#include "platform/wtf/PtrUtil.h"
#include "ui/gfx/geometry/mojo/geometry.mojom.h"
#include "public/web/unhandled_tap_info.mojom.h" // TODO(donnd): use mojom-blink.h?
#include "services/service_manager/public/cpp/interface_provider.h"

namespace blink {

WebUnhandledTapInfo::WebUnhandledTapInfo(LocalFrame& frame) : frame_(frame) {}

void WebUnhandledTapInfo::ShowUnhandledTapUIIfNeeded(
    bool dom_tree_changed,
    bool style_changed,
    Node* tapped_node,
    IntPoint tapped_position_in_viewport) const {
  // Construct a data record and send it down the mojo pipe.
  WebNode web_node(tapped_node);  // TODO(donnd): unwrap!!!!!!!!!
  auto tapped_info = mojom::blink::UnhandledTapInfo::New(
      dom_tree_changed, style_changed, tapped_position_in_viewport.X(),
      tapped_position_in_viewport.Y(),
      tapped_node->IsTextNode(), web_node.IsContentEditable(),
      web_node.IsInsideFocusableElementOrARIAWidget());

  mojom::blink::UnhandledTapNotifierServicePtr service_ptr = nullptr;
  //DVLOG(0) << "ctxs service_ptr first: " << service_ptr == nullptr;
  frame_->GetInterfaceProvider().GetInterface(
      mojo::MakeRequest(&service_ptr));
 // DVLOG(0) << "ctxs service_ptr now: " << service_ptr == nullptr;
  if (service_ptr && tapped_info) {
    DVLOG(0) << "ctxs actually calling through WebUnhandledTapInfo ";
    service_ptr->ShowUnhandledTapUIIfNeeded(std::move(tapped_info));
  } else {
    DVLOG(0) << "ctxs coudn't call WebUnhandledTapInfo ";
  }
}

// TODO(donnd): Construct a data record and send it down the mojo pipe
void WebUnhandledTapInfo::ShowUnhandledTapUIIfNeeded(
    mojom::blink::UnhandledTapInfoPtr unhandled_tap_info) {}

// static
void WebUnhandledTapInfo::BindMojoRequest(
    LocalFrame* frame,
    mojom::blink::UnhandledTapNotifierServiceRequest request) {
  DCHECK(frame);

  mojo::MakeStrongBinding(WTF::WrapUnique(new WebUnhandledTapInfo(*frame)),
                          std::move(request));
}

}  // namespace blink
