// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_AURA_CLIENT_AURA_CONSTANTS_INTERNAL_H_
#define UI_AURA_CLIENT_AURA_CONSTANTS_INTERNAL_H_

#include "ui/aura/window.h"

// This file contains constants used internally by aura.

namespace aura {
namespace client {

// Indicates whether children of a window should be automatically restacked
// to honor transient stacking. Default is true. This property is provided
// specifically for WindowTreeClient and is not generally useful outside of
// WindowTreeClient.
extern const WindowProperty<bool>* const kRestackTransientChildren;

}  // namespace client
}  // namespace aura

#endif  // UI_AURA_CLIENT_AURA_CONSTANTS_INTERNAL_H_
