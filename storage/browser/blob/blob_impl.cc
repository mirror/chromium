// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "storage/browser/blob/blob_impl.h"

#include "storage/browser/blob/blob_data_handle.h"
#include "storage/browser/blob/mojo_blob_reader.h"

namespace storage {
namespace {

class ReaderDelegate : public MojoBlobReader::Delegate {
 public:
  ReaderDelegate(mojo::ScopedDataPipeProducerHandle handle,
                 mojom::Blob::ReadAllCallback callback)
      : handle_(std::move(handle)), callback_(std::move(callback)) {}

  mojo::ScopedDataPipeProducerHandle GetDataPipe() override {
    return std::move(handle_);
  }

  void OnComplete(net::Error result, uint64_t total_written_bytes) override {
    std::move(callback_).Run(mojom::BlobReadResult::New(
        result == net::OK, result, total_written_bytes));
  }

 private:
  mojo::ScopedDataPipeProducerHandle handle_;
  mojom::Blob::ReadAllCallback callback_;
};

}  // namespace

// static
base::WeakPtr<BlobImpl> BlobImpl::Create(std::unique_ptr<BlobDataHandle> handle,
                                         mojom::BlobRequest request) {
  return (new BlobImpl(std::move(handle), std::move(request)))
      ->weak_ptr_factory_.GetWeakPtr();
}

void BlobImpl::Clone(mojom::BlobRequest request) {
  bindings_.AddBinding(this, std::move(request));
}

void BlobImpl::ReadRange(uint64_t offset,
                         uint64_t length,
                         mojo::ScopedDataPipeProducerHandle handle,
                         ReadRangeCallback callback) {
  MojoBlobReader::Create(
      nullptr, handle_.get(),
      net::HttpByteRange::Bounded(offset, offset + length - 1),
      base::MakeUnique<ReaderDelegate>(std::move(handle), std::move(callback)));
}

void BlobImpl::ReadAll(mojo::ScopedDataPipeProducerHandle handle,
                       ReadAllCallback callback) {
  MojoBlobReader::Create(
      nullptr, handle_.get(), net::HttpByteRange(),
      base::MakeUnique<ReaderDelegate>(std::move(handle), std::move(callback)));
}

void BlobImpl::GetInternalUUID(GetInternalUUIDCallback callback) {
  std::move(callback).Run(handle_->uuid());
}

void BlobImpl::FlushForTesting() {
  bindings_.FlushForTesting();
  if (bindings_.empty())
    delete this;
}

BlobImpl::BlobImpl(std::unique_ptr<BlobDataHandle> handle,
                   mojom::BlobRequest request)
    : handle_(std::move(handle)), weak_ptr_factory_(this) {
  DCHECK(handle_);
  bindings_.AddBinding(this, std::move(request));
  bindings_.set_connection_error_handler(
      base::Bind(&BlobImpl::OnConnectionError, base::Unretained(this)));
}

BlobImpl::~BlobImpl() {}

void BlobImpl::OnConnectionError() {
  if (!bindings_.empty())
    return;
  delete this;
}

}  // namespace storage
