// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_PUBLIC_BROWSER_FILE_URL_LOADER_H_
#define CONTENT_PUBLIC_BROWSER_FILE_URL_LOADER_H_

#include <stddef.h>

#include "base/files/file_path.h"
#include "content/common/content_export.h"
#include "content/public/common/url_loader.mojom.h"

namespace content {

struct ResourceRequest;

// Helper to create a self-owned URLLoader instance which fulfills |request|
// using the contents of the file at |path|. The URL in |request| is ignored,
// but headers (such as range specifiers) are observed.
void CONTENT_EXPORT
CreateFileURLLoaderWithPathOverride(const ResourceRequest& request,
                                    const base::FilePath& path,
                                    mojom::URLLoaderRequest loader,
                                    mojom::URLLoaderClientPtr client);

}  // namespace content

#endif  // CONTENT_PUBLIC_BROWSER_FILE_URL_LOADER_H_
