// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/ui/ws2/window_service_client_binding.h"

#include "services/ui/ws2/window_service.h"
#include "services/ui/ws2/window_service_client.h"
#include "ui/aura/window.h"

namespace ui {
namespace ws2 {

WindowServiceClientBinding::WindowServiceClientBinding(
    WindowService* window_service,
    mojom::WindowTreeClientPtr window_tree_client,
    bool intercepts_events,
    aura::Window* initial_root)
    : window_tree_client_(std::move(window_tree_client)) {
  window_service_client_ = window_service->CreateWindowServiceClient(
      window_tree_client_.get(), intercepts_events);
  mojom::WindowTreePtr window_tree_ptr;
  auto window_tree_request = mojo::MakeRequest(&window_tree_ptr);
  binding_ = std::make_unique<mojo::Binding<mojom::WindowTree>>(
      window_service_client_.get(), std::move(window_tree_request));
  // XXX
  /*
  binding_->set_connection_error_handler(
      base::Bind(&WindowServer::DestroyTree, base::Unretained(window_server),
                 base::Unretained(tree)));
  */
  window_service_client_->InitForEmbedding(initial_root,
                                           std::move(window_tree_ptr));
}

WindowServiceClientBinding::~WindowServiceClientBinding() {}

}  // namespace ws2
}  // namespace ui
