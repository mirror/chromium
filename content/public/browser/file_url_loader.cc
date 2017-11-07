// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/public/browser/file_url_loader.h"

#include "content/browser/file_url_loader_factory.h"

namespace content {

void CreateFileURLLoaderWithPathOverride(const ResourceRequest& request,
                                         const base::FilePath& path,
                                         mojom::URLLoaderRequest loader,
                                         mojom::URLLoaderClientPtr client) {
  return FileURLLoaderFactory::CreateLoaderWithPathOverride(
      request, path, std::move(loader), std::move(client));
}

}  // namespace content
