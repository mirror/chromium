// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LinkFetchResource_h
#define LinkFetchResource_h

#include <memory>
#include "platform/loader/fetch/Resource.h"
#include "platform/loader/fetch/ResourceClient.h"

namespace blink {

class FetchParameters;
class ResourceFetcher;

class LinkFetchResource final : public Resource {
 public:
  using ClientType = ResourceClient;

  static Resource* Fetch(Resource::Type, FetchParameters&, ResourceFetcher*);
  ~LinkFetchResource() override;

 private:
  class LinkResourceFactory : public ResourceFactory {
   public:
    explicit LinkResourceFactory(Resource::Type type) : ResourceFactory(type) {}

    Resource* Create(const ResourceRequest& request,
                     std::unique_ptr<ResourceLoaderOptions> options,
                     const String& charset) const override {
      return new LinkFetchResource(request, GetType(), std::move(options));
    }
  };
  LinkFetchResource(const ResourceRequest&,
                    Type,
                    std::unique_ptr<ResourceLoaderOptions>);
};

}  // namespace blink

#endif  // LinkFetchResource_h
