// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_BLOB_STORAGE_BLOB_PROTOCOL_HANDLER_NETWORK_SERVICE_H
#define CONTENT_BROWSER_BLOB_STORAGE_BLOB_PROTOCOL_HANDLER_NETWORK_SERVICE_H

#include <memory>

#include "base/callback.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "content/common/content_export.h"
#include "content/public/browser/non_network_protocol_handler.h"

namespace storage {
class BlobStorageContext;
class FileSystemContext;
}  // namespace storage

namespace content {

// A handler for blob URLs with Network Service enabled. Constructed on the main
// thread but effectively lives and dies on the IO thread.
class CONTENT_EXPORT BlobProtocolHandlerNetworkService
    : public NonNetworkProtocolHandler {
 public:
  using BlobStorageContextGetter =
      base::OnceCallback<base::WeakPtr<storage::BlobStorageContext>()>;

  BlobProtocolHandlerNetworkService(
      BlobStorageContextGetter blob_context_getter,
      scoped_refptr<storage::FileSystemContext> filesystem_context);
  ~BlobProtocolHandlerNetworkService() override;

  // NonNetworkProtocolHandler:
  void CreateAndStartLoader(const ResourceRequest& request,
                            mojom::URLLoaderRequest loader,
                            mojom::URLLoaderClientPtr client) override;

 private:
  void InitializeOnIO(BlobStorageContextGetter blob_storage_context_getter);

  scoped_refptr<storage::FileSystemContext> file_system_context_;
  base::WeakPtr<storage::BlobStorageContext> blob_storage_context_;
  base::WeakPtrFactory<BlobProtocolHandlerNetworkService> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(BlobProtocolHandlerNetworkService);
};

}  // namespace content

#endif  // CONTENT_BROWSER_BLOB_STORAGE_BLOB_PROTOCOL_HANDLER_NETWORK_SERVICE_H
