// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/network/public/cpp/cors/cors.h"

#include "url/gurl.h"
#include "url/origin.h"

namespace network {

namespace cors {

base::Optional<mojom::CORSError> CheckAccess(
    const GURL& response_url,
    const int response_status_code,
    const base::Optional<std::string>& allow_origin_header,
    const base::Optional<std::string>& allow_suborigin_header,
    const base::Optional<std::string>& allow_credentials_header,
    mojom::FetchCredentialsMode credentials_mode,
    const url::Origin& origin) {
  if (!response_status_code)
    return mojom::CORSError::kInvalidResponse;

  // Check Suborigins, unless the Access-Control-Allow-Origin is '*', which
  // implies that all Suborigins are okay as well.
  bool allow_all_origins = allow_origin_header == "*";
  if (!origin.suborigin().empty() && !allow_all_origins) {
    if (allow_suborigin_header != "*" &&
        allow_suborigin_header != origin.suborigin()) {
      return mojom::CORSError::kSubOriginMismatch;
    }
  }

  if (allow_all_origins) {
    // A wildcard Access-Control-Allow-Origin can not be used if credentials are
    // to be sent, even with Access-Control-Allow-Credentials set to true.
    if (credentials_mode != mojom::FetchCredentialsMode::kInclude)
      return base::nullopt;
    if (response_url.SchemeIsHTTPOrHTTPS() || response_url.SchemeIsSuborigin())
      return mojom::CORSError::kWildcardOriginNotAllowed;
  } else if (!allow_origin_header) {
    return mojom::CORSError::kMissingAllowOriginHeader;
  } else if (allow_origin_header != origin.GetPhysicalOrigin().Serialize()) {
    // Does not allow to have multiple origins in the allow origin header.
    // See https://www.w3.org/TR/cors/#http-access-control-allow-origin and
    // https://fetch.spec.whatwg.org/#http-access-control-allow-origin.
    if (allow_origin_header->find_first_of(" ,") != std::string::npos)
      return mojom::CORSError::kMultipleAllowOriginValues;
    GURL header_origin(*allow_origin_header);
    if (!header_origin.is_valid())
      return mojom::CORSError::kInvalidAllowOriginValue;
    return mojom::CORSError::kAllowOriginMismatch;
  }

  if (credentials_mode == mojom::FetchCredentialsMode::kInclude) {
    // https://fetch.spec.whatwg.org/#http-access-control-allow-credentials.
    if (allow_credentials_header != "true")
      return mojom::CORSError::kDisallowCredentialsNotSetToTrue;
  }
  return base::nullopt;
}

}  // namespace cors

}  // namespace network
