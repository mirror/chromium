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

void UIDevToolsOverlayAgent::OnMouseEvent(ui::MouseEvent* event) {
  // Make sure the element tree has been populated before processing
  // mouse events.
  if (!dom_agent_->window_element_root())
    return;

  // Find node id of element whose bounds contain the mouse pointer location.
  int element_id =
      dom_agent_->FindElementByEventHandler(event->root_location());

  if (event->type() == ui::ET_MOUSEWHEEL && pinned_id) {
    ui::MouseWheelEvent* mouse_event = static_cast<ui::MouseWheelEvent*>(event);
    if (mouse_event->y_offset() > 0) {
      int parent_node_id = dom_agent_->GetUIElementParentOfNodeId(pinned_id);
      if (parent_node_id) {
        pinned_id = parent_node_id;
        frontend()->nodeHighlightRequested(pinned_id);
        dom_agent_->HighlightNode(pinned_id, true);
      }
    }
  } else if (pinned_id == element_id || event->type() == ui::ET_MOUSE_PRESSED) {
    event->SetHandled();
    if (element_id) {
      pinned_id = element_id;
      frontend()->nodeHighlightRequested(element_id);
      dom_agent_->HighlightNode(element_id, true);
    }
  } else if (element_id && !pinned_id) {
    frontend()->nodeHighlightRequested(element_id);
    dom_agent_->HighlightNode(element_id, false);
  } else if (element_id && pinned_id) {
    // Show distances between 2 elements.
    dom_agent_->HighlightNode(element_id, false);
    dom_agent_->ShowDistances(pinned_id, element_id);
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
