// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "mojo/public/cpp/bindings/clone_traits.h"
#include "services/network/public/interfaces/url_loader_factory.mojom.h"

namespace mojo {

template <>
struct CloneTraits<network::mojom::URLLoaderFactoryPtrInfo> {
  static network::mojom::URLLoaderFactoryPtrInfo Clone(
      const network::mojom::URLLoaderFactoryPtrInfo& input) {
    if (!input.is_valid())
      return nullptr;
    network::mojom::URLLoaderFactoryPtr ptr(
        network::mojom::URLLoaderFactoryPtrInfo(
            ScopedMessagePipeHandle(input.handle().get()),
            network::mojom::URLLoaderFactory::Version_));
    network::mojom::URLLoaderFactoryPtr clone;
    ptr->Clone(MakeRequest(&clone));
    // We cheated by creating a second ScopedMessagePipeHandle for the same
    // handle, so now release that without closing the handle.
    ignore_result(ptr.PassInterface().PassHandle().release());
    return clone.PassInterface();
  }
};

}  // namespace mojo
