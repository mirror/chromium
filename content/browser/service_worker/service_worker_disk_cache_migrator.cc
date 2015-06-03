// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/service_worker/service_worker_disk_cache_migrator.h"

#include "base/memory/ref_counted.h"
#include "base/strings/string_number_conversions.h"
#include "content/common/service_worker/service_worker_types.h"
#include "net/base/io_buffer.h"
#include "net/base/net_errors.h"
#include "net/disk_cache/disk_cache.h"

namespace content {

namespace {

// Disk cache entry data indices (Copied from appcache_diskcache.cc).
enum { kResponseInfoIndex, kResponseContentIndex, kResponseMetadataIndex };

}  // namespace

// A task to move a cached resource from the src DiskCache to the dest
// DiskCache. This is owned by ServiceWorkerDiskCacheMigrator.
class ServiceWorkerDiskCacheMigrator::Task {
 public:
  Task(InflightTaskMap::KeyType task_id,
       int64 resource_id,
       int32 data_size,
       ServiceWorkerDiskCache* src,
       ServiceWorkerDiskCache* dest,
       const base::WeakPtr<ServiceWorkerDiskCacheMigrator>& owner);
  ~Task();

  void Run();

  InflightTaskMap::KeyType task_id() const { return task_id_; }

 private:
  void ReadResponseInfo();
  void OnReadResponseInfo(
      const scoped_refptr<HttpResponseInfoIOBuffer>& info_buffer,
      int result);
  void OnWriteResponseInfo(
      const scoped_refptr<HttpResponseInfoIOBuffer>& info_buffer,
      int result);
  void WriteResponseMetadata(
      const scoped_refptr<HttpResponseInfoIOBuffer>& info_buffer);
  void OnWriteResponseMetadata(
      const scoped_refptr<HttpResponseInfoIOBuffer>& info_buffer,
      int result);
  void ReadResponseData();
  void OnReadResponseData(const scoped_refptr<net::IOBuffer>& buffer,
                          int result);
  void OnWriteResponseData(int result);
  void Finish(ServiceWorkerStatusCode status);

  InflightTaskMap::KeyType task_id_;
  int64 resource_id_;
  int32 data_size_;
  base::WeakPtr<ServiceWorkerDiskCacheMigrator> owner_;

  scoped_ptr<ServiceWorkerResponseReader> reader_;
  scoped_ptr<ServiceWorkerResponseWriter> writer_;
  scoped_ptr<ServiceWorkerResponseMetadataWriter> metadata_writer_;

  base::WeakPtrFactory<Task> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(Task);
};

// A wrapper class for disk_cache::Entry. This is used for holding an open entry
// and ensuring that the entry gets closed on the dtor.
class ServiceWorkerDiskCacheMigrator::WrappedEntry {
 public:
  WrappedEntry() {}

  ~WrappedEntry() {
    if (entry_)
      entry_->Close();
  }

  disk_cache::Entry* Unwrap() {
    disk_cache::Entry* entry = entry_;
    entry_ = nullptr;
    return entry;
  }

  disk_cache::Entry** GetPtr() { return &entry_; }

 private:
  disk_cache::Entry* entry_ = nullptr;

  DISALLOW_COPY_AND_ASSIGN(WrappedEntry);
};

ServiceWorkerDiskCacheMigrator::Task::Task(
    InflightTaskMap::KeyType task_id,
    int64 resource_id,
    int32 data_size,
    ServiceWorkerDiskCache* src,
    ServiceWorkerDiskCache* dest,
    const base::WeakPtr<ServiceWorkerDiskCacheMigrator>& owner)
    : task_id_(task_id),
      resource_id_(resource_id),
      data_size_(data_size),
      owner_(owner),
      weak_factory_(this) {
  DCHECK_LE(0, data_size_);
  reader_.reset(new ServiceWorkerResponseReader(resource_id, src));
  writer_.reset(new ServiceWorkerResponseWriter(resource_id, dest));
  metadata_writer_.reset(
      new ServiceWorkerResponseMetadataWriter(resource_id, dest));
}

ServiceWorkerDiskCacheMigrator::Task::~Task() {
}

void ServiceWorkerDiskCacheMigrator::Task::Run() {
  ReadResponseInfo();
}

void ServiceWorkerDiskCacheMigrator::Task::ReadResponseInfo() {
  scoped_refptr<HttpResponseInfoIOBuffer> info_buffer(
      new HttpResponseInfoIOBuffer);
  reader_->ReadInfo(info_buffer.get(),
                    base::Bind(&Task::OnReadResponseInfo,
                               weak_factory_.GetWeakPtr(), info_buffer));
}

void ServiceWorkerDiskCacheMigrator::Task::OnReadResponseInfo(
    const scoped_refptr<HttpResponseInfoIOBuffer>& info_buffer,
    int result) {
  if (result < 0) {
    LOG(ERROR) << "Failed to read the response info";
    Finish(SERVICE_WORKER_ERROR_FAILED);
    return;
  }
  writer_->WriteInfo(info_buffer.get(),
                     base::Bind(&Task::OnWriteResponseInfo,
                                weak_factory_.GetWeakPtr(), info_buffer));
}

void ServiceWorkerDiskCacheMigrator::Task::OnWriteResponseInfo(
    const scoped_refptr<HttpResponseInfoIOBuffer>& buffer,
    int result) {
  if (result < 0) {
    LOG(ERROR) << "Failed to write the response info";
    Finish(SERVICE_WORKER_ERROR_FAILED);
    return;
  }

  const net::HttpResponseInfo* http_info = buffer->http_info.get();
  if (http_info->metadata && http_info->metadata->size()) {
    WriteResponseMetadata(buffer);
    return;
  }
  ReadResponseData();
}

void ServiceWorkerDiskCacheMigrator::Task::WriteResponseMetadata(
    const scoped_refptr<HttpResponseInfoIOBuffer>& info_buffer) {
  const net::HttpResponseInfo* http_info = info_buffer->http_info.get();
  DCHECK(http_info->metadata);
  DCHECK(http_info->metadata->size());

  // |wrapped_buffer| does not own the given metadata buffer, so a callback
  // for WriteMetadata keeps |info_buffer| which is the real owner of the
  // metadata buffer.
  scoped_refptr<net::WrappedIOBuffer> wrapped_buffer =
      new net::WrappedIOBuffer(http_info->metadata->data());
  metadata_writer_->WriteMetadata(
      wrapped_buffer.get(), http_info->metadata->size(),
      base::Bind(&Task::OnWriteResponseMetadata, weak_factory_.GetWeakPtr(),
                 info_buffer));
}

void ServiceWorkerDiskCacheMigrator::Task::OnWriteResponseMetadata(
    const scoped_refptr<HttpResponseInfoIOBuffer>& info_buffer,
    int result) {
  if (result < 0) {
    LOG(ERROR) << "Failed to write the response metadata";
    Finish(SERVICE_WORKER_ERROR_FAILED);
    return;
  }
  DCHECK_EQ(info_buffer->http_info->metadata->size(), result);
  ReadResponseData();
}

void ServiceWorkerDiskCacheMigrator::Task::ReadResponseData() {
  scoped_refptr<net::IOBuffer> buffer = new net::IOBuffer(data_size_);
  reader_->ReadData(buffer.get(), data_size_,
                    base::Bind(&Task::OnReadResponseData,
                               weak_factory_.GetWeakPtr(), buffer));
}

void ServiceWorkerDiskCacheMigrator::Task::OnReadResponseData(
    const scoped_refptr<net::IOBuffer>& buffer,
    int result) {
  if (result < 0) {
    LOG(ERROR) << "Failed to read the response data";
    Finish(SERVICE_WORKER_ERROR_FAILED);
    return;
  }
  DCHECK_EQ(data_size_, result);
  writer_->WriteData(
      buffer.get(), result,
      base::Bind(&Task::OnWriteResponseData, weak_factory_.GetWeakPtr()));
}

void ServiceWorkerDiskCacheMigrator::Task::OnWriteResponseData(int result) {
  if (result < 0) {
    LOG(ERROR) << "Failed to write the response data";
    Finish(SERVICE_WORKER_ERROR_FAILED);
    return;
  }
  DCHECK_EQ(data_size_, result);
  Finish(SERVICE_WORKER_OK);
}

void ServiceWorkerDiskCacheMigrator::Task::Finish(
    ServiceWorkerStatusCode status) {
  DCHECK(owner_);
  owner_->OnEntryMigrated(task_id_, status);
}

ServiceWorkerDiskCacheMigrator::ServiceWorkerDiskCacheMigrator(
    ServiceWorkerDiskCache* src,
    ServiceWorkerDiskCache* dest)
    : src_(src), dest_(dest), weak_factory_(this) {
  DCHECK(!src_->is_disabled());
  DCHECK(!dest_->is_disabled());
}

ServiceWorkerDiskCacheMigrator::~ServiceWorkerDiskCacheMigrator() {
}

void ServiceWorkerDiskCacheMigrator::Start(const StatusCallback& callback) {
  callback_ = callback;
  iterator_ = src_->disk_cache()->CreateIterator();
  OpenNextEntry();
}

void ServiceWorkerDiskCacheMigrator::OpenNextEntry() {
  DCHECK(!pending_task_);
  DCHECK(!is_iterating_);
  is_iterating_ = true;

  scoped_ptr<WrappedEntry> wrapped_entry(new WrappedEntry);
  disk_cache::Entry** entry_ptr = wrapped_entry->GetPtr();

  net::CompletionCallback callback = base::Bind(
      &ServiceWorkerDiskCacheMigrator::OnNextEntryOpened,
      weak_factory_.GetWeakPtr(), base::Passed(wrapped_entry.Pass()));
  int result = iterator_->OpenNextEntry(entry_ptr, callback);
  if (result == net::ERR_IO_PENDING)
    return;
  callback.Run(result);
}

void ServiceWorkerDiskCacheMigrator::OnNextEntryOpened(
    scoped_ptr<WrappedEntry> wrapped_entry,
    int result) {
  DCHECK(!pending_task_);
  is_iterating_ = false;

  if (result == net::ERR_FAILED) {
    // ERR_FAILED means the iterator reached the end of the enumeration.
    if (inflight_tasks_.IsEmpty())
      Complete(SERVICE_WORKER_OK);
    return;
  }

  if (result != net::OK) {
    LOG(ERROR) << "Failed to open the next entry";
    inflight_tasks_.Clear();
    Complete(SERVICE_WORKER_ERROR_FAILED);
    return;
  }

  disk_cache::ScopedEntryPtr scoped_entry(wrapped_entry->Unwrap());
  DCHECK(scoped_entry);

  int64 resource_id = kInvalidServiceWorkerResourceId;
  if (!base::StringToInt64(scoped_entry->GetKey(), &resource_id)) {
    LOG(ERROR) << "Failed to read the resource id";
    inflight_tasks_.Clear();
    Complete(SERVICE_WORKER_ERROR_FAILED);
    return;
  }

  InflightTaskMap::KeyType task_id = next_task_id_++;
  pending_task_.reset(new Task(task_id, resource_id,
                               scoped_entry->GetDataSize(kResponseContentIndex),
                               src_, dest_, weak_factory_.GetWeakPtr()));
  if (inflight_tasks_.size() < max_number_of_inflight_tasks_) {
    RunPendingTask();
    OpenNextEntry();
    return;
  }
  // |pending_task_| will run when an inflight task is completed.
}

void ServiceWorkerDiskCacheMigrator::RunPendingTask() {
  DCHECK(pending_task_);
  DCHECK_GT(max_number_of_inflight_tasks_, inflight_tasks_.size());
  InflightTaskMap::KeyType task_id = pending_task_->task_id();
  pending_task_->Run();
  inflight_tasks_.AddWithID(pending_task_.release(), task_id);
}

void ServiceWorkerDiskCacheMigrator::OnEntryMigrated(
    InflightTaskMap::KeyType task_id,
    ServiceWorkerStatusCode status) {
  DCHECK(inflight_tasks_.Lookup(task_id));
  inflight_tasks_.Remove(task_id);

  if (status != SERVICE_WORKER_OK) {
    inflight_tasks_.Clear();
    Complete(status);
    return;
  }

  if (pending_task_) {
    RunPendingTask();
    OpenNextEntry();
    return;
  }

  if (is_iterating_)
    return;

  if (inflight_tasks_.IsEmpty())
    Complete(SERVICE_WORKER_OK);
}

void ServiceWorkerDiskCacheMigrator::Complete(ServiceWorkerStatusCode status) {
  DCHECK(inflight_tasks_.IsEmpty());
  // TODO(nhiroki): Add UMA for the result of migration.
  callback_.Run(status);
}

}  // namespace content
