// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/net/cross_origin_access_control.h"

#include "base/stl_util.h"
#include "base/strings/string_util.h"
#include "net/http/http_util.h"
#include "url/url_util.h"

namespace content {

namespace CrossOriginAccessControl {

namespace {

// TODO(hintzed): Copied from platform/network/HTTPParsers.h
std::string ExtractMIMETypeFromMediaType(const std::string& media_type) {
  unsigned length = media_type.length();

  unsigned pos = 0;

  while (pos < length) {
    char c = media_type[pos];
    if (c != '\t' && c != ' ')
      break;
    ++pos;
  }

  if (pos == length)
    return media_type;

  unsigned type_start = pos;

  unsigned type_end = pos;
  while (pos < length) {
    char c = media_type[pos];

    // While RFC 2616 does not allow it, other browsers allow multiple values in
    // the HTTP media type header field, Content-Type. In such cases, the media
    // type string passed here may contain the multiple values separated by
    // commas. For now, this code ignores text after the first comma, which
    // prevents it from simply failing to parse such types altogether.  Later
    // for better compatibility we could consider using the first or last valid
    // MIME type instead.
    // See https://bugs.webkit.org/show_bug.cgi?id=25352 for more discussion.
    if (c == ',' || c == ';')
      break;

    if (c != '\t' && c != ' ')
      type_end = pos + 1;

    ++pos;
  }

  return std::string(media_type.substr(type_start, type_end - type_start));
}

bool IsCORSSafelistedContentType(const std::string& media_type) {
  std::string mime_type = ExtractMIMETypeFromMediaType(media_type);
  return base::EqualsCaseInsensitiveASCII(
             mime_type, "application/x-www-form-urlencoded") ||
         base::EqualsCaseInsensitiveASCII(mime_type, "multipart/form-data") ||
         base::EqualsCaseInsensitiveASCII(mime_type, "text/plain");
}

/* TODO(hintzed): I think I will need those methods later in the refactoring.
bool IsOnAccessControlResponseHeaderWhitelist(const std::string& name) {
  for (const char* header : kAllowedCrossOriginResponseHeaders) {
    if (base::LowerCaseEqualsASCII(name, header))
      return true;
  }
  return false;
}

void ExtractCorsExposedHeaderNamesList(const ResourceResponseHead& head,
                                       IgnoreCaseStringSet& header_set) {
  // If a response was fetched via a service worker, it will always have
  // corsExposedHeaderNames set, either from the
  // Access-Control-Expose-Headers header, or explicitly via foreign
  // fetch. For requests that didn't come from a service worker, foreign
  // fetch doesn't apply so just parse the CORS header.
  if (head.was_fallback_required_by_service_worker) {
    for (const auto& header : head.cors_exposed_header_names)
      header_set.insert(header);
    return;
  }

  std::string expose_headers;
  head.headers->GetNormalizedHeader(kAccessControlExposeHeaders,
                                    &expose_headers);

  ParseAccessControlExposeHeadersAllowList(expose_headers, header_set);
}
*/
}  // namespace

const char kAccessControlAllowExternal[] = "Access-Control-Allow-External";
const char kAccessControlAllowHeaders[] = "Access-Control-Allow-Headers";
const char kAccessControlAllowMethods[] = "Access-Control-Allow-Methods";
const char kAccessControlAllowOrigin[] = "Access-Control-Allow-Origin";
const char kAccessControlAllowSuborigin[] = "Access-Control-Allow-Suborigin";
const char kAccessControlAllowCredentials[] =
    "Access-Control-Allow-Credentials";
const char kAccessControlExposeHeaders[] = "Access-Control-Expose-Headers";
const char kAccessControlMaxAge[] = "Access-Control-Max-Age";
const char kAccessControlRequestExternal[] = "Access-Control-Request-External";
const char kAccessControlRequestHeaders[] = "Access-Control-Request-Headers";
const char kAccessControlRequestMethod[] = "Access-Control-Request-Method";

const char* kAllowedCrossOriginResponseHeaders[] = {
    "cache-control", "content-language", "content-type",
    "expires",       "last-modified",    "pragma"};

// Fetch API Spec: https://fetch.spec.whatwg.org/#cors-preflight-fetch-0
std::string CreateAccessControlRequestHeadersHeader(
    const net::HttpRequestHeaders& headers) {
  std::vector<std::string> filtered_headers;

  for (net::HttpRequestHeaders::Iterator it(headers); it.GetNext();) {
    // Exclude CORS-safelisted headers.
    if (IsCORSSafelistedHeader(it.name(), it.value()))
      continue;

    if (base::LowerCaseEqualsASCII(it.name(), "referer")) {
      // When the request is from a Worker, referrer header was added by
      // WorkerThreadableLoader. But it should not be added to
      // Access-Control-Request-Headers header.
      continue;
    }
    filtered_headers.push_back(base::ToLowerASCII(it.name()));
  }

  if (filtered_headers.empty())
    return "";

  // Sort header names lexicographically.
  std::sort(filtered_headers.begin(), filtered_headers.end());

  std::string header_buffer;
  for (const std::string& header : filtered_headers)
    header_buffer += (header_buffer.empty() ? header : ',' + header);

  return header_buffer;
}

// https://fetch.spec.whatwg.org/#cors-safelisted-method
bool IsCORSSafelistedMethod(const std::string& method) {
  return method == "GET" || method == "HEAD" || method == "POST";
}

bool ContainsOnlyCORSSafelistedOrForbiddenHeaders(
    const net::HttpRequestHeaders& headers) {
  for (net::HttpRequestHeaders::Iterator it(headers); it.GetNext();) {
    if (!IsCORSSafelistedHeader(it.name(), it.value()) &&
        !IsForbiddenHeaderName(it.name()))
      return false;
  }
  return true;
}

bool IsCORSSafelistedHeader(const std::string& name, const std::string& value) {
  // https://fetch.spec.whatwg.org/#cors-safelisted-request-header
  // "A CORS-safelisted header is a header whose name is either one of `Accept`,
  // `Accept-Language`, and `Content-Language`, or whose name is
  // `Content-Type` and value, once parsed, is one of
  // `application/x-www-form-urlencoded`, `multipart/form-data`, and
  // `text/plain`."
  //
  // Treat 'Save-Data' as a CORS-safelisted header, since it is added by Chrome
  // when Data Saver feature is enabled. Treat inspector headers as a
  // CORS-safelisted headers, since they are added by blink when the inspector
  // is open.
  //
  // Treat 'Intervention' as a CORS-safelisted header, since it is added by
  // Chrome when an intervention is (or may be) applied.

  if (base::EqualsCaseInsensitiveASCII(name, "accept") ||
      base::EqualsCaseInsensitiveASCII(name, "accept-language") ||
      base::EqualsCaseInsensitiveASCII(name, "content-language") ||
      base::EqualsCaseInsensitiveASCII(
          name, "X-DevTools-Emulate-Network-Conditions-Client-Id") ||
      base::EqualsCaseInsensitiveASCII(name, "save-data") ||
      base::EqualsCaseInsensitiveASCII(name, "intervention"))
    return true;

  if (base::EqualsCaseInsensitiveASCII(name, "content-type"))
    return IsCORSSafelistedContentType(value);

  return false;
}

bool IsForbiddenHeaderName(const std::string& name) {
  // http://fetch.spec.whatwg.org/#forbidden-header-name
  // "A forbidden header name is a header names that is one of:
  //   `Accept-Charset`, `Accept-Encoding`, `Access-Control-Request-Headers`,
  //   `Access-Control-Request-Method`, `Connection`,
  //   `Content-Length, Cookie`, `Cookie2`, `Date`, `DNT`, `Expect`, `Host`,
  //   `Keep-Alive`, `Origin`, `Referer`, `TE`, `Trailer`,
  //   `Transfer-Encoding`, `Upgrade`, `User-Agent`, `Via`
  // or starts with `Proxy-` or `Sec-` (including when it is just `Proxy-` or
  // `Sec-`)."

  return !net::HttpUtil::IsSafeHeader(name);
}

bool ShouldTreatCredentialsModeAsInclude(
    FetchCredentialsMode credentials_mode) {
  return credentials_mode ==
             FetchCredentialsMode::FETCH_CREDENTIALS_MODE_INCLUDE ||
         credentials_mode ==
             FetchCredentialsMode::FETCH_CREDENTIALS_MODE_PASSWORD;
}

ResourceRequest CreateAccessControlPreflightRequest(
    const ResourceRequest& request) {
  const GURL& request_url = request.url;

  DCHECK(request_url.username().empty());
  DCHECK(request_url.password().empty());

  ResourceRequest preflight_request;
  preflight_request.method = "OPTIONS";
  preflight_request.url = request.url;
  preflight_request.request_initiator = request.request_initiator;
  preflight_request.resource_type = request.resource_type;
  // TODO(hintzed): Spec says to copy destination?
  preflight_request.origin_pid = request.origin_pid;
  preflight_request.referrer = request.referrer;
  preflight_request.referrer_policy = request.referrer_policy;

  //  preflight_request.priority = request.priority;
  //  preflight_request.fetch_credentials_mode = FETCH_CREDENTIALS_MODE_OMIT;
  //  preflight_request.service_worker_mode = ServiceWorkerMode::NONE;
  //  preflight_request.request_context = request.request_context;

  net::HttpRequestHeaders preflight_headers;
  preflight_headers.SetHeader("Access-Control-Request-Method", request.method);

  if (request.is_external_request)
    preflight_headers.SetHeader("Access-Control-Request-External", "true");

  if (!request.headers.empty()) {
    net::HttpRequestHeaders request_headers;
    request_headers.AddHeadersFromString(request.headers);

    std::string access_control_request_headers =
        CreateAccessControlRequestHeadersHeader(request_headers);

    if (!access_control_request_headers.empty()) {
      preflight_headers.SetHeader(kAccessControlRequestHeaders,
                                  access_control_request_headers);
    }
  }

  preflight_request.headers = preflight_headers.ToString();

  return preflight_request;
}

AccessStatus CheckAccess(const GURL& request_url,
                         const ResourceResponseHead& response,
                         FetchCredentialsMode credentials_mode,
                         const url::Origin& security_origin) {
  int status_code = response.headers->response_code();

  if (!status_code)
    return kInvalidResponse;

  net::HttpResponseHeaders* headers = response.headers.get();

  std::string allow_origin_header_value;
  headers->GetNormalizedHeader(kAccessControlAllowOrigin,
                               &allow_origin_header_value);

  // Check Suborigins, unless the Access-Control-Allow-Origin is '*', which
  // implies that all Suborigins are okay as well.
  if (!security_origin.suborigin().empty() &&
      allow_origin_header_value != "*") {
    std::string allow_suborigin_header_value;
    headers->GetNormalizedHeader(kAccessControlAllowSuborigin,
                                 &allow_suborigin_header_value);

    std::string atomic_suborigin_name = security_origin.suborigin();
    if (allow_suborigin_header_value != "*" &&
        allow_suborigin_header_value != atomic_suborigin_name) {
      return kSubOriginMismatch;
    }
  }

  if (allow_origin_header_value == "*") {
    // A wildcard Access-Control-Allow-Origin can not be used if credentials are
    // to be sent, even with Access-Control-Allow-Credentials set to true.
    if (!ShouldTreatCredentialsModeAsInclude(credentials_mode))
      return kAccessAllowed;
    // TODO(hintzed): Is that a correct substitute for headers->
    // blink::ResourceResponse::IsHTTP()
    if (request_url.SchemeIsHTTPOrHTTPS()) {
      return kWildcardOriginNotAllowed;
    }
  } else if (!security_origin.IsSameOriginWith(
                 url::Origin(GURL(allow_origin_header_value)))) {
    if (allow_origin_header_value.empty())
      return kMissingAllowOriginHeader;

    // Check for origin separator (whitespace or comma):
    if (allow_origin_header_value.find_first_of(" \t\n\v\f\r,") !=
        std::string::npos)
      return kMultipleAllowOriginValues;

    GURL header_origin(allow_origin_header_value);
    if (!header_origin.is_valid())
      return kInvalidAllowOriginValue;

    return kAllowOriginMismatch;
  }

  if (ShouldTreatCredentialsModeAsInclude(credentials_mode)) {
    std::string allow_credentials_header_value;
    headers->GetNormalizedHeader(kAccessControlAllowCredentials,
                                 &allow_credentials_header_value);
    if (allow_credentials_header_value != "true") {
      return kDisallowCredentialsNotSetToTrue;
    }
  }

  return kAccessAllowed;
}

std::string GetHeaderValue(const ResourceResponseHead& head, std::string name) {
  std::string value;
  bool success = head.headers->GetNormalizedHeader(name, &value);

  DCHECK(success);

  return value;
}

std::string AccessControlErrorString(const AccessStatus status,
                                     const ResourceResponseHead& head,
                                     const url::Origin& origin,
                                     const RequestContextType context) {
  int status_code = head.headers->response_code();

  // Predicate that gates what status codes should be included in console error
  // messages for responses containing no access control headers.
  bool isInterestingStatusCode = status_code >= 400;

  std::string originDeniedMsg =
      " Origin '" + origin.Serialize() + "' is therefore not allowed access.";

  std::string xhrCredentialMsg =
      (context == RequestContextType::REQUEST_CONTEXT_TYPE_XML_HTTP_REQUEST
           ? " The credentials mode of requests initiated by the "
             "XMLHttpRequest is controlled by the withCredentials "
             "attribute."
           : "");

  std::string noCORSInformationalMsg =
      (context == RequestContextType::REQUEST_CONTEXT_TYPE_FETCH
           ? " Have the server send the header with a valid value, or, if an "
             "opaque response serves your needs, set the request's mode to "
             "'no-cors' to fetch the resource with CORS disabled."
           : "");

  switch (status) {
    case kInvalidResponse:
      return "Invalid response." + originDeniedMsg;
    case kSubOriginMismatch:
      return "The 'Access-Control-Allow-Suborigin' header has a value '" +
             GetHeaderValue(head, kAccessControlAllowSuborigin) +
             "' that is not equal to the supplied suborigin." + originDeniedMsg;
    case kWildcardOriginNotAllowed:
      return "The value of the 'Access-Control-Allow-Origin' header in the "
             "response must not be the wildcard '*' when the request's "
             "credentials mode is 'include'." +
             originDeniedMsg + xhrCredentialMsg;
    case kMissingAllowOriginHeader:
      return "No 'Access-Control-Allow-Origin' header is present on the "
             "requested resource." +
             originDeniedMsg +
             (isInterestingStatusCode ? " The response had HTTP status code " +
                                            std::to_string(status_code) + "."
                                      : "") +
             (context == RequestContextType::REQUEST_CONTEXT_TYPE_FETCH
                  ? " If an opaque response serves your needs, set the "
                    "request's mode to 'no-cors' to fetch the resource with "
                    "CORS disabled."
                  : "");
    case kMultipleAllowOriginValues:
      return "The 'Access-Control-Allow-Origin' header contains multiple "
             "values '" +
             GetHeaderValue(head, kAccessControlAllowOrigin) +
             "', but only one is allowed." + originDeniedMsg +
             noCORSInformationalMsg;
    case kInvalidAllowOriginValue:
      return "The 'Access-Control-Allow-Origin' header contains the invalid "
             "value '" +
             GetHeaderValue(head, kAccessControlAllowOrigin) + "'." +
             originDeniedMsg + noCORSInformationalMsg;
    case kAllowOriginMismatch:
      return "The 'Access-Control-Allow-Origin' header has a value '" +
             GetHeaderValue(head, kAccessControlAllowOrigin) +
             "' that is not equal to the supplied origin." + originDeniedMsg +
             noCORSInformationalMsg;
    case kDisallowCredentialsNotSetToTrue:
      return "The value of the 'Access-Control-Allow-Credentials' header in "
             "the response is '" +
             GetHeaderValue(head, kAccessControlAllowCredentials) +
             "' which must be 'true' when the request's credentials mode is "
             "'include'." +
             originDeniedMsg + xhrCredentialMsg;
    default:
      NOTREACHED();
      return "";
  }
}

PreflightStatus CheckPreflight(const ResourceResponseHead& head) {
  // CORS preflight with 3XX is considered network error in
  // Fetch API Spec: https://fetch.spec.whatwg.org/#cors-preflight-fetch
  // CORS Spec: http://www.w3.org/TR/cors/#cross-origin-request-with-preflight-0
  // https://crbug.com/452394
  int status = head.headers->response_code();
  if (status < 200 || status >= 300)
    return kPreflightInvalidStatus;

  return kPreflightSuccess;
}

PreflightStatus CheckExternalPreflight(const ResourceResponseHead& head) {
  std::string result;

  if (!head.headers->GetNormalizedHeader(kAccessControlAllowExternal, &result))
    return kPreflightMissingAllowExternal;
  if (!base::EqualsCaseInsensitiveASCII(result, "true")) {
    return kPreflightInvalidAllowExternal;
  }
  return kPreflightSuccess;
}

std::string PreflightErrorString(const PreflightStatus status,
                                 const ResourceResponseHead& head) {
  switch (status) {
    case kPreflightInvalidStatus:
      return "Response for preflight has invalid HTTP status code " +
             std::to_string(head.headers->response_code());
    case kPreflightMissingAllowExternal:
      return "No 'Access-Control-Allow-External' header was present in the "
             "preflight response for this external request (This is an "
             "experimental header which is defined in "
             "'https://wicg.github.io/cors-rfc1918/').";
    case kPreflightInvalidAllowExternal: {
      return "The 'Access-Control-Allow-External' header in the preflight "
             "response for this external request had a value of '" +
             GetHeaderValue(head, kAccessControlAllowExternal) +
             "',  not 'true' (This is an experimental header which is defined "
             "in 'https://wicg.github.io/cors-rfc1918/').";
    }
    default:
      NOTREACHED();
      return "";
  }
}

// A parser for the value of the Access-Control-Expose-Headers header.
class HTTPHeaderNameListParser {
  //  TODO(hintzed): Does something like this exist in content?
  // STACK_ALLOCATED();

 public:
  explicit HTTPHeaderNameListParser(const std::string& value)
      : value_(value), pos_(0) {}

  // Tries parsing |value_| expecting it to be conforming to the #field-name
  // ABNF rule defined in RFC 7230. Returns with the field-name entries stored
  // in |output| when successful. Otherwise, returns with |output| kept empty.
  //
  // |output| must be empty.
  void Parse(IgnoreCaseStringSet& output) {
    DCHECK(output.empty());

    while (true) {
      ConsumeSpaces();

      size_t token_start = pos_;
      ConsumeTokenChars();
      size_t token_size = pos_ - token_start;
      if (token_size == 0) {
        output.clear();
        return;
      }

      output.insert(value_.substr(token_start, token_size));

      ConsumeSpaces();

      if (pos_ == value_.length()) {
        return;
      }

      if (value_[pos_] == ',') {
        ++pos_;
      } else {
        output.clear();
        return;
      }
    }
  }

 private:
  // Consumes zero or more spaces (SP and HTAB) from value_.
  void ConsumeSpaces() {
    while (true) {
      if (pos_ == value_.length()) {
        return;
      }

      char c = value_[pos_];
      if (c != ' ' && c != '\t') {
        return;
      }
      ++pos_;
    }
  }

  // Consumes zero or more tchars from value_.
  void ConsumeTokenChars() {
    while (true) {
      if (pos_ == value_.length()) {
        return;
      }

      char c = value_[pos_];
      if (c > 0x7F || !net::HttpUtil::IsTokenChar(c)) {
        return;
      }
      ++pos_;
    }
  }

  const std::string& value_;
  size_t pos_;
};

void ParseAccessControlExposeHeadersAllowList(const std::string& header_value,
                                              IgnoreCaseStringSet& header_set) {
  HTTPHeaderNameListParser parser(header_value);
  parser.Parse(header_set);
}

RedirectStatus CheckRedirectLocation(const GURL& redirect_url) {
  // Block non HTTP(S) schemes as specified in the step 4 in
  // https://fetch.spec.whatwg.org/#http-redirect-fetch. Chromium also
  // allows the data scheme.
  //
  // TODO(tyoshino): This check should be performed regardless of the CORS
  // flag and request's mode.
  if (!base::ContainsValue(url::GetCORSEnabledSchemes(), redirect_url.scheme()))
    return kRedirectDisallowedScheme;

  // Block URLs including credentials as specified in the step 9 in
  // https://fetch.spec.whatwg.org/#http-redirect-fetch.
  //
  // TODO(tyoshino): This check should be performed also when request's
  // origin is not same origin with the redirect destination's origin.
  if (!(redirect_url.username().empty() && redirect_url.password().empty()))
    return kRedirectContainsCredentials;

  // TODO(hintzed): Should we not also check for Non-secure origins making
  // "external requests" here?

  return kRedirectSuccess;
}

std::string RedirectErrorString(const RedirectStatus status,
                                const GURL& redirect_url) {
  switch (status) {
    case kRedirectDisallowedScheme: {
      return "Redirect location '" + redirect_url.spec() +
             "' has a disallowed scheme for cross-origin requests.";
    }
    case kRedirectContainsCredentials: {
      return "Redirect location '" + redirect_url.spec() +
             "' contains a username and password, which is disallowed for "
             "cross-origin requests.";
    }
    default:
      NOTREACHED();
      return "";
  }
}

/*
bool HandleRedirect(
    RefPtr<SecurityOrigin> current_security_origin,
    ResourceRequest& new_request,
    const ResourceResponse& redirect_response,
    WebURLRequest::FetchCredentialsMode credentials_mode,
    ResourceLoaderOptions& options,
    String& error_message) {
  // http://www.w3.org/TR/cors/#redirect-steps terminology:
  const KURL& last_url = redirect_response.Url();
  const KURL& new_url = new_request.Url();

  RefPtr<SecurityOrigin> new_security_origin = current_security_origin;

  // TODO(tyoshino): This should be fixed to check not only the last one but
  // all redirect responses.
  if (!current_security_origin->CanRequest(last_url)) {
    // Follow http://www.w3.org/TR/cors/#redirect-steps
    RedirectStatus redirect_status =
        CheckRedirectLocation(new_url);
    if (redirect_status != kRedirectSuccess) {
      StringBuilder builder;
      builder.Append("Redirect from '");
      builder.Append(last_url.GetString());
      builder.Append("' has been blocked by CORS policy: ");
      RedirectErrorString(builder, redirect_status,
                                                    new_url);
      error_message = builder.ToString();
      return false;
    }

    // Step 5: perform resource sharing access check.
    AccessStatus cors_status =
        CheckAccess(
            redirect_response, credentials_mode, current_security_origin.Get());
    if (cors_status != kAccessAllowed) {
      StringBuilder builder;
      builder.Append("Redirect from '");
      builder.Append(last_url.GetString());
      builder.Append("' has been blocked by CORS policy: ");
      AccessControlErrorString(
          builder, cors_status, redirect_response,
          current_security_origin.Get(), new_request.GetRequestContext());
      error_message = builder.ToString();
      return false;
    }

    RefPtr<SecurityOrigin> last_origin = SecurityOrigin::Create(last_url);
    // Set request's origin to a globally unique identifier as specified in
    // the step 10 in https://fetch.spec.whatwg.org/#http-redirect-fetch.
    if (!last_origin->CanRequest(new_url)) {
      options.security_origin = SecurityOrigin::CreateUnique();
      new_security_origin = options.security_origin;
    }
  }

  if (!current_security_origin->CanRequest(new_url)) {
    new_request.ClearHTTPOrigin();
    new_request.SetHTTPOrigin(new_security_origin.Get());

    options.cors_flag = true;
  }
  return true;
}

*/

std::string ListOfCORSEnabledURLSchemes() {
  std::string list;
  for (const auto& scheme : url::GetCORSEnabledSchemes()) {
    list += list.empty() ? scheme : ", " + scheme;
  }
  return list;
}

}  // namespace CrossOriginAccessControl
}  // namespace content
