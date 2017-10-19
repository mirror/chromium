// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_INTERFACE_PROVIDER_FILTERING_H_
#define CONTENT_BROWSER_INTERFACE_PROVIDER_FILTERING_H_

#include "base/strings/string_piece.h"
#include "services/service_manager/public/interfaces/interface_provider.mojom.h"

namespace content {

service_manager::mojom::InterfaceProviderRequest
FilterRendererExposedInterfaces(
    base::StringPiece spec,
    int process_id,
    service_manager::mojom::InterfaceProviderRequest request);

}  // namespace content
#endif  // CONTENT_BROWSER_INTERFACE_PROVIDER_FILTERING_H_
