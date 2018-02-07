// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_UI_WS2_WINDOW_SERVICE_DELEGATE_H_
#define SERVICES_UI_WS2_WINDOW_SERVICE_DELEGATE_H_

#include <stdint.h>

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "base/component_export.h"

namespace aura {
class Window;
}

namespace gfx {
class Rect;
}

namespace ui {
namespace ws2 {

class COMPONENT_EXPORT(WINDOW_SERVICE) WindowServiceDelegate {
 public:
  // A client has requested the bounds of a window that was created locally
  // change. |window| is either an embed root, or a top-level window.
  // Default implementation changes the bounds to |bounds|.
  virtual void ClientRequestedWindowBoundsChange(aura::Window* window,
                                                 const gfx::Rect& bounds);

  // A client requested a new top-level window. This implementation should
  // create a new window, parenting it in the appropriate container.
  virtual std::unique_ptr<aura::Window> NewTopLevel(
      const std::unordered_map<std::string, std::vector<uint8_t>>&
          properties) = 0;

 protected:
  virtual ~WindowServiceDelegate() = default;
};

}  // namespace ws2
}  // namespace ui

#endif  // SERVICES_UI_WS2_WINDOW_SERVICE_DELEGATE_H_
