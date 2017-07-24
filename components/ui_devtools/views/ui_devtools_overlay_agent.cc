// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/ui_devtools/views/ui_devtools_overlay_agent.h"

#include "ui/aura/env.h"

namespace ui_devtools {

UIDevToolsOverlayAgent::UIDevToolsOverlayAgent(UIDevToolsDOMAgent* dom_agent)
    : dom_agent_(dom_agent) {
  DCHECK(dom_agent_);
  pinned_id = 0;
}

UIDevToolsOverlayAgent::~UIDevToolsOverlayAgent() {}

ui_devtools::protocol::Response UIDevToolsOverlayAgent::enable() {
  dom_agent_->AddObserver(this);
  return ui_devtools::protocol::Response::OK();
}

ui_devtools::protocol::Response UIDevToolsOverlayAgent::disable() {
  dom_agent_->RemoveObserver(this);
  return ui_devtools::protocol::Response::OK();
}

ui_devtools::protocol::Response UIDevToolsOverlayAgent::setInspectMode(
    const String& in_mode,
    protocol::Maybe<protocol::Overlay::HighlightConfig> in_highlightConfig) {
  pinned_id = 0;
  if (in_mode.compare("searchForNode") == 0)
    aura::Env::GetInstance()->PrependPreTargetHandler(this);
  else if (in_mode.compare("none") == 0)
    aura::Env::GetInstance()->RemovePreTargetHandler(this);
  return ui_devtools::protocol::Response::OK();
}

ui_devtools::protocol::Response UIDevToolsOverlayAgent::highlightNode(
    std::unique_ptr<ui_devtools::protocol::Overlay::HighlightConfig>
        highlight_config,
    ui_devtools::protocol::Maybe<int> node_id) {
  return dom_agent_->HighlightNode(node_id.fromJust());
}

ui_devtools::protocol::Response UIDevToolsOverlayAgent::hideHighlight() {
  return dom_agent_->hideHighlight();
}

void UIDevToolsOverlayAgent::OnNodeBoundsChanged(int node_id) {}

void UIDevToolsOverlayAgent::OnMouseEvent(ui::MouseEvent* event) {
  if (!dom_agent_->window_element_root())
    return;

  // Find node id of element whose bounds contain the mouse pointer location.
  int element_id = 0;
  dom_agent_->FindElementByEventHandler(event->root_location(), &element_id);

  if (event->type() == ui::ET_MOUSE_PRESSED && (pinned_id != element_id)) {
    event->SetHandled();
    if (element_id) {
      pinned_id = element_id;
      frontend()->nodeHighlightRequested(element_id);
      dom_agent_->HighlightNode(element_id, true);
    }
  } else if (element_id && !pinned_id) {
    frontend()->nodeHighlightRequested(element_id);
    dom_agent_->HighlightNode(element_id, false);
  }
}

void UIDevToolsOverlayAgent::OnKeyEvent(ui::KeyEvent* event) {
  if (!dom_agent_->window_element_root())
    return;

  // Exit inspection mode by pressing ESC key.
  if (event->key_code() == ui::KeyboardCode::VKEY_ESCAPE) {
    aura::Env::GetInstance()->RemovePreTargetHandler(this);
    if (pinned_id) {
      frontend()->inspectNodeRequested(pinned_id);
      dom_agent_->HighlightNode(pinned_id, true);
    }
    pinned_id = 0;
  }
}

}  // namespace ui_devtools
