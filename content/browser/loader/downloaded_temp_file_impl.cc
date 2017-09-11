// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/loader/downloaded_temp_file_impl.h"

#include "base/memory/ptr_util.h"
#include "content/browser/loader/resource_dispatcher_host_impl.h"
#include "mojo/public/cpp/bindings/strong_binding.h"

namespace content {

// static
mojom::DownloadedTempFilePtr DownloadedTempFileImpl::CreateForFile(
    int child_id,
    int request_id) {
  mojo::InterfacePtr<mojom::DownloadedTempFile> ptr;
  mojo::MakeStrongBinding(base::MakeUnique<DownloadedTempFileImpl>(
                              child_id, request_id, Type::FILE),
                          mojo::MakeRequest(&ptr));
  return ptr;
}

// static
mojom::DownloadedTempFilePtr DownloadedTempFileImpl::CreateForBlob(
    int child_id,
    int request_id) {
  mojo::InterfacePtr<mojom::DownloadedTempFile> ptr;
  mojo::MakeStrongBinding(base::MakeUnique<DownloadedTempFileImpl>(
                              child_id, request_id, Type::BLOB),
                          mojo::MakeRequest(&ptr));
  return ptr;
}

DownloadedTempFileImpl::~DownloadedTempFileImpl() {
  switch (type_) {
    case Type::FILE:
      ResourceDispatcherHostImpl::Get()->UnregisterDownloadedTempFile(
          child_id_, request_id_);
    case Type::BLOB:
      ResourceDispatcherHostImpl::Get()->UnregisterDownloadedTempBlob(
          child_id_, request_id_);
  }
}
DownloadedTempFileImpl::DownloadedTempFileImpl(
    int child_id,
    int request_id,
    DownloadedTempFileImpl::Type type)
    : child_id_(child_id), request_id_(request_id), type_(type) {}

}  // namespace content
