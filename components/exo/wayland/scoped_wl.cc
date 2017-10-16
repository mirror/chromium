// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/exo/wayland/scoped_wl.h"

#include <wayland-server-core.h>

namespace exo {
namespace wayland {

void WlDisplayDeleter::operator()(wl_display* display) const {
  wl_list* client_list = wl_display_get_client_list(display);
  while (!wl_list_empty(client_list)) {
    wl_client* client = wl_client_from_link(client_list->next);
    wl_client_destroy(client);
  }
  wl_display_destroy(display);
}

}  // namespace wayland
}  // namespace exo
