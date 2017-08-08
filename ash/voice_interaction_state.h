// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_VOICE_INTERACTION_STATE_H_
#define ASH_VOICE_INTERACTION_STATE_H_

namespace ash {

enum class VoiceInteractionState {
  STOPPED = 0,  // Not running
  RUNNING,      // Running
  WAITING       // Request is sent, but waiting to run.
};

}  // namespace ash

#endif  // ASH_VOICE_INTERACTION_STATE_H_
