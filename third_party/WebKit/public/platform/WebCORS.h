// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WebCORS_h
#define WebCORS_h

#include "WebString.h"
#include "WebURL.h"
#include "platform/loader/fetch/ResourceLoaderOptions.h"
#include "platform/wtf/HashSet.h"
#include "platform/wtf/text/AtomicString.h"
#include "platform/wtf/text/StringHash.h"
#include "public/platform/WebSecurityOrigin.h"
#include "public/platform/WebURLRequest.h"
#include "public/platform/WebURLResponse.h"

namespace blink {

namespace WebCORS {

typedef HashSet<String, CaseFoldingHash> HTTPHeaderSet;

// Enumerating the error conditions that the CORS
// access control check can report, including success.
//
// See |checkAccess()| and |accessControlErrorString()| which respectively
// produce and consume these error values, for precise meaning.
enum AccessStatus {
  kAccessAllowed,
  kInvalidResponse,
  kAllowOriginMismatch,
  kSubOriginMismatch,
  kWildcardOriginNotAllowed,
  kMissingAllowOriginHeader,
  kMultipleAllowOriginValues,
  kInvalidAllowOriginValue,
  kDisallowCredentialsNotSetToTrue,
};

// Enumerating the error conditions that CORS preflight
// can report, including success.
//
// See |checkPreflight()| methods and |preflightErrorString()| which
// respectively produce and consume these error values, for precise meaning.
enum PreflightStatus {
  kPreflightSuccess,
  kPreflightInvalidStatus,
  // "Access-Control-Allow-External:"
  // ( https://wicg.github.io/cors-rfc1918/#headers ) specific error
  // conditions:
  kPreflightMissingAllowExternal,
  kPreflightInvalidAllowExternal,
};

// Enumerating the error conditions that CORS redirect target URL
// checks can report, including success.
//
// See |checkRedirectLocation()| methods and |redirectErrorString()| which
// respectively produce and consume these error values, for precise meaning.
enum RedirectStatus {
  kRedirectSuccess,
  kRedirectDisallowedScheme,
  kRedirectContainsCredentials,
};

// Perform a CORS access check on the response. Returns |kAccessAllowed| if
// access is allowed. Use |accessControlErrorString()| to construct a
// user-friendly error message for any of the other (error) conditions.
BLINK_PLATFORM_EXPORT AccessStatus
CheckAccess(const WebURLResponse&,
            WebURLRequest::FetchCredentialsMode,
            const WebSecurityOrigin&);

// Given a redirected-to URL, check if the location is allowed
// according to CORS. That is:
// - the URL has a CORS supported scheme and
// - the URL does not contain the userinfo production.
//
// Returns |kRedirectSuccess| in all other cases. Use
// |redirectErrorString()| to construct a user-friendly error
// message for any of the error conditions.
BLINK_PLATFORM_EXPORT RedirectStatus CheckRedirectLocation(const WebURL&);

// Perform the required CORS checks on the response to a preflight request.
// Returns |kPreflightSuccess| if preflight response was successful.
// Use |preflightErrorString()| to construct a user-friendly error message
// for any of the other (error) conditions.
BLINK_PLATFORM_EXPORT PreflightStatus CheckPreflight(const WebURLResponse&);

// Error checking for the currently experimental
// "Access-Control-Allow-External:" header. Shares error conditions with
// standard preflight checking.
BLINK_PLATFORM_EXPORT PreflightStatus
CheckExternalPreflight(const WebURLResponse&);

BLINK_PLATFORM_EXPORT WebURLRequest
CreateAccessControlPreflightRequest(const WebURLRequest&);

BLINK_PLATFORM_EXPORT bool HandleRedirect(WebSecurityOrigin&,
                                          WebURLRequest&,
                                          const WebURLResponse&,
                                          WebURLRequest::FetchCredentialsMode,
                                          ResourceLoaderOptions&,
                                          WebString&);

// Stringify errors from CORS access checks, preflight or redirect checks.
BLINK_PLATFORM_EXPORT WebString
AccessControlErrorString(const AccessStatus,
                         const WebURLResponse&,
                         const WebSecurityOrigin&,
                         const WebURLRequest::RequestContext);

BLINK_PLATFORM_EXPORT WebString PreflightErrorString(const PreflightStatus,
                                                     const WebURLResponse&);

BLINK_PLATFORM_EXPORT WebString RedirectErrorString(const RedirectStatus,
                                                    const WebURL&);

BLINK_PLATFORM_EXPORT void ParseAccessControlExposeHeadersAllowList(
    const WebString&,
    HTTPHeaderSet&);

BLINK_PLATFORM_EXPORT void ExtractCorsExposedHeaderNamesList(
    const WebURLResponse&,
    HTTPHeaderSet&);

BLINK_PLATFORM_EXPORT bool IsOnAccessControlResponseHeaderWhitelist(
    const WebString&);

}  // namespace WebCORS

}  // namespace blink

#endif  // WebCORS_h
