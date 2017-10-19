// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/ui/public/interfaces/window_manager_struct_traits.h"

namespace mojo {

// static
ui::mojom::MultiDisplayMode
EnumTraits<ui::mojom::MultiDisplayMode, display::DisplayManager::MultiDisplayMode>::ToMojom(
    display::DisplayManager::MultiDisplayMode input) {
  switch (input) {
    case display::DisplayManager::MultiDisplayMode::EXTENDED:
      return ui::mojom::MultiDisplayMode::EXTENDED;
    case display::DisplayManager::MultiDisplayMode::MIRRORING:
      return ui::mojom::MultiDisplayMode::MIRRORING;
    case display::DisplayManager::MultiDisplayMode::UNIFIED:
      return ui::mojom::MultiDisplayMode::UNIFIED;
  }
  NOTREACHED();
  return ui::mojom::MultiDisplayMode::EXTENDED;
}

// static
bool EnumTraits<ui::mojom::MultiDisplayMode, display::DisplayManager::MultiDisplayMode>::FromMojom(
    ui::mojom::MultiDisplayMode input,
    display::DisplayManager::MultiDisplayMode* out) {
  switch (input) {
    case ui::mojom::MultiDisplayMode::EXTENDED:
      *out = display::DisplayManager::MultiDisplayMode::EXTENDED;
      return true;
    case ui::mojom::MultiDisplayMode::MIRRORING:
      *out = display::DisplayManager::MultiDisplayMode::MIRRORING;
      return true;
    case ui::mojom::MultiDisplayMode::UNIFIED:
      *out = display::DisplayManager::MultiDisplayMode::UNIFIED;
      return true;
  }

  NOTREACHED();
  return false;
}

}  // namespace mojo
