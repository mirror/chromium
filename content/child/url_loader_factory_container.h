// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_CHILD_URL_LOADER_FACTORY_CONTAINER_H_
#define CONTENT_CHILD_URL_LOADER_FACTORY_CONTAINER_H_

#include "content/public/common/url_loader_factory.mojom.h"

namespace content {

// Contains default URLLoaderFactory's.
struct URLLoaderFactoryContainer {
  mojom::URLLoaderFactory* network_loader_factory = nullptr;
  mojom::URLLoaderFactory* blob_loader_factory = nullptr;
};

}  // namespace content

#endif  // CONTENT_CHILD_URL_LOADER_FACTORY_CONTAINER_H_
