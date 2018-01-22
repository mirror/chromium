// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_CACHE_STORAGE_CACHE_STORAGE_BLOB_TO_DISK_CACHE_H_
#define CONTENT_BROWSER_CACHE_STORAGE_CACHE_STORAGE_BLOB_TO_DISK_CACHE_H_

#include <memory>

#include "base/callback.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "content/common/content_export.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "net/disk_cache/disk_cache.h"
#include "services/network/public/cpp/net_adapters.h"
#include "third_party/WebKit/common/blob/blob.mojom.h"

namespace content {

// Streams data from a blob and writes it to a given disk_cache::Entry.
class CONTENT_EXPORT CacheStorageBlobToDiskCache {
 public:
  using EntryAndBoolCallback =
      base::OnceCallback<void(disk_cache::ScopedEntryPtr, bool)>;

  // The buffer size used for reading from blobs and writing to disk cache.
  static const int kBufferSize;

  CacheStorageBlobToDiskCache();
  virtual ~CacheStorageBlobToDiskCache();

  // Writes the body of |blob| to |entry| with index
  // |disk_cache_body_index|. |entry| is passed to the callback once complete.
  // Only call this once per instantiation of CacheStorageBlobToDiskCache.
  void StreamBlobToCache(disk_cache::ScopedEntryPtr entry,
                         int disk_cache_body_index,
                         blink::mojom::BlobPtr blob,
                         EntryAndBoolCallback callback);

 protected:
  // Virtual for testing.
  virtual void ReadFromBlob();

 private:
  void DidWriteDataToEntry(int expected_bytes, int rv);
  void RunCallbackAndRemoveObserver(bool success);

  void OnDataPipeReadable(MojoResult result);
  void DidWriteDataToEntryMojo(int expected_bytes, int rv);

  int cache_entry_offset_;
  disk_cache::ScopedEntryPtr entry_;

  int disk_cache_body_index_;
  EntryAndBoolCallback callback_;

  mojo::ScopedDataPipeConsumerHandle consumer_handle_;
  scoped_refptr<network::MojoToNetPendingBuffer> pending_read_;
  mojo::SimpleWatcher handle_watcher_;

  base::WeakPtrFactory<CacheStorageBlobToDiskCache> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(CacheStorageBlobToDiskCache);
};

}  // namespace content

#endif  // CONTENT_BROWSER_CACHE_STORAGE_CACHE_STORAGE_BLOB_TO_DISK_CACHE_H_
