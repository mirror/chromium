// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_LOADER_WEBPACKAGE_SUBRESOURCE_MANAGER_IMPL_H_
#define CONTENT_BROWSER_LOADER_WEBPACKAGE_SUBRESOURCE_MANAGER_IMPL_H_

#include "content/common/webpackage_subresource_manager.mojom.h"

#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"

namespace content {

class WebPackageReaderAdapter;

class WebPackageSubresourceManagerImpl :
    public mojom::WebPackageSubresourceManager {
 public:
  explicit WebPackageSubresourceManagerImpl(
      scoped_refptr<WebPackageReaderAdapter> reader);
  ~WebPackageSubresourceManagerImpl() override;

  void OnResponse(mojom::WebPackageRequestPtr request,
                  mojom::WebPackageResponsePtr response) override;

 private:
  scoped_refptr<WebPackageReaderAdapter> reader_;
  base::WeakPtrFactory<WebPackageSubresourceManagerImpl> weak_factory_;
};

}  // namespace content

#endif  // CONTENT_BROWSER_LOADER_WEBPACKAGE_SUBRESOURCE_MANAGER_IMPL_H_
