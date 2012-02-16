// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/aura/client/visibility_client.h"

#include "ui/aura/root_window.h"
#include "ui/aura/window_property.h"

DECLARE_WINDOW_PROPERTY_TYPE(aura::client::VisibilityClient*)

namespace aura {
namespace client {
namespace {

// A property key to store a client that handles window visibility changes.
const WindowProperty<VisibilityClient*>
    kRootWindowVisibilityClientProp = {NULL};
const WindowProperty<VisibilityClient*>* const
    kRootWindowVisibilityClientKey = &kRootWindowVisibilityClientProp;

}  // namespace

void SetVisibilityClient(VisibilityClient* client) {
  RootWindow::GetInstance()->SetProperty(kRootWindowVisibilityClientKey,
                                         client);
}

VisibilityClient* GetVisibilityClient() {
  return RootWindow::GetInstance()->GetProperty(kRootWindowVisibilityClientKey);
}

}  // namespace client
}  // namespace aura
