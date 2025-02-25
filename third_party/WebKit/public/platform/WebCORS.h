/*
 * Copyright (C) 2008 Apple Inc. All Rights Reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#ifndef WebCORS_h
#define WebCORS_h

#include "base/optional.h"
#include "public/platform/WebHTTPHeaderMap.h"
#include "public/platform/WebHTTPHeaderSet.h"
#include "public/platform/WebString.h"
#include "public/platform/WebURL.h"
#include "public/platform/WebURLRequest.h"
#include "services/network/public/interfaces/cors.mojom-shared.h"
#include "services/network/public/interfaces/fetch_api.mojom-shared.h"

namespace blink {

class WebURLResponse;
class WebSecurityOrigin;
struct ResourceLoaderOptions;

namespace WebCORS {

BLINK_PLATFORM_EXPORT WebURLRequest
CreateAccessControlPreflightRequest(const WebURLRequest&);

// TODO(tyoshino): Using platform/loader/fetch/ResourceLoaderOptions violates
// the DEPS rule. This will be fixed soon by making HandleRedirect() not
// depending on ResourceLoaderOptions.
BLINK_PLATFORM_EXPORT base::Optional<network::mojom::CORSError> HandleRedirect(
    WebSecurityOrigin&,
    WebURLRequest&,
    const WebURL,
    const int redirect_response_status_code,
    const WebHTTPHeaderMap&,
    network::mojom::FetchCredentialsMode,
    ResourceLoaderOptions&);

BLINK_PLATFORM_EXPORT WebHTTPHeaderSet
ExtractCorsExposedHeaderNamesList(network::mojom::FetchCredentialsMode,
                                  const WebURLResponse&);

BLINK_PLATFORM_EXPORT bool IsOnAccessControlResponseHeaderWhitelist(
    const WebString&);

// Checks whether request mode 'no-cors' is allowed for a certain context and
// service-worker mode.
BLINK_PLATFORM_EXPORT bool IsNoCORSAllowedContext(
    WebURLRequest::RequestContext,
    WebURLRequest::ServiceWorkerMode);

// TODO(hintzed): The following three methods delegate to SchemeRegistry and
// FetchUtils respectively to expose them for outofblink-CORS in CORSURLLoader.
// This is a temporary solution with the mid-term goal being to move e.g.
// FetchUtils somewhere where it can be used from /content. The long term goal
// is that CORS will be handled ouf of blink (https://crbug/736308).

BLINK_PLATFORM_EXPORT WebString ListOfCORSEnabledURLSchemes();

BLINK_PLATFORM_EXPORT bool ContainsOnlyCORSSafelistedOrForbiddenHeaders(
    const WebHTTPHeaderMap&);

}  // namespace WebCORS

}  // namespace blink

#endif  // WebCORS_h
