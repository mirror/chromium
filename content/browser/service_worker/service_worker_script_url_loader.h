// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_SERVICE_WORKER_SERVICE_WORKER_SCRIPT_URL_LOADER_H_
#define CONTENT_BROWSER_SERVICE_WORKER_SERVICE_WORKER_SCRIPT_URL_LOADER_H_

#include "base/macros.h"
#include "content/browser/service_worker/service_worker_version.h"
#include "content/common/content_export.h"
#include "content/public/common/resource_request.h"
#include "content/public/common/url_loader.mojom.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "services/network/public/cpp/net_adapters.h"
#include "url/gurl.h"

namespace content {

class ServiceWorkerCacheWriter;
class ServiceWorkerVersion;
class URLLoaderFactoryGetter;
struct HttpResponseInfoIOBuffer;

// S13nServiceWorker:
// Used by a Service Worker for script loading only during the installation
// time. For now this is just a proxy loader for the network loader.
// Eventually this should replace the existing URLRequestJob-based request
// interception for script loading, namely ServiceWorkerWriteToCacheJob.
class CONTENT_EXPORT ServiceWorkerScriptURLLoader
    : public mojom::URLLoader,
      public mojom::URLLoaderClient {
 public:
  ServiceWorkerScriptURLLoader(
      int32_t routing_id,
      int32_t request_id,
      uint32_t options,
      const ResourceRequest& resource_request,
      mojom::URLLoaderClientPtr client,
      scoped_refptr<ServiceWorkerVersion> version,
      scoped_refptr<URLLoaderFactoryGetter> loader_factory_getter,
      const net::MutableNetworkTrafficAnnotationTag& traffic_annotation);
  ~ServiceWorkerScriptURLLoader() override;

  // mojom::URLLoader:
  void FollowRedirect() override;
  void SetPriority(net::RequestPriority priority,
                   int32_t intra_priority_value) override;

  // mojom::URLLoaderClient:
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
  void InitializeScriptCacheWriter();

  // Writes the given headers into the ServiceWorkerScriptCache.
  void WriteHeaders(scoped_refptr<HttpResponseInfoIOBuffer> info_buffer);
  void OnWriteHeadersComplete(net::Error error);

  void StartHandleWatcher();

  // Called repeatedly when the data is ready to be read and passes it into
  // WriteData().
  void OnDataAvailable(MojoResult);

  // Writes the given data into the ServiceWorkerScriptCache.
  void WriteData(scoped_refptr<net::IOBuffer> buffer, size_t available_bytes);
  void OnWriteDataComplete(size_t bytes_written, net::Error error);

  // Called on the completion of all storage writes.
  void OnWriteHeadersAndDataComplete();

  // This is the last method that is called on this class. Notifies the final
  // result to |forwarding_client_| and clears all mojo connections etc.
  void Finalize(const ResourceRequestCompletionStatus& status);

  const GURL request_url_;

  // This is RESOURCE_TYPE_SERVICE_WORKER for the main script or
  // RESOURCE_TYPE_SCRIPT for an imported script.
  const ResourceType resource_type_;

  const int resource_id_;

  bool did_write_headers_ = false;

  scoped_refptr<ServiceWorkerVersion> version_;
  std::unique_ptr<ServiceWorkerCacheWriter> cache_writer_;

  // Used for receiving a fetched script from the network service.
  mojom::URLLoaderPtr network_loader_;
  mojo::Binding<mojom::URLLoaderClient> network_client_binding_;
  mojo::ScopedDataPipeConsumerHandle network_consumer_;
  mojo::SimpleWatcher watcher_;

  // Used for forwarding a fetched script to the controller in a renderer.
  mojom::URLLoaderClientPtr forwarding_client_;
  mojo::ScopedDataPipeProducerHandle forwarding_producer_;

  // Adapter for transferring data from a mojo data pipe to net.
  scoped_refptr<network::MojoToNetPendingBuffer> pending_read_;

  // A barrier closure for synchronizing the completion of the message pipe read
  // with the completion of the data pipe read.
  base::RepeatingClosure completion_closure_;

  base::WeakPtrFactory<ServiceWorkerScriptURLLoader> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(ServiceWorkerScriptURLLoader);
};

}  // namespace content

#endif  // CONTENT_BROWSER_SERVICE_WORKER_SERVICE_WORKER_SCRIPT_URL_LOADER_H_
