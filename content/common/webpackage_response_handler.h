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
  bool connected_to_consumer() const { return has_callback(); }

 private:
  class BufferedStream;
  void OnPipeWritable(MojoResult result);
  void ReadData();

  void OnReadable(int net_err);
  void ContinueRead();

  void CompleteAndNotify();

  const GURL request_url_;
  const std::string request_method_;
  const ResourceResponseHead response_head_;

  // For writing the package data to the consumer.
  std::unique_ptr<mojo::SimpleWatcher> writable_handle_watcher_;
  mojo::ScopedDataPipeProducerHandle producer_handle_;

  std::unique_ptr<net::SourceStream> source_stream_;
  // |source_stream_| owns |buffered_stream_| for zipped data.
  // Otherwise |source_stream_| == |buffered_stream_|.
  BufferedStream* buffered_stream_;

  scoped_refptr<network::NetToMojoPendingBuffer> pending_write_;
  int writing_data_size_ = 0;
  int pending_buffer_size_ = 0;
  bool finished_ = false;

  // We expect single reader for each resource (for now).
  CompletionCallback completion_callback_;
  base::OnceClosure done_callback_;

  base::Callback<void(int)> on_read_callback_;

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
