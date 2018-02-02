// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MockResource_h
#define MockResource_h

#include "platform/heap/Handle.h"
#include "platform/loader/fetch/Resource.h"

namespace blink {

class FetchParameters;
class ResourceFetcher;
struct ResourceLoaderOptions;
class MockCacheHandler;

// Mocked Resource sub-class for testing. MockResource class can pretend a type
// of Resource sub-class in a simple way. You should not expect anything
// complicated to emulate actual sub-resources, but you may be able to use this
// class to verify classes that consume Resource sub-classes in a simple way.
class MockResource final : public Resource {
 public:
  static MockResource* Fetch(FetchParameters&,
                             ResourceFetcher*,
                             ResourceClient*);
  static MockResource* Create(const ResourceRequest&);
  static MockResource* Create(const KURL&);
  MockResource(const ResourceRequest&, const ResourceLoaderOptions&);

  void CreateCachedMetadataHandler(
      std::unique_ptr<CacheMetadataSender> send_callback) override;
  void ClearCachedMetadataHandler() override;
  void SetSerializedCachedMetadata(const char*, size_t) override;
  void ClearCachedMetadata(CachedMetadataHandler::CacheType) override;

  void SendCachedMetadata(const char*, size_t);

  std::shared_ptr<MockCacheHandler> CacheHandler() const {
    return cache_handler_;
  }

 private:
  std::shared_ptr<MockCacheHandler> cache_handler_;
};

}  // namespace blink

#endif
