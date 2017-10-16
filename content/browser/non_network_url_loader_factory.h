// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_NON_NETWORK_URL_LOADER_FACTORY_H_
#define CONTENT_BROWSER_NON_NETWORK_URL_LOADER_FACTORY_H_

#include "base/callback.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "content/common/content_export.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/common/url_loader_factory.mojom.h"
#include "mojo/public/cpp/bindings/binding_set.h"

namespace storage {
class BlobStorageContext;
class FileSystemContext;
}  // namespace storage

namespace content {

class CONTENT_EXPORT NonNetworkURLLoaderFactory
    : public base::RefCountedThreadSafe<NonNetworkURLLoaderFactory,
                                        BrowserThread::DeleteOnIOThread>,
      public mojom::URLLoaderFactory {
 public:
  using BlobContextGetter =
      base::OnceCallback<base::WeakPtr<storage::BlobStorageContext>()>;

  static scoped_refptr<NonNetworkURLLoaderFactory> Create(
      BlobContextGetter blob_context_getter,
      scoped_refptr<storage::FileSystemContext> file_system_context);

  void BindRequest(mojom::URLLoaderFactoryRequest request);

 private:
  friend class base::DeleteHelper<NonNetworkURLLoaderFactory>;
  friend struct BrowserThread::DeleteOnThread<BrowserThread::IO>;

  explicit NonNetworkURLLoaderFactory(
      scoped_refptr<storage::FileSystemContext> file_system_context);
  ~NonNetworkURLLoaderFactory() override;

  // mojom::URLLoaderFactory:
  void CreateLoaderAndStart(mojom::URLLoaderRequest loader,
                            int32_t routing_id,
                            int32_t request_id,
                            uint32_t options,
                            const ResourceRequest& request,
                            mojom::URLLoaderClientPtr client,
                            const net::MutableNetworkTrafficAnnotationTag&
                                traffic_annotation) override;
  void Clone(mojom::URLLoaderFactoryRequest request) override;

  void InitializeOnIO(BlobContextGetter blob_storage_context_getter);
  void BindOnIO(mojom::URLLoaderFactoryRequest request);

  mojo::BindingSet<mojom::URLLoaderFactory> bindings_;
  scoped_refptr<storage::FileSystemContext> file_system_context_;
  base::WeakPtr<storage::BlobStorageContext> blob_storage_context_;

  DISALLOW_COPY_AND_ASSIGN(NonNetworkURLLoaderFactory);
};

}  // namespace content

#endif  // CONTENT_BROWSER_NON_NETWORK_URL_LOADER_FACTORY_H_
