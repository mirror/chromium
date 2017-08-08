// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_SHELF_VOICE_INTERACTION_OVERLAY_H_
#define ASH_SHELF_VOICE_INTERACTION_OVERLAY_H_

#include <memory>

#include "ash/ash_export.h"
#include "base/macros.h"
#include "ui/compositor/layer_animation_observer.h"
#include "ui/views/animation/ink_drop_painted_layer_delegates.h"
#include "ui/views/view.h"

namespace ash {

class AppListButton;
class VoiceInteractionIconBackground;
class VoiceInteractionIcon;

class ASH_EXPORT VoiceInteractionOverlay : public views::View {
 public:
  explicit VoiceInteractionOverlay(AppListButton* host_view);
  ~VoiceInteractionOverlay() override;

  void StartAnimation(bool show_icon);
  void EndAnimation();
  void BurstAnimation();
  void WaitingAnimation();
  void HideAnimation();
  bool IsBursting() const { return is_bursting_; }
  bool IsWaiting() const { return is_waiting_; }

 private:
  std::unique_ptr<ui::Layer> ripple_layer_;
  std::unique_ptr<VoiceInteractionIcon> icon_layer_;
  std::unique_ptr<VoiceInteractionIconBackground> background_layer_;

  AppListButton* host_view_;

  // Indiates the current animation is in the bursting phase, which means no
  // turning back.
  bool is_bursting_ = false;

  // Indicates if currently playing the waiting animation.
  bool is_waiting_ = false;

  // Whether showing the icon animation or not.
  bool show_icon_ = false;

  views::CircleLayerDelegate circle_layer_delegate_;

  DISALLOW_COPY_AND_ASSIGN(VoiceInteractionOverlay);
};

}  // namespace ash
#endif  // ASH_SHELF_VOICE_INTERACTION_OVERLAY_H_
