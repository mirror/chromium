// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform/origin_manifest/OriginManifestStoreClient.h"

#include "platform/loader/fetch/ResourceRequest.h"
#include "platform/loader/fetch/ResourceResponse.h"
#include "platform/weborigin/SecurityOrigin.h"
#include "public/platform/InterfaceProvider.h"
#include "public/platform/Platform.h"

namespace blink {

OriginManifestStoreClient::OriginManifestStoreClient()
    : cors_preflight_(nullptr) {
  Platform::Current()->GetInterfaceProvider()->GetInterface(
      mojo::MakeRequest(&store_));
}

OriginManifestStoreClient::~OriginManifestStoreClient() {}

bool OriginManifestStoreClient::DefinesCORSPreflight(SecurityOrigin* origin) {
  if (store_->GetCORSPreflight(origin->ToString().Ascii().data(),
                               &cors_preflight_)) {
    return !cors_preflight_.Equals(nullptr);
  }
  return false;
}

void OriginManifestStoreClient::SetCORSFetchModes(ResourceRequest& request) {
  DCHECK(!cors_preflight_.Equals(nullptr));

  RefPtr<SecurityOrigin> request_origin = SecurityOrigin::Create(request.Url());

  // TODO(dhausknecht) The decision making below should be revised regarding
  // which decision can override which one. We should probably give
  // no-credentials priority over with-credentials(?)
  if (!cors_preflight_->nocredentials.Equals(nullptr)) {
    WTF::Vector<WTF::String> origins = cors_preflight_->nocredentials->origins;
    for (WTF::Vector<WTF::String>::iterator it = origins.begin();
         it != origins.end(); ++it) {
      if (*it == "*") {
        request.SetFetchCredentialsMode(
            WebURLRequest::kFetchCredentialsModeSameOrigin);
        break;
      }
      RefPtr<SecurityOrigin> listed_origin =
          SecurityOrigin::CreateFromString(*it);
      if (request_origin->IsSameSchemeHostPort(listed_origin.Get())) {
        request.SetFetchCredentialsMode(
            WebURLRequest::kFetchCredentialsModeSameOrigin);
        break;
      }
    }
  }

  if (!cors_preflight_->withcredentials.Equals(nullptr)) {
    WTF::Vector<WTF::String> origins =
        cors_preflight_->withcredentials->origins;
    for (WTF::Vector<WTF::String>::iterator it = origins.begin();
         it != origins.end(); ++it) {
      if (*it == "*") {
        request.SetFetchCredentialsMode(
            WebURLRequest::kFetchCredentialsModeInclude);
        break;
      }
      RefPtr<SecurityOrigin> listed_origin =
          SecurityOrigin::CreateFromString(*it);
      if (request_origin->IsSameSchemeHostPort(listed_origin.Get())) {
        request.SetFetchCredentialsMode(
            WebURLRequest::kFetchCredentialsModeInclude);
        break;
      }
    }
  }
}

}  // namespace blink
