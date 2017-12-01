// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_LOADER_WEBPACKAGE_SUBRESOURCE_MANAGER_HOST_H_
#define CONTENT_BROWSER_LOADER_WEBPACKAGE_SUBRESOURCE_MANAGER_HOST_H_

#include "content/common/webpackage_subresource_manager.mojom.h"

#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "content/browser/loader/webpackage_reader_adapter.h"

namespace content {

// This class is self-destruct, this deletes itself when the connection
// to the subresource manager is dropped.
class WebPackageSubresourceManagerHost :
    public WebPackageReaderAdapter::PushObserver {
 public:
  using CompletionCallback = WebPackageResponseHandler::CompletionCallback;

  WebPackageSubresourceManagerHost(
      scoped_refptr<WebPackageReaderAdapter> reader,
      mojom::WebPackageSubresourceManagerPtr subresource_manager);
  ~WebPackageSubresourceManagerHost() override;

  base::WeakPtr<WebPackageSubresourceManagerHost> GetWeakPtr() {
    return weak_factory_.GetWeakPtr();
  }

  // WebPackageReaderAdapter::PushObserver:
  void OnRequest(const GURL& request_url,
                 const std::string& method) override;
  void OnResponse(const GURL& request_url,
                  const std::string& method,
                  const ResourceResponseHead& response_head,
                  mojo::ScopedDataPipeConsumerHandle body,
                  CompletionCallback* callback) override;

 private:
  void OnConnectionError();

  scoped_refptr<WebPackageReaderAdapter> reader_;
  mojom::WebPackageSubresourceManagerPtr subresource_manager_;

  base::WeakPtrFactory<WebPackageSubresourceManagerHost> weak_factory_;
};

}  // namespace content

#endif  // CONTENT_BROWSER_LOADER_WEBPACKAGE_SUBRESOURCE_MANAGER_HOST_H_
