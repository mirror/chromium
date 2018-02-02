// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform/loader/testing/MockResource.h"

#include "platform/loader/fetch/FetchParameters.h"
#include "platform/loader/fetch/ResourceFetcher.h"
#include "platform/loader/fetch/ResourceLoaderOptions.h"

namespace blink {

namespace {

class MockResourceFactory final : public NonTextResourceFactory {
 public:
  MockResourceFactory() : NonTextResourceFactory(Resource::kMock) {}

  Resource* Create(const ResourceRequest& request,
                   const ResourceLoaderOptions& options) const override {
    return new MockResource(request, options);
  }
};

}  // namespace

// static
MockResource* MockResource::Fetch(FetchParameters& params,
                                  ResourceFetcher* fetcher,
                                  ResourceClient* client) {
  params.SetRequestContext(WebURLRequest::kRequestContextSubresource);
  return static_cast<MockResource*>(
      fetcher->RequestResource(params, MockResourceFactory(), client));
}

// static
MockResource* MockResource::Create(const ResourceRequest& request) {
  ResourceLoaderOptions options;
  return new MockResource(request, options);
}

MockResource* MockResource::Create(const KURL& url) {
  ResourceRequest request(url);
  return Create(request);
}

MockResource::MockResource(const ResourceRequest& request,
                           const ResourceLoaderOptions& options)
    : Resource(request, Resource::kMock, options) {}

class MockCacheHandler {
 public:
  MockCacheHandler(std::unique_ptr<CacheMetadataSender> send_callback)
      : send_callback_(std::move(send_callback)) {}

  void Set(const char* data,
           size_t size,
           CachedMetadataHandler::CacheType cache_type) {
    data_.emplace();
    data_->Append(data, size);
    Send(cache_type);
  }

  void Clear(CachedMetadataHandler::CacheType cache_type) {
    Send(cache_type);
    data_.reset();
  }

 private:
  void Send(CachedMetadataHandler::CacheType cache_type) {
    if (cache_type == CachedMetadataHandler::kSendToPlatform) {
      if (data_) {
        send_callback_->Send(data_->data(), data_->size());
      } else {
        send_callback_->Send(nullptr, 0);
      }
    }
    data_.reset();
  }

  std::unique_ptr<CacheMetadataSender> send_callback_;
  base::Optional<Vector<char>> data_;
};

void MockResource::CreateCachedMetadataHandler(
    std::unique_ptr<CacheMetadataSender> send_callback) {
  cache_handler_ = std::make_shared<MockCacheHandler>(std::move(send_callback));
}
void MockResource::ClearCachedMetadataHandler() {
  cache_handler_.reset();
}
void MockResource::SetSerializedCachedMetadata(const char* data, size_t size) {
  Resource::SetSerializedCachedMetadata(data, size);
  cache_handler_->Set(data, size, CachedMetadataHandler::kCacheLocally);
}
void MockResource::ClearCachedMetadata(
    CachedMetadataHandler::CacheType cache_type) {
  cache_handler_->Clear(cache_type);
}

void MockResource::SendCachedMetadata(const char* data, size_t size) {
  cache_handler_->Set(data, size, CachedMetadataHandler::kSendToPlatform);
}

}  // namespace blink
