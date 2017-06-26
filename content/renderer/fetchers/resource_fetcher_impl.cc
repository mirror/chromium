// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/fetchers/resource_fetcher_impl.h"

#include <stdint.h>

#include "base/logging.h"
#include "base/macros.h"
#include "base/strings/string_util.h"
#include "base/time/time.h"
#include "third_party/WebKit/public/platform/WebSecurityOrigin.h"
#include "third_party/WebKit/public/platform/WebString.h"
#include "third_party/WebKit/public/platform/WebURL.h"
#include "third_party/WebKit/public/platform/WebURLError.h"
#include "third_party/WebKit/public/platform/WebURLLoader.h"
#include "third_party/WebKit/public/platform/WebURLLoaderClient.h"
#include "third_party/WebKit/public/platform/WebURLRequest.h"
#include "third_party/WebKit/public/platform/WebURLResponse.h"
#include "third_party/WebKit/public/web/WebDocument.h"
#include "third_party/WebKit/public/web/WebKit.h"
#include "third_party/WebKit/public/web/WebLocalFrame.h"
#include "third_party/WebKit/public/web/WebSecurityPolicy.h"

namespace content {

// static
ResourceFetcher* ResourceFetcher::Create() {
  return new ResourceFetcherImpl();
}

class ResourceFetcherImpl::ClientImpl : public blink::WebURLLoaderClient {
 public:
  ClientImpl(ResourceFetcherImpl* parent, const Callback& callback)
      : parent_(parent),
        completed_(false),
        status_(LOADING),
        callback_(callback) {}

  ~ClientImpl() override {}

  virtual void Cancel() { OnLoadCompleteInternal(LOAD_FAILED); }

  bool completed() const { return completed_; }

 private:
  enum LoadStatus {
    LOADING,
    LOAD_FAILED,
    LOAD_SUCCEEDED,
  };

  void OnLoadCompleteInternal(LoadStatus status) {
    DCHECK(!completed_);
    DCHECK_EQ(status_, LOADING);

    completed_ = true;
    status_ = status;

    parent_->OnLoadComplete();

    if (callback_.is_null())
      return;

    // Take a reference to the callback as running the callback may lead to our
    // destruction.
    Callback callback = callback_;
    callback.Run(status_ == LOAD_FAILED ? blink::WebURLResponse() : response_,
                 status_ == LOAD_FAILED ? std::string() : data_);
  }

  // WebURLLoaderClient methods:
  void DidReceiveResponse(const blink::WebURLResponse& response) override {
    DCHECK(!completed_);

    response_ = response;
  }
  void DidReceiveCachedMetadata(const char* data, int data_length) override {
    DCHECK(!completed_);
    DCHECK_GT(data_length, 0);
  }
  void DidReceiveData(const char* data, int data_length) override {
    DCHECK(!completed_);
    DCHECK_GT(data_length, 0);

    data_.append(data, data_length);
  }
  void DidFinishLoading(double finishTime,
                        int64_t total_encoded_data_length,
                        int64_t total_encoded_body_length,
                        int64_t total_decoded_body_length) override {
    DCHECK(!completed_);

    OnLoadCompleteInternal(LOAD_SUCCEEDED);
  }
  void DidFail(const blink::WebURLError& error,
               int64_t total_encoded_data_length,
               int64_t total_encoded_body_length,
               int64_t total_decoded_body_length) override {
    OnLoadCompleteInternal(LOAD_FAILED);
  }

 private:
  ResourceFetcherImpl* parent_;

  // Set to true once the request is complete.
  bool completed_;

  // Buffer to hold the content from the server.
  std::string data_;

  // A copy of the original resource response.
  blink::WebURLResponse response_;

  LoadStatus status_;

  // Callback when we're done.
  Callback callback_;

  DISALLOW_COPY_AND_ASSIGN(ClientImpl);
};

ResourceFetcherImpl::ResourceFetcherImpl() {}

ResourceFetcherImpl::~ResourceFetcherImpl() {
  if (!loader_)
    return;

  DCHECK(client_);

  if (!client_->completed())
    loader_->Cancel();
}

void ResourceFetcherImpl::Start(blink::WebLocalFrame* frame,
                                std::unique_ptr<blink::WebURLRequest> request,
                                const Callback& callback) {
  DCHECK(!loader_);
  DCHECK(!client_);

  DCHECK(frame);
  DCHECK(!frame->GetDocument().IsNull());
  if (!request->HttpBody().IsNull())
    DCHECK_NE("GET", request->HttpMethod().Utf8()) << "GETs can't have bodies.";

  request->SetFirstPartyForCookies(frame->GetDocument().FirstPartyForCookies());
  request->AddHTTPOriginIfNeeded(blink::WebSecurityOrigin::CreateUnique());

  client_.reset(new ClientImpl(this, callback));

  loader_ = frame->CreateURLLoader();
  loader_->LoadAsynchronously(*request, client_.get());
}

void ResourceFetcherImpl::SetTimeout(const base::TimeDelta& timeout) {
  DCHECK(loader_);
  DCHECK(client_);
  DCHECK(!client_->completed());

  timeout_timer_.Start(FROM_HERE, timeout, this, &ResourceFetcherImpl::Cancel);
}

void ResourceFetcherImpl::OnLoadComplete() {
  timeout_timer_.Stop();
}

void ResourceFetcherImpl::Cancel() {
  loader_->Cancel();
  client_->Cancel();
}

}  // namespace content
