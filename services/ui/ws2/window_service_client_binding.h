// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_UI_WS2_WINDOW_SERVICE_CLIENT_BINDING_H_
#define SERVICES_UI_WS2_WINDOW_SERVICE_CLIENT_BINDING_H_

#include <memory>

#include "base/component_export.h"
#include "base/macros.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "services/ui/public/interfaces/window_tree.mojom.h"

namespace aura {
class Window;
}

namespace ui {
namespace ws2 {

class WindowService;
class WindowServiceClient;

class COMPONENT_EXPORT(WINDOW_SERVICE) WindowServiceClientBinding {
 public:
  // TODO: add support for callback when binding destroyed.
  WindowServiceClientBinding();
  ~WindowServiceClientBinding();

  void InitForEmbed(WindowService* window_service,
                    mojom::WindowTreeClientPtr window_tree_client,
                    bool intercepts_events,
                    aura::Window* initial_root);

  void InitFromFactory(WindowService* window_service,
                       mojom::WindowTreeRequest window_tree_request,
                       mojom::WindowTreeClientPtr window_tree_client,
                       bool intercepts_events);

 private:
  friend class WindowServiceClient;

  mojom::WindowTreeClientPtr window_tree_client_;
  std::unique_ptr<WindowServiceClient> window_service_client_;
  std::unique_ptr<mojo::Binding<mojom::WindowTree>> binding_;

  DISALLOW_COPY_AND_ASSIGN(WindowServiceClientBinding);
};

}  // namespace ws2
}  // namespace ui

#endif  // SERVICES_UI_WS2_WINDOW_SERVICE_CLIENT_BINDING_H_
