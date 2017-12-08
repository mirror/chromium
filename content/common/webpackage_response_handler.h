// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_COMMON_WEBPACKAGE_RESPONSE_HANDLER_H_
#define CONTENT_COMMON_WEBPACKAGE_RESPONSE_HANDLER_H_

#include "base/callback.h"
#include "content/public/common/resource_response.h"
#include "mojo/public/cpp/system/data_pipe.h"
#include "mojo/public/cpp/system/simple_watcher.h"
#include "services/network/public/cpp/net_adapters.h"
#include "services/network/public/cpp/url_loader_completion_status.h"
#include "url/gurl.h"

namespace net {
class SourceStream;
class DrainableIOBuffer;
}  // namespace net

namespace content {

struct ResourceResponseHead;

using WebPackageResourceCallback =
    base::OnceCallback<void(int error_code,
                            const ResourceResponseHead& response_head,
                            mojo::ScopedDataPipeConsumerHandle body)>;
using WebPackageCompletionCallback =
    base::OnceCallback<void(const network::URLLoaderCompletionStatus&)>;

class WebPackageResponseHandler {
 public:
  using ResourceCallback = WebPackageResourceCallback;
  using CompletionCallback = WebPackageCompletionCallback;

  WebPackageResponseHandler(const GURL& request_url,
                            const std::string& request_method,
                            ResourceResponseHead response_head,
                            base::OnceClosure done_callback,
                            bool support_filters = true);
  ~WebPackageResponseHandler();

  const GURL& request_url() const { return request_url_; }
  const std::string& request_method() const { return request_method_; }

  // May synchronously fire the callbacks.
  void RegisterCallbacks(ResourceCallback callback,
                         CompletionCallback completion_callback);

  void OnDataReceived(const void* data, size_t size);
  void OnNotifyFinished(int error_code);

  bool has_callback() const { return !completion_callback_.is_null(); }

 private:
  class WebPackageSourceStream;

  void PushDataToBuffer(const void* data, size_t size);

  int Read(net::IOBuffer* dest_buffer, int buffer_size);

  void OnPipeWritable(MojoResult result);

  void WriteBufferToPipe();

  int WriteDataToPipe(network::NetToMojoPendingBuffer* dest_buffer,
                      size_t dest_offset,
                      size_t dest_size,
                      const void* src,
                      size_t src_size,
                      size_t* written_size,
                      size_t* consumed_size);
  int DrainDataToPipe(network::NetToMojoPendingBuffer* dest_buffer,
                      size_t dest_offset,
                      size_t dest_size,
                      size_t* written_size,
                      bool* finished);
  void CompleteAndNotify();
  void Abort(int error_code);

  const GURL request_url_;
  const std::string request_method_;
  const ResourceResponseHead response_head_;

  // For writing the package data to the consumer.
  std::unique_ptr<mojo::SimpleWatcher> writable_handle_watcher_;
  mojo::ScopedDataPipeProducerHandle producer_handle_;

  // For buffering the package data returned by the reader.
  std::deque<scoped_refptr<net::DrainableIOBuffer>> read_buffers_;

  int error_code_ = net::OK;

  std::unique_ptr<net::SourceStream> source_stream_;

  bool source_finished_ = false;
  bool stream_need_to_be_drained_ = false;

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

struct WebPackagePendingCallbacks {
  using ResourceCallback = WebPackageResourceCallback;
  using CompletionCallback = WebPackageCompletionCallback;

  WebPackagePendingCallbacks(ResourceCallback callback,
                             CompletionCallback completion_callback);
  ~WebPackagePendingCallbacks();
  WebPackagePendingCallbacks(WebPackagePendingCallbacks&& other);
  WebPackagePendingCallbacks& operator=(WebPackagePendingCallbacks&& other);

  ResourceCallback callback;
  CompletionCallback completion_callback;
};

}  // namespace content

#endif  // CONTENT_COMMON_WEBPACKAGE_RESPONSE_HANDLER_H_
