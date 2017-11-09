// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_SHELF_BACK_BUTTON_H_
#define ASH_SHELF_BACK_BUTTON_H_

#include <memory>

#include "ash/ash_export.h"
#include "base/macros.h"
#include "ui/views/controls/button/image_button.h"

namespace ash {
class InkDropButtonListener;
class Shelf;
class ShelfView;
class VoiceInteractionOverlay;

// Button used for the AppList icon on the shelf.
class ASH_EXPORT BackButton : public views::ImageButton {
 public:
  BackButton(InkDropButtonListener* listener,
             ShelfView* shelf_view,
             Shelf* shelf);
  ~BackButton() override;

  // Get the center point of the app list button back arrow. Returns an empty
  // gfx::Point if the back arrow is not shown.
  gfx::Point GetBackButtonCenterPoint() const;

 protected:
  // views::ImageButton:
  void OnGestureEvent(ui::GestureEvent* event) override;
  bool OnMousePressed(const ui::MouseEvent& event) override;
  void OnMouseReleased(const ui::MouseEvent& event) override;
  void GetAccessibleNodeData(ui::AXNodeData* node_data) override;
  std::unique_ptr<views::InkDropRipple> CreateInkDropRipple() const override;
  std::unique_ptr<views::InkDrop> CreateInkDrop() override;
  std::unique_ptr<views::InkDropMask> CreateInkDropMask() const override;
  void PaintButtonContents(gfx::Canvas* canvas) override;

 private:
  // Generate and send a VKEY_BROWSER_BACK key event when the back button
  // portion is clicked or tapped.
  void GenerateAndSendBackEvent(const ui::LocatedEvent& original_event);

  ShelfView* shelf_view_ = nullptr;
  Shelf* shelf_ = nullptr;

  DISALLOW_COPY_AND_ASSIGN(BackButton);
};

}  // namespace ash

#endif  // ASH_SHELF_BACK_BUTTON_H_
