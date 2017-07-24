// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_UI_DEVTOOLS_VIEWS_UI_DEVTOOLS_OVERLAY_AGENT_H_
#define COMPONENTS_UI_DEVTOOLS_VIEWS_UI_DEVTOOLS_OVERLAY_AGENT_H_

#include "components/ui_devtools/Overlay.h"
#include "components/ui_devtools/views/ui_devtools_dom_agent.h"
#include "ui/events/event_handler.h"

namespace ui_devtools {

class UIDevToolsOverlayAgent : public ui_devtools::UiDevToolsBaseAgent<
                                   ui_devtools::protocol::Overlay::Metainfo>,
                               public UIDevToolsDOMAgentObserver,
                               public ui::EventHandler {
 public:
  explicit UIDevToolsOverlayAgent(UIDevToolsDOMAgent* dom_agent);
  ~UIDevToolsOverlayAgent() override;

  // DOM::Backend:
  ui_devtools::protocol::Response enable() override;
  ui_devtools::protocol::Response disable() override;
  ui_devtools::protocol::Response setInspectMode(
      const String& in_mode,
      protocol::Maybe<protocol::Overlay::HighlightConfig> in_highlightConfig)
      override;
  ui_devtools::protocol::Response highlightNode(
      std::unique_ptr<ui_devtools::protocol::Overlay::HighlightConfig>
          highlight_config,
      ui_devtools::protocol::Maybe<int> node_id) override;
  ui_devtools::protocol::Response hideHighlight() override;
  void OnNodeBoundsChanged(int node_id) override;

  // ui:EventHandler:
  void OnMouseEvent(ui::MouseEvent* event) override;
  void OnKeyEvent(ui::KeyEvent* event) override;

 private:
  UIDevToolsDOMAgent* dom_agent_;
  int pinned_id;

  DISALLOW_COPY_AND_ASSIGN(UIDevToolsOverlayAgent);
};

}  // namespace ui_devtools

#endif  // COMPONENTS_UI_DEVTOOLS_VIEWS_UI_DEVTOOLS_OVERLAY_AGENT_H_
