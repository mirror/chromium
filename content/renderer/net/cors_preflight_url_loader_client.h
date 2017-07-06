// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDERER_NET_CORS_PREFLIGHT_URL_LOADER_CLIENT_H_
#define CONTENT_RENDERER_NET_CORS_PREFLIGHT_URL_LOADER_CLIENT_H_

#include <stdint.h>
#include <vector>
#include "base/callback_forward.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "content/common/content_export.h"
#include "content/common/url_loader.mojom.h"
#include "ipc/ipc_message.h"
#include "mojo/public/cpp/system/data_pipe.h"

namespace base {
class SingleThreadTaskRunner;
}  // namespace base

namespace net {
struct RedirectInfo;
}  // namespace net

namespace content {
struct ResourceRequestCompletionStatus;
struct ResourceResponseHead;

class CORSPreflightCallback {
 public:
  virtual void OnPreflightResponse(scoped_refptr<ResourceResponse>) = 0;
  virtual void OnPreflightRedirect(
      const net::RedirectInfo& redirect_info,
      const ResourceResponseHead& response_head) = 0;
};

class CORSPreflightURLLoaderClient final : public mojom::URLLoaderClient {
 public:
  CORSPreflightURLLoaderClient(
      CORSPreflightCallback&,
      scoped_refptr<base::SingleThreadTaskRunner> task_runner);
  ~CORSPreflightURLLoaderClient() override;

  // mojom::URLLoaderClient implementation
  void OnReceiveResponse(const ResourceResponseHead& response_head,
                         const base::Optional<net::SSLInfo>& ssl_info,
                         mojom::DownloadedTempFilePtr downloaded_file) override;
  void OnReceiveRedirect(const net::RedirectInfo& redirect_info,
                         const ResourceResponseHead& response_head) override;
  void OnDataDownloaded(int64_t data_len, int64_t encoded_data_len) override;
  void OnUploadProgress(int64_t current_position,
                        int64_t total_size,
                        OnUploadProgressCallback ack_callback) override;
  void OnReceiveCachedMetadata(const std::vector<uint8_t>& data) override;
  void OnTransferSizeUpdated(int32_t transfer_size_diff) override;
  void OnStartLoadingResponseBody(
      mojo::ScopedDataPipeConsumerHandle body) override;
  void OnComplete(const ResourceRequestCompletionStatus& status) override;

 private:
  CORSPreflightCallback& callback_;
  scoped_refptr<base::SingleThreadTaskRunner> task_runner_;
  base::WeakPtrFactory<CORSPreflightURLLoaderClient> weak_factory_;
};

}  // namespace content

#endif  // CONTENT_RENDERER_NET_CORS_PREFLIGHT_URL_LOADER_CLIENT_H_
