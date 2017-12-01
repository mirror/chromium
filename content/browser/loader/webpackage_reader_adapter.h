// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_LOADER_WEBPACKAGE_READER_ADAPTER_H_
#define CONTENT_BROWSER_LOADER_WEBPACKAGE_READER_ADAPTER_H_

#include "base/callback.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "content/browser/url_loader_factory_getter.h"
#include "content/public/common/url_loader.mojom.h"
#include "mojo/public/cpp/system/data_pipe.h"
#include "mojo/public/cpp/system/simple_watcher.h"
#include "net/base/io_buffer.h"
#include "net/filter/source_stream.h"
#include "services/network/public/cpp/net_adapters.h"
#include "url/gurl.h"
#include "wpkg/webpackage_reader.h"

namespace net {
class SourceStream;
}

namespace content {

struct WebPackageManifest {
  WebPackageManifest();
  ~WebPackageManifest();

  // We use 'origin' as start_url for now.
  GURL start_url;
};

class WebPackageResponseHandler {
 public:
  using ResourceCallback =
      base::OnceCallback<void(int error_code,
                              const ResourceResponseHead& response_head,
                              mojo::ScopedDataPipeConsumerHandle body)>;
  using CompletionCallback =
      base::OnceCallback<void(const network::URLLoaderCompletionStatus&)>;

  WebPackageResponseHandler(const GURL& request_url,
                            ResourceResponseHead response_head,
                            base::OnceClosure done_callback);
  ~WebPackageResponseHandler();

  // May synchronously fire the callbacks.
  void AddCallbacks(ResourceCallback callback,
                    CompletionCallback completion_callback);

  void OnDataReceived(const void* data, size_t size);
  void OnNotifyFinished(int error_code);

  void GetCurrentReadBuffer(const char** buf, size_t* size);
  void UpdateConsumedReadSize(size_t size);

 private:
  void OnPipeWritable(MojoResult result);
  MojoResult BeginWriteToPipe(
      scoped_refptr<network::NetToMojoPendingBuffer>* pending,
      uint32_t* available);
  int WriteToPipe(network::NetToMojoPendingBuffer* dest_buffer,
                  size_t dest_offset,
                  size_t* dest_size_inout,
                  const void* src,
                  size_t* src_size_inout);
  void MaybeCompleteAndNotify();

  GURL request_url_;
  ResourceResponseHead response_head_;

  // For writing the package data to the consumer.
  std::unique_ptr<mojo::SimpleWatcher> writable_handle_watcher_;
  mojo::ScopedDataPipeProducerHandle producer_handle_;
  bool data_write_finished_ = false;

  // For buffering the package data returned by the reader.
  std::deque<scoped_refptr<net::DrainableIOBuffer>> read_buffers_;
  bool data_receive_finished_ = false;
  int error_code_ = net::OK;

  std::unique_ptr<net::SourceStream> source_stream_;

  // Accessed only during srouce_stream_->Read() via
  // GetCurrentReadBuffer() and UpdateConsumedReadSize().
  const char* current_read_pointer_ = nullptr;
  size_t current_read_size_ = 0;
  size_t consumed_read_size_ = 0;

  // We expect single reader for each resource (for now).
  CompletionCallback completion_callback_;
  base::OnceClosure done_callback_;

  DISALLOW_COPY_AND_ASSIGN(WebPackageResponseHandler);
};

// TODO: Remove this indirection.
class WebPackageReaderAdapterClient {
 public:
  virtual ~WebPackageReaderAdapterClient() {}

  // Called at most once.
  virtual void OnFoundManifest(const WebPackageManifest& manifest) = 0;
  virtual void OnFinishedPackage() = 0;
};

class WebPackageReaderAdapter
    : public wpkg::WebPackageReaderClient,
      public base::RefCounted<WebPackageReaderAdapter> {
 public:
  using ResourceCallback = WebPackageResponseHandler::ResourceCallback;
  using CompletionCallback = WebPackageResponseHandler::CompletionCallback;

  WebPackageReaderAdapter(WebPackageReaderAdapterClient* client,
                          mojo::ScopedDataPipeConsumerHandle handle);

  // Asynchronously returns the first resource data, which is expected to
  // be the main resource of the entry page (e.g. index.html).
  // If no resource is found |callback| is fired with non-zero |error_code|
  // (which encodes network error code).
  // If |error_code| is net::OK (which is zero), valid |response_head| and
  // |handle| must be also given.
  // |completion_callback| is additionally called only if |callback| is fired
  // with net::OK, and when streaming the response body is finished.
  void GetFirstResource(ResourceCallback callback,
                        CompletionCallback completion_callback);

  // If no resource is found |callback| is fired with non-zero |error_code|
  // (which encodes network error code).
  // If |error_code| is net::OK (which is zero), valid |response_head| and
  // |handle| must be also given.
  // |completion_callback| is additionally called only if |callback| is fired
  // with net::OK, and when streaming the response body is finished.
  void GetResource(const ResourceRequest& resource_request,
                   ResourceCallback callback,
                   CompletionCallback completion_callback);

  WebPackageReaderAdapterClient* client() { return client_; }

 private:
  const int kInvalidRequestId = -1;

  enum State {
    kInitial,
    kManifestFound,
    kRequestStarted,
    kResponseStarted,
    kFinished,
  };

  struct PendingCallbacks {
    PendingCallbacks(ResourceCallback callback,
                     CompletionCallback completion_callback);
    ~PendingCallbacks();
    PendingCallbacks(PendingCallbacks&& other);
    PendingCallbacks& operator=(PendingCallbacks&& other);

    ResourceCallback callback;
    CompletionCallback completion_callback;
  };

  using RequestURLAndMethod = std::pair<GURL, std::string>;

  friend class base::RefCounted<WebPackageReaderAdapter>;
  ~WebPackageReaderAdapter() override;

  State state() const { return state_; }

  // For reading the data pipe.
  void OnReadable(MojoResult unused);
  void NotifyCompletion(int error_code);

  // wpkg::WebPackageReaderClient:
  void OnOrigin(const std::string& origin) override;
  void OnResourceRequest(const wpkg::HttpHeaders& request_headers,
                         int request_id) override;
  void OnResourceResponse(int request_id,
                          const wpkg::HttpHeaders& response_headers) override;
  void OnDataReceived(int request_id, const void* data, size_t size) override;
  void OnNotifyFinished(int request_id) override;
  void OnEnd() override {}

  void OnResponseHandlerFinished(int request_id);
  void AbortNotFoundPendingRequests();
  void AbortAllPendingRequests(int error_code);

  WebPackageReaderAdapterClient* client_;  // Not owned.

  State state_ = kInitial;

  mojo::ScopedDataPipeConsumerHandle handle_;
  mojo::SimpleWatcher handle_watcher_;
  int main_handle_error_code_ = net::OK;
  bool finished_reading_data_ = false;

  std::map<RequestURLAndMethod, int /* request_id */> request_map_;
  std::map<int /* request_id */, RequestURLAndMethod> reverse_request_map_;
  std::map<int /* request_id */, std::unique_ptr<WebPackageResponseHandler>>
      response_map_;

  std::map<RequestURLAndMethod, PendingCallbacks> pending_request_map_;

  base::Optional<PendingCallbacks> pending_first_request_;
  int first_request_id_ = kInvalidRequestId;

  std::unique_ptr<wpkg::WebPackageReader> reader_;

  base::WeakPtrFactory<WebPackageReaderAdapter> weak_factory_;
  DISALLOW_COPY_AND_ASSIGN(WebPackageReaderAdapter);
};

}  // namespace content

#endif  // CONTENT_BROWSER_LOADER_WEBPACKAGE_READER_ADAPTER_H_
