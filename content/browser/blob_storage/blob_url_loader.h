// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_BLOB_STORAGE_BLOB_URL_LOADER_H_
#define CONTENT_BROWSER_BLOB_STORAGE_BLOB_URL_LOADER_H_

#include "base/callback.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "content/common/content_export.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/common/url_loader.mojom.h"
#include "content/public/common/url_loader_factory.mojom.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "net/base/io_buffer.h"
#include "net/http/http_byte_range.h"
#include "net/http/http_status_code.h"
#include "storage/browser/blob/mojo_blob_reader.h"

namespace storage {
class BlobDataHandle;
class FileSystemContext;
}  // namespace storage

namespace content {

// A URLLoader implementation for blob scheme. Handles a single request for a
// blob:// url. Instances of this may be created with CreateAndStart() and will
// always self-destruct when finished. Typically these are created by
// NonNetworkURLLoaderFactory.
//
// NOTE: Some of this code in this class is duplicated from
// storage::BlobURLRequestJob.
class CONTENT_EXPORT BlobURLLoader : public storage::MojoBlobReader::Delegate,
                                     public mojom::URLLoader {
 public:
  ~BlobURLLoader() override;

  static void CreateAndStart(
      mojom::URLLoaderRequest loader,
      const ResourceRequest& request,
      mojom::URLLoaderClientPtr client,
      std::unique_ptr<storage::BlobDataHandle> blob_handle,
      storage::FileSystemContext* file_system_context);

 private:
  BlobURLLoader(mojom::URLLoaderRequest url_loader_request,
                const ResourceRequest& request,
                mojom::URLLoaderClientPtr client,
                std::unique_ptr<storage::BlobDataHandle> blob_handle,
                storage::FileSystemContext* file_system_context);

  void Start(const ResourceRequest& request,
             scoped_refptr<storage::FileSystemContext> file_system_context);

  // mojom::URLLoader implementation:
  void FollowRedirect() override;
  void SetPriority(net::RequestPriority priority,
                   int32_t intra_priority_value) override;
  void PauseReadingBodyFromNet() override;
  void ResumeReadingBodyFromNet() override;

  // storage::MojoBlobReader::Delegate implementation:
  mojo::ScopedDataPipeProducerHandle PassDataPipe() override;
  RequestSideData DidCalculateSize(uint64_t total_size,
                                   uint64_t content_size) override;
  void DidReadSideData(net::IOBufferWithSize* data) override;
  void DidRead(int num_bytes) override;
  void OnComplete(net::Error error_code, uint64_t total_written_bytes) override;

  void HeadersCompleted(net::HttpStatusCode status_code,
                        uint64_t content_size,
                        net::IOBufferWithSize* metadata);

  mojo::Binding<mojom::URLLoader> binding_;
  mojom::URLLoaderClientPtr client_;

  bool byte_range_set_ = false;
  net::HttpByteRange byte_range_;

  uint64_t total_size_ = 0;
  bool sent_headers_ = false;

  std::unique_ptr<storage::BlobDataHandle> blob_handle_;
  mojo::ScopedDataPipeConsumerHandle response_body_consumer_handle_;

  base::WeakPtrFactory<BlobURLLoader> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(BlobURLLoader);
};

}  // namespace content

#endif  // CONTENT_BROWSER_BLOB_STORAGE_BLOB_URL_LOADER_H_
