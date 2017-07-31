// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_UI_DEVTOOLS_VIEWS_UI_DEVTOOLS_DOM_AGENT_H_
#define COMPONENTS_UI_DEVTOOLS_VIEWS_UI_DEVTOOLS_DOM_AGENT_H_

#include "components/ui_devtools/DOM.h"
#include "components/ui_devtools/Overlay.h"
#include "components/ui_devtools/devtools_base_agent.h"
#include "components/ui_devtools/views/ui_element_delegate.h"
#include "ui/aura/env_observer.h"
#include "ui/compositor/layer_delegate.h"
#include "ui/views/view.h"
#include "ui/views/widget/widget.h"

namespace aura {
class Window;
}

namespace ui_devtools {

class UIElement;

class UIDevToolsDOMAgentObserver {
 public:
  // TODO(thanhph): Use UIElement* as input argument instead.
  virtual void OnNodeBoundsChanged(int node_id) = 0;
};

class UIDevToolsDOMAgent : public ui_devtools::UiDevToolsBaseAgent<
                               ui_devtools::protocol::DOM::Metainfo>,
                           public UIElementDelegate,
                           public aura::EnvObserver,
                           public ui::LayerDelegate {
 public:
  UIDevToolsDOMAgent();
  ~UIDevToolsDOMAgent() override;

  // DOM::Backend:
  ui_devtools::protocol::Response disable() override;
  ui_devtools::protocol::Response getDocument(
      std::unique_ptr<ui_devtools::protocol::DOM::Node>* out_root) override;
  ui_devtools::protocol::Response hideHighlight() override;
  ui_devtools::protocol::Response pushNodesByBackendIdsToFrontend(
      std::unique_ptr<protocol::Array<int>> backend_node_ids,
      std::unique_ptr<protocol::Array<int>>* result) override;

  // UIElementDelegate:
  void OnUIElementAdded(UIElement* parent, UIElement* child) override;
  void OnUIElementReordered(UIElement* parent, UIElement* child) override;
  void OnUIElementRemoved(UIElement* ui_element) override;
  void OnUIElementBoundsChanged(UIElement* ui_element) override;

  void AddObserver(UIDevToolsDOMAgentObserver* observer);
  void RemoveObserver(UIDevToolsDOMAgentObserver* observer);
  UIElement* GetElementFromNodeId(int node_id);
  UIElement* window_element_root() const { return window_element_root_.get(); };
  const std::vector<aura::Window*>& root_windows() const {
    return root_windows_;
  };
  ui_devtools::protocol::Response HighlightNode(int node_id,
                                                bool show_size = false);

  // Return element id under cursor location |p| in the root coordinate.
  // First by looking for the closest window, then looking for the closest
  // widget if exists, then looking for the closest view if exists.
  int FindElementByEventHandler(const gfx::Point& p);

  // ui::LayerDelegate:
  void OnDelegatedFrameDamage(const gfx::Rect& damage_rect_in_dip) override {}
  void OnDeviceScaleFactorChanged(float device_scale_factor) override {}

 private:
  // ui::LayerDelegate:
  void OnPaintLayer(const ui::PaintContext& context) override;

  // aura::EnvObserver:
  void OnWindowInitialized(aura::Window* window) override {}
  void OnHostInitialized(aura::WindowTreeHost* host) override;

  void OnNodeBoundsChanged(int node_id);
  std::unique_ptr<ui_devtools::protocol::DOM::Node> BuildInitialTree();
  std::unique_ptr<ui_devtools::protocol::DOM::Node> BuildTreeForUIElement(
      UIElement* ui_element);
  std::unique_ptr<ui_devtools::protocol::DOM::Node> BuildTreeForWindow(
      UIElement* window_element_root,
      aura::Window* window);
  std::unique_ptr<ui_devtools::protocol::DOM::Node> BuildTreeForRootWidget(
      UIElement* widget_element,
      views::Widget* widget);
  std::unique_ptr<ui_devtools::protocol::DOM::Node> BuildTreeForView(
      UIElement* view_element,
      views::View* view);
  void RemoveDomNode(UIElement* ui_element);
  void Reset();
  void UpdateHighlight(
      const std::pair<aura::Window*, gfx::Rect>& window_and_bounds);

  bool is_building_tree_;
  bool show_size_on_canvas_;
  std::unique_ptr<UIElement> window_element_root_;
  std::unordered_map<int, UIElement*> node_id_to_ui_element_;
  std::unique_ptr<ui::Layer> layer_for_highlighting_;
  gfx::Rect screen_bounds;
  gfx::Rect hovered_element_bounds;
  std::vector<aura::Window*> root_windows_;
  base::ObserverList<UIDevToolsDOMAgentObserver> observers_;

  DISALLOW_COPY_AND_ASSIGN(UIDevToolsDOMAgent);
};

}  // namespace ui_devtools

#endif  // COMPONENTS_UI_DEVTOOLS_VIEWS_UI_DEVTOOLS_DOM_AGENT_H_
