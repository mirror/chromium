// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "public/platform/WebBlobInfo.h"

#include "platform/blob/BlobData.h"

namespace blink {

WebBlobInfo::WebBlobInfo(const WebString& uuid,
                         const WebString& type,
                         long long size,
                         mojo::ScopedMessagePipeHandle handle)
    : WebBlobInfo(
          BlobDataHandle::Create(uuid,
                                 type,
                                 size,
                                 storage::mojom::blink::BlobPtrInfo(
                                     std::move(handle),
                                     storage::mojom::blink::Blob::Version_))) {}

WebBlobInfo::~WebBlobInfo() {
  blob_handle_.Reset();
}

void WebBlobInfo::assign(const WebBlobInfo& other) {
  is_file_ = other.is_file_;
  uuid_ = other.uuid_;
  type_ = other.type_;
  size_ = other.size_;
  blob_handle_ = other.blob_handle_;
  file_path_ = other.file_path_;
  file_name_ = other.file_name_;
  last_modified_ = other.last_modified_;
}

mojo::ScopedMessagePipeHandle WebBlobInfo::CloneBlobHandle() const {
  if (!blob_handle_)
    return mojo::ScopedMessagePipeHandle();
  return blob_handle_->CloneBlobPtr().PassInterface().PassHandle();
}

WebBlobInfo::WebBlobInfo(RefPtr<BlobDataHandle> handle)
    : is_file_(false),
      uuid_(handle->Uuid()),
      type_(handle->GetType()),
      size_(handle->size()),
      blob_handle_(std::move(handle)),
      last_modified_(0) {}

RefPtr<BlobDataHandle> WebBlobInfo::GetBlobHandle() const {
  return blob_handle_.Get();
}

}  // namespace blink
