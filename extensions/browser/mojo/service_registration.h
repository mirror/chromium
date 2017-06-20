// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EXTENSIONS_BROWSER_MOJO_SERVICE_REGISTRATION_H_
#define EXTENSIONS_BROWSER_MOJO_SERVICE_REGISTRATION_H_

namespace content {
class RenderFrameHost;
}

namespace extensions {

using Registry =
    service_manager::BinderRegistryWithParams<content::RenderFrameHost*>;

void AddInterfacesForFrameRegistry(Registry* registry);

}  // namespace extensions

#endif  // EXTENSIONS_BROWSER_MOJO_SERVICE_REGISTRATION_H_
