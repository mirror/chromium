// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_UI_PUBLIC_INTERFACES_CURSOR_CURSOR_STRUCT_TRAITS_H_
#define SERVICES_UI_PUBLIC_INTERFACES_CURSOR_CURSOR_STRUCT_TRAITS_H_

#include "services/ui/public/interfaces/window_manager_constants.mojom-shared.h"
#include "ui/display/manager/display_manager.h"

namespace mojo {

template <>
struct EnumTraits<ui::mojom::MultiDisplayMode,
                  display::DisplayManager::MultiDisplayMode> {
  static ui::mojom::MultiDisplayMode ToMojom(
      display::DisplayManager::MultiDisplayMode input);
  static bool FromMojom(ui::mojom::MultiDisplayMode input,
                        display::DisplayManager::MultiDisplayMode* out);
};

}  // namespace mojo

#endif  // SERVICES_UI_PUBLIC_INTERFACES_CURSOR_CURSOR_STRUCT_TRAITS_H_
