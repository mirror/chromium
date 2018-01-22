// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_MAGNIFIER_DOCKED_MAGNIFIER_H_
#define ASH_MAGNIFIER_DOCKED_MAGNIFIER_H_

#include <memory>

#include "ash/display/window_tree_host_manager.h"
#include "base/macros.h"
#include "ui/base/ime/input_method_observer.h"
#include "ui/events/event_handler.h"

namespace aura {
class Window;
}  // namespace aura

namespace ui {
class Reflector;
class Layer;
}  // namespace ui

namespace views {
class Widget;
}  // namespace views

namespace ash {

class DockedMagnifier : public ui::EventHandler,
                        public WindowTreeHostManager::Observer,
                        public ui::InputMethodObserver {
 public:
  DockedMagnifier();
  ~DockedMagnifier() override;

  bool enabled() const { return enabled_; }

  void SetEnabled(bool enabled);

  void Toggle();

  // WindowTreeHostManager::Observer:
  void OnDisplayConfigurationChanged() override;

  // ui::EventHandler:
  void OnMouseEvent(ui::MouseEvent* event) override;
  void OnScrollEvent(ui::ScrollEvent* event) override;

  // ui::InputMethodObserver:
  void OnFocus() override {}
  void OnBlur() override {}
  void OnTextInputStateChanged(const ui::TextInputClient* client) override {}
  void OnInputMethodDestroyed(const ui::InputMethod* input_method) override {}
  void OnShowImeIfNeeded() override {}
  void OnCaretBoundsChanged(const ui::TextInputClient* client) override;

 private:
  void SwitchCurrentSourceRootWindowIfNeeded(aura::Window* cursor_root_window);

  void AdjustMagnifierLayerTransform(
      const gfx::Point& magnify_around_screen_point);

  void CreateMagnifierWidgets();

  void CloseMagnifierWidgets();

  bool enabled_ = false;

  float scale_;

  aura::Window* current_source_root_window_ = nullptr;

  views::Widget* target_widget_ = nullptr;

  std::unique_ptr<ui::Layer> splitter_layer_;

  std::unique_ptr<ui::Layer> magnifier_background_layer_;

  std::unique_ptr<ui::Layer> magnifier_layer_;

  std::unique_ptr<ui::Reflector> reflector_;

  DISALLOW_COPY_AND_ASSIGN(DockedMagnifier);
};

}  // namespace ash

#endif  // ASH_MAGNIFIER_DOCKED_MAGNIFIER_H_
