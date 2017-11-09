// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_BASE_CLIPBOARD_MOJO_CLIPBOARD_PARAM_TRAITS_H_
#define UI_BASE_CLIPBOARD_MOJO_CLIPBOARD_PARAM_TRAITS_H_

#include "ui/base/clipboard/clipboard_types.h"
#include "ui/base/clipboard/mojo/clipboard.mojom.h"

namespace mojo {

template <>
struct EnumTraits<ui::mojom::ClipboardType, ui::ClipboardType> {
  static ui::mojom::ClipboardType ToMojom(ui::ClipboardType clipboard_type) {
    switch (clipboard_type) {
      case ui::CLIPBOARD_TYPE_COPY_PASTE:
        return ui::mojom::ClipboardType::CLIPBOARD_TYPE_COPY_PASTE;
      case ui::CLIPBOARD_TYPE_SELECTION:
        return ui::mojom::ClipboardType::CLIPBOARD_TYPE_SELECTION;
      case ui::CLIPBOARD_TYPE_DRAG:
        return ui::mojom::ClipboardType::CLIPBOARD_TYPE_DRAG;
      default:
        NOTREACHED();
        return ui::mojom::ClipboardType::CLIPBOARD_TYPE_COPY_PASTE;
    }
  }

  static bool FromMojom(ui::mojom::ClipboardType clipboard_type,
                        ui::ClipboardType* out) {
    switch (clipboard_type) {
      case ui::mojom::ClipboardType::CLIPBOARD_TYPE_COPY_PASTE:
        *out = ui::CLIPBOARD_TYPE_COPY_PASTE;
        return true;
      case ui::mojom::ClipboardType::CLIPBOARD_TYPE_SELECTION:
        *out = ui::CLIPBOARD_TYPE_SELECTION;
        return true;
      case ui::mojom::ClipboardType::CLIPBOARD_TYPE_DRAG:
        *out = ui::CLIPBOARD_TYPE_DRAG;
        return true;
      default:
        NOTREACHED();
        return false;
    }
  }
};

}  // namespace mojo

#endif  // UI_BASE_CLIPBOARD_MOJO_CLIPBOARD_PARAM_TRAITS_H_
