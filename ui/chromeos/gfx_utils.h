// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_CHROMEOS_GFX_UTILS_H_
#define UI_CHROMEOS_GFX_UTILS_H_

#include "ui/chromeos/ui_chromeos_export.h"

namespace gfx {
class ImageSkia;
}  // namespace gfx

namespace ui {

// Create an |icon| image with |badge| image placed in a white circle shadowed
// background.
UI_CHROMEOS_EXPORT gfx::ImageSkia CreateIconWithBadge(
    const gfx::ImageSkia& icon,
    const gfx::ImageSkia& badge);

}  // namespace ui

#endif  // UI_CHROMEOS_GFX_UTILS_H_
