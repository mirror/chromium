// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/base/ime/input_method_delegate.h"

#include "base/callback.h"
#include "ui/events/event.h"

namespace ui {
namespace internal {

ui::EventDispatchDetails InputMethodDelegate::DispatchKeyEventPostIME(
    ui::KeyEvent* key_event,
    std::unique_ptr<base::OnceCallback<void(bool)>> ack_callback) {
  ui::EventDispatchDetails details = DispatchKeyEventPostIME(key_event);
  if (ack_callback)
    std::move(*ack_callback).Run(key_event->stopped_propagation());
  return details;
}

}  // namespace internal
}  // namespace ui
