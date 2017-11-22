// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/loader/webpackage_reader_adapter.h"

#include "base/bind.h"
#include "base/location.h"
#include "base/threading/thread_task_runner_handle.h"
#include "base/time/time.h"
#include "content/public/common/resource_request.h"

namespace content {

WebPackageManifest::WebPackageManifest() = default;
WebPackageManifest::~WebPackageManifest() = default;

WebPackageReaderAdapter::WebPackageReaderAdapter(
    WebPackageReaderAdapterClient* client,
    mojo::ScopedDataPipeConsumerHandle handle)
    : client_(client),
      handle_(std::move(handle)),
      handle_watcher_(FROM_HERE, mojo::SimpleWatcher::ArmingPolicy::MANUAL),
      reader_(std::make_unique<wpkg::WebPackageReader>(this)),
      weak_factory_(this) {
  handle_watcher_.Watch(
      handle_.get(), MOJO_HANDLE_SIGNAL_READABLE,
      base::Bind(&WebPackageReaderAdapter::OnReadable,
                 base::Unretained(this)));
  handle_watcher_.ArmOrNotify();
}

WebPackageReaderAdapter::~WebPackageReaderAdapter() = default;

bool WebPackageReaderAdapter::GetResource(
    const ResourceRequest& resource_request,
    ResourceCallback callback) {
  // TODO(kinuko): implement.
  return false;
}

void WebPackageReaderAdapter::OnReadable(MojoResult unused) {
  while (true) {
    const void* buffer = nullptr;
    uint32_t available = 0;
    MojoResult result =
        handle_->BeginReadData(&buffer, &available, MOJO_READ_DATA_FLAG_NONE);
    if (result == MOJO_RESULT_SHOULD_WAIT) {
      handle_watcher_.ArmOrNotify();
      return;
    }
    if (result == MOJO_RESULT_BUSY)
      return;
    if (result == MOJO_RESULT_FAILED_PRECONDITION) {
      // The end of data.
      NotifyCompletion(net::OK);
      return;
    }
    if (result != MOJO_RESULT_OK) {
      NotifyCompletion(net::ERR_FAILED);
      return;
    }
    // TODO(kinuko): Also cap how much we read in this task.
    if (available == 0) {
      result = handle_->EndReadData(0);
      DCHECK_EQ(result, MOJO_RESULT_OK);
      handle_watcher_.ArmOrNotify();
      return;
    }
    reader_->ConsumeDataChunk(buffer, available);
  }
}

void WebPackageReaderAdapter::NotifyCompletion(int error_code) {
  // What else to do?
  reader_->ConsumeEOS();
}

void WebPackageReaderAdapter::OnOrigin(const std::string& origin) {
  WebPackageManifest manifest;
  // Just for now.
  manifest.start_url = GURL(origin);
  client_->OnFoundManifest(manifest);
}

void WebPackageReaderAdapter::OnResourceRequest(
    const wpkg::HttpHeaders& request_headers,
    int request_id) {
  // TODO(kinuko): Implement.
}

void WebPackageReaderAdapter::OnResourceResponse(
    int request_id,
    const wpkg::HttpHeaders& response_headers) {
  // TODO(kinuko): Implement.
}

}  // namespace content
