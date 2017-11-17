/*
    Copyright (C) 1998 Lars Knoll (knoll@mpi-hd.mpg.de)
    Copyright (C) 2001 Dirk Mueller <mueller@kde.org>
    Copyright (C) 2006 Samuel Weinig (sam.weinig@gmail.com)
    Copyright (C) 2004, 2005, 2006, 2007 Apple Inc. All rights reserved.

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#ifndef RawResource_h
#define RawResource_h

#include <memory>
#include "platform/PlatformExport.h"
#include "platform/loader/fetch/Resource.h"
#include "platform/loader/fetch/ResourceClient.h"
#include "platform/loader/fetch/ResourceLoaderOptions.h"
#include "platform/wtf/WeakPtr.h"

namespace blink {
class FetchParameters;
class ResourceFetcher;
class SubstituteData;

class PLATFORM_EXPORT RawResource final : public Resource {
 public:
  using ClientType = ResourceClient;

  static RawResource* FetchSynchronously(FetchParameters&, ResourceFetcher*);
  static RawResource* Fetch(FetchParameters&, ResourceFetcher*);
  static RawResource* FetchMainResource(FetchParameters&,
                                        ResourceFetcher*,
                                        const SubstituteData&);
  static RawResource* FetchImport(FetchParameters&, ResourceFetcher*);
  static RawResource* FetchMedia(FetchParameters&, ResourceFetcher*);
  static RawResource* FetchTextTrack(FetchParameters&, ResourceFetcher*);
  static RawResource* FetchManifest(FetchParameters&, ResourceFetcher*);

  // Exposed for testing
  static RawResource* CreateForTest(ResourceRequest request, Type type) {
    ResourceLoaderOptions options;
    return new RawResource(request, type, options);
  }
  static RawResource* CreateForTest(const KURL& url, Type type) {
    ResourceRequest request(url);
    return CreateForTest(request, type);
  }
  static RawResource* CreateForTest(const char* url, Type type) {
    return CreateForTest(KURL(url), type);
  }

  // FIXME: AssociatedURLLoader shouldn't be a DocumentThreadableLoader and
  // therefore shouldn't use RawResource. However, it is, and it needs to be
  // able to defer loading. This can be fixed by splitting CORS preflighting out
  // of DocumentThreadableLoader.
  void SetDefersLoading(bool);

  // Resource implementation
  bool CanReuse(const FetchParameters&) const override;

 private:
  class RawResourceFactory : public NonTextResourceFactory {
   public:
    explicit RawResourceFactory(Resource::Type type)
        : NonTextResourceFactory(type) {}

    Resource* Create(const ResourceRequest& request,
                     const ResourceLoaderOptions& options) const override {
      return new RawResource(request, type_, options);
    }
  };

  RawResource(const ResourceRequest&, Type, const ResourceLoaderOptions&);

  // Resource implementation
  bool ShouldIgnoreHTTPStatusCodeErrors() const override {
    return !IsLinkPreload();
  }
  bool MatchPreload(const FetchParameters&, WebTaskRunner*) override;
};

// TODO(yhirano): Recover #if ENABLE_SECURITY_ASSERT when we stop adding
// RawResources to MemoryCache.
inline bool IsRawResource(const Resource& resource) {
  Resource::Type type = resource.GetType();
  return type == Resource::kMainResource || type == Resource::kRaw ||
         type == Resource::kTextTrack || type == Resource::kMedia ||
         type == Resource::kManifest || type == Resource::kImportResource;
}
inline RawResource* ToRawResource(Resource* resource) {
  SECURITY_DCHECK(!resource || IsRawResource(*resource));
  return static_cast<RawResource*>(resource);
}

}  // namespace blink

#endif  // RawResource_h
