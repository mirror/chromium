// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_MAGNIFIER_DOCKED_MAGNIFIER_H_
#define ASH_MAGNIFIER_DOCKED_MAGNIFIER_H_

#include <memory>

#include "base/macros.h"
#include "ui/events/event_handler.h"

namespace ui {
class Reflector;
class Layer;
}  // namespace ui

namespace views {
class Widget;
}  // namespace views

namespace ash {

class DockedMagnifier : public ui::EventHandler {
 public:
  DockedMagnifier();
  ~DockedMagnifier() override;

  bool enabled() const { return enabled_; }

  void SetEnabled(bool enabled);

  void Toggle();

  // ui::EventHandler:
  void OnMouseEvent(ui::MouseEvent* event) override;

 private:
  void CreateMagnifierWidgets();

  void CloseMagnifierWidgets();

  void AdjustMagnifierLayerTransform();

  views::Widget* target_widget_ = nullptr;

  std::unique_ptr<ui::Layer> solid_color_layer_;

  std::unique_ptr<ui::Layer> magnifier_layer_;

  std::unique_ptr<ui::Reflector> reflector_;

  bool enabled_ = false;

  DISALLOW_COPY_AND_ASSIGN(DockedMagnifier);
};

}  // namespace ash

#endif  // ASH_MAGNIFIER_DOCKED_MAGNIFIER_H_
