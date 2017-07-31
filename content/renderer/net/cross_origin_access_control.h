// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDERER_NET_CROSS_ORIGIN_ACCESS_CONTROL_H_
#define CONTENT_RENDERER_NET_CROSS_ORIGIN_ACCESS_CONTROL_H_

#include "content/common/service_worker/service_worker_types.h"
#include "content/public/common/resource_request.h"
#include "content/public/common/resource_response.h"
#include "content/renderer/net/str_util.h"

namespace content {

namespace CrossOriginAccessControl {

extern const char kAccessControlAllowExternal[];
extern const char kAccessControlAllowHeaders[];
extern const char kAccessControlAllowMethods[];
extern const char kAccessControlAllowOrigin[];
extern const char kAccessControlAllowSuborigin[];
extern const char kAccessControlAllowCredentials[];
extern const char kAccessControlExposeHeaders[];
extern const char kAccessControlMaxAge[];
extern const char kAccessControlRequestExternal[];
extern const char kAccessControlRequestHeaders[];
extern const char kAccessControlRequestMethod[];

extern const char* kAllowedCrossOriginResponseHeaders[];

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
CONTENT_EXPORT AccessStatus CheckAccess(const GURL& url,
                                        const ResourceResponseHead& response,
                                        FetchCredentialsMode credentials_mode,
                                        const url::Origin& origin);

// Perform the required CORS checks on the response to a preflight request.
// Returns |kPreflightSuccess| if preflight response was successful.
// Use |preflightErrorString()| to construct a user-friendly error message
// for any of the other (error) conditions.
CONTENT_EXPORT PreflightStatus CheckPreflight(const ResourceResponseHead& head);

// Error checking for the currently experimental
// "Access-Control-Allow-External:" header. Shares error conditions with
// standard preflight checking.
CONTENT_EXPORT PreflightStatus
CheckExternalPreflight(const ResourceResponseHead& head);

// https://fetch.spec.whatwg.org/#cors-safelisted-method
// "A CORS-safelisted method is a method that is `GET`, `HEAD`, or `POST`."
CONTENT_EXPORT bool IsCORSSafelistedMethod(const std::string& method);

CONTENT_EXPORT bool ContainsOnlyCORSSafelistedOrForbiddenHeaders(
    const net::HttpRequestHeaders& request_headers);

// Given a redirected-to URL, check if the location is allowed
// according to CORS. That is:
// - the URL has a CORS supported scheme and
// - the URL does not contain the userinfo production.
//
// Returns |kRedirectSuccess| in all other cases. Use
// |redirectErrorString()| to construct a user-friendly error
// message for any of the error conditions.
CONTENT_EXPORT RedirectStatus CheckRedirectLocation(const GURL& url);

/*
 bool HandleRedirect(RefPtr<SecurityOrigin>,
                           ResourceRequest&,
                           const ResourceResponse&,
                           WebURLRequest::FetchCredentialsMode,
                           ResourceLoaderOptions&,
                           String&);
                         */

// Stringify errors from CORS access checks, preflight or redirect checks.
CONTENT_EXPORT std::string AccessControlErrorString(
    const AccessStatus status,
    const ResourceResponseHead& head,
    const url::Origin& origin,
    const RequestContextType context);
CONTENT_EXPORT std::string PreflightErrorString(
    const PreflightStatus status,
    const ResourceResponseHead& head);

CONTENT_EXPORT std::string RedirectErrorString(const RedirectStatus status,
                                               const GURL& redirect_url);

// Used by e.g. the CORS check algorithm to check if the FetchCredentialsMode
// should be treated as equivalent to "include" in the Fetch spec.
CONTENT_EXPORT bool ShouldTreatCredentialsModeAsInclude(
    FetchCredentialsMode credentials_mode);

CONTENT_EXPORT bool IsForbiddenHeaderName(const std::string& name);

CONTENT_EXPORT bool IsCORSSafelistedHeader(const std::string& name,
                                           const std::string& value);

// Serialize the registered schemes in a comma-separated list.
CONTENT_EXPORT std::string ListOfCORSEnabledURLSchemes();

CONTENT_EXPORT ResourceRequest
CreateAccessControlPreflightRequest(const ResourceRequest&);

CONTENT_EXPORT void ParseAccessControlExposeHeadersAllowList(
    const std::string& header_value,
    IgnoreCaseStringSet& headers);

}  // namespace CrossOriginAccessControl
}  // namespace content

#endif  // CONTENT_RENDERER_NET_CROSS_ORIGIN_ACCESS_CONTROL_H_
