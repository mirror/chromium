/*
 * Copyright (C) 2011 Google Inc. All Rights Reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "platform/loader/fetch/RawResource.h"

#include <memory>
#include "mojo/public/cpp/system/data_pipe.h"
#include "platform/loader/fetch/FetchParameters.h"
#include "platform/loader/fetch/MemoryCache.h"
#include "platform/loader/fetch/ResourceFetcher.h"
#include "platform/loader/fetch/ResourceLoader.h"
#include "platform/network/http_names.h"
#include "public/platform/Platform.h"
#include "public/platform/WebThread.h"

namespace blink {

inline RawResource* ToRawResource(Resource* resource) {
  SECURITY_DCHECK(!resource || IsRawResource(*resource));
  return static_cast<RawResource*>(resource);
}

RawResource* RawResource::FetchSynchronously(FetchParameters& params,
                                             ResourceFetcher* fetcher) {
  params.MakeSynchronous();
  return ToRawResource(
      fetcher->RequestResource(params, RawResourceFactory(Resource::kRaw)));
}

RawResource* RawResource::FetchImport(FetchParameters& params,
                                      ResourceFetcher* fetcher) {
  DCHECK_EQ(params.GetResourceRequest().GetFrameType(),
            WebURLRequest::kFrameTypeNone);
  params.SetRequestContext(WebURLRequest::kRequestContextImport);
  return ToRawResource(fetcher->RequestResource(
      params, RawResourceFactory(Resource::kImportResource)));
}

RawResource* RawResource::Fetch(FetchParameters& params,
                                ResourceFetcher* fetcher) {
  DCHECK_EQ(params.GetResourceRequest().GetFrameType(),
            WebURLRequest::kFrameTypeNone);
  DCHECK_NE(params.GetResourceRequest().GetRequestContext(),
            WebURLRequest::kRequestContextUnspecified);
  return ToRawResource(
      fetcher->RequestResource(params, RawResourceFactory(Resource::kRaw)));
}

RawResource* RawResource::FetchMainResource(
    FetchParameters& params,
    ResourceFetcher* fetcher,
    const SubstituteData& substitute_data) {
  DCHECK_NE(params.GetResourceRequest().GetFrameType(),
            WebURLRequest::kFrameTypeNone);
  DCHECK(params.GetResourceRequest().GetRequestContext() ==
             WebURLRequest::kRequestContextForm ||
         params.GetResourceRequest().GetRequestContext() ==
             WebURLRequest::kRequestContextFrame ||
         params.GetResourceRequest().GetRequestContext() ==
             WebURLRequest::kRequestContextHyperlink ||
         params.GetResourceRequest().GetRequestContext() ==
             WebURLRequest::kRequestContextIframe ||
         params.GetResourceRequest().GetRequestContext() ==
             WebURLRequest::kRequestContextInternal ||
         params.GetResourceRequest().GetRequestContext() ==
             WebURLRequest::kRequestContextLocation);

  return ToRawResource(fetcher->RequestResource(
      params, RawResourceFactory(Resource::kMainResource), substitute_data));
}

RawResource* RawResource::FetchMedia(FetchParameters& params,
                                     ResourceFetcher* fetcher) {
  DCHECK_EQ(params.GetResourceRequest().GetFrameType(),
            WebURLRequest::kFrameTypeNone);
  DCHECK(params.GetResourceRequest().GetRequestContext() ==
             WebURLRequest::kRequestContextAudio ||
         params.GetResourceRequest().GetRequestContext() ==
             WebURLRequest::kRequestContextVideo);
  return ToRawResource(
      fetcher->RequestResource(params, RawResourceFactory(Resource::kMedia)));
}

RawResource* RawResource::FetchTextTrack(FetchParameters& params,
                                         ResourceFetcher* fetcher) {
  DCHECK_EQ(params.GetResourceRequest().GetFrameType(),
            WebURLRequest::kFrameTypeNone);
  params.SetRequestContext(WebURLRequest::kRequestContextTrack);
  return ToRawResource(fetcher->RequestResource(
      params, RawResourceFactory(Resource::kTextTrack)));
}

RawResource* RawResource::FetchManifest(FetchParameters& params,
                                        ResourceFetcher* fetcher) {
  DCHECK_EQ(params.GetResourceRequest().GetFrameType(),
            WebURLRequest::kFrameTypeNone);
  DCHECK_EQ(params.GetResourceRequest().GetRequestContext(),
            WebURLRequest::kRequestContextManifest);
  return ToRawResource(fetcher->RequestResource(
      params, RawResourceFactory(Resource::kManifest)));
}

RawResource::RawResource(const ResourceRequest& resource_request,
                         Type type,
                         const ResourceLoaderOptions& options)
    : Resource(resource_request, type, options) {}

}  // namespace blink
