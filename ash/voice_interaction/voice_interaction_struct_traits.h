// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_VOICE_INTERACTION_VOICE_INTERACTION_STRUCT_TRAITS_H_
#define ASH_VOICE_INTERACTION_VOICE_INTERACTION_STRUCT_TRAITS_H_

#include "ash/public/cpp/voice_interaction_state.h"
#include "ash/public/interfaces/voice_interaction_controller.mojom.h"

namespace mojo {

template <>
struct EnumTraits<ash::mojom::VoiceInteractionState,
                  ash::VoiceInteractionState> {
  static ash::mojom::VoiceInteractionState ToMojom(
      ash::VoiceInteractionState state) {
    switch (state) {
      case ash::VoiceInteractionState::NOT_READY:
        return ash::mojom::VoiceInteractionState::NOT_READY;
      case ash::VoiceInteractionState::STOPPED:
        return ash::mojom::VoiceInteractionState::STOPPED;
      case ash::VoiceInteractionState::RUNNING:
        return ash::mojom::VoiceInteractionState::RUNNING;
    }

    NOTREACHED() << "Invalid state: " << static_cast<int>(state);
    return ash::mojom::VoiceInteractionState::NOT_READY;
  }

  static bool FromMojom(ash::mojom::VoiceInteractionState mojom_state,
                        ash::VoiceInteractionState* state) {
    switch (mojom_state) {
      case ash::mojom::VoiceInteractionState::NOT_READY:
        *state = ash::VoiceInteractionState::NOT_READY;
        return true;
      case ash::mojom::VoiceInteractionState::STOPPED:
        *state = ash::VoiceInteractionState::STOPPED;
        return true;
      case ash::mojom::VoiceInteractionState::RUNNING:
        *state = ash::VoiceInteractionState::RUNNING;
        return true;
    }

    NOTREACHED() << "Invalid state: " << static_cast<int>(mojom_state);
    *state = ash::VoiceInteractionState::NOT_READY;
    return false;
  }
};

}  // namespace mojo

#endif  // ASH_VOICE_INTERACTION_VOICE_INTERACTION_STRUCT_TRAITS_H_
