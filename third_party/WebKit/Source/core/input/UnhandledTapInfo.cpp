// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/input/UnhandledTapInfo.h"

#include "core/dom/Node.h"
#include "core/frame/LocalFrame.h"
#include "core/frame/LocalFrameClient.h"
#include "public/web/WebNode.h"
#include "services/service_manager/public/cpp/interface_provider.h"

namespace blink {

UnhandledTapInfo::UnhandledTapInfo(LocalFrame& frame) {
  frame.Client()->GetInterfaceProvider()->GetInterface(
      mojo::MakeRequest(&provider_));
}

UnhandledTapInfo::~UnhandledTapInfo() {}

void UnhandledTapInfo::ShowUnhandledTapUIIfNeeded(
    bool dom_tree_changed,
    bool style_changed,
    Node* tapped_node,
    IntPoint tapped_position_in_viewport) const {
  // Construct a data record and send it down the mojo pipe.
  WebNode web_node(tapped_node);
  auto tapped_info = mojom::blink::UnhandledTapInfo::New(
      dom_tree_changed, style_changed,
      // point,
      tapped_position_in_viewport.X(), tapped_position_in_viewport.Y(),
      tapped_node->IsTextNode(), web_node.IsContentEditable(),
      web_node.IsInsideFocusableElementOrARIAWidget());
  if (provider_ && tapped_info) {
    provider_->ShowUnhandledTapUIIfNeeded(std::move(tapped_info));
  }
}

}  // namespace blink
