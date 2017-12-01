// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_LOADER_WEBPACKAGE_READER_ADAPTER_H_
#define CONTENT_BROWSER_LOADER_WEBPACKAGE_READER_ADAPTER_H_

#include "base/callback.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "content/browser/url_loader_factory_getter.h"
#include "content/common/webpackage_response_handler.h"
#include "content/public/common/url_loader.mojom.h"
#include "mojo/public/cpp/system/data_pipe.h"
#include "mojo/public/cpp/system/simple_watcher.h"
#include "url/gurl.h"
#include "wpkg/webpackage_reader.h"

namespace content {

struct WebPackageManifest {
  WebPackageManifest();
  ~WebPackageManifest();

  // We use 'origin' as start_url for now.
  GURL start_url;
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
  using ResourceCallback = WebPackageResourceCallback;
  using CompletionCallback = WebPackageCompletionCallback;

  class PushObserver {
   public:
    virtual ~PushObserver() {}
    virtual void OnRequest(const GURL& request_url,
                           const std::string& method) = 0;
    virtual void OnResponse(const GURL& request_url,
                            const std::string& method,
                            const ResourceResponseHead& response_head,
                            mojo::ScopedDataPipeConsumerHandle body,
                            CompletionCallback* callback) = 0;
  };

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
  //
  // It is not valid to call this once StartPushResources() is called.
  void GetFirstResource(ResourceCallback callback,
                        CompletionCallback completion_callback);

  // If no resource is found |callback| is fired with non-zero |error_code|
  // (which encodes network error code).
  // If |error_code| is net::OK (which is zero), valid |response_head| and
  // |handle| must be also given.
  // |completion_callback| is additionally called only if |callback| is fired
  // with net::OK, and when streaming the response body is finished.
  //
  // It is not valid to call this once StartPushResources() is called.
  void GetResource(const ResourceRequest& resource_request,
                   ResourceCallback callback,
                   CompletionCallback completion_callback);

  // This attaches a push observer and also enables resource pushing.
  // After calling this, all responses that do not have corresponding
  // GetResource() calls will start to be pushed via the Observer
  // interface.
  // Only a single instance can observe pushes, it is not valid to call
  // this multiple times.
  //
  // This can also only be called when there're no more pending GetResource
  // requests.
  void StartPushResources(base::WeakPtr<PushObserver> observer);

 private:
  const int kInvalidRequestId = -1;

  enum FetchMode {
    kPull,
    kPush,
  };

  enum State {
    kInitial,
    kManifestFound,
    kRequestStarted,
    kResponseStarted,
    kFinished,
  };

  using PendingCallbacks = WebPackagePendingCallbacks;
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

  FetchMode fetch_mode_ = kPull;
  base::WeakPtr<PushObserver> push_observer_;

  base::WeakPtrFactory<WebPackageReaderAdapter> weak_factory_;
  DISALLOW_COPY_AND_ASSIGN(WebPackageReaderAdapter);
};

}  // namespace content

#endif  // CONTENT_BROWSER_LOADER_WEBPACKAGE_READER_ADAPTER_H_
