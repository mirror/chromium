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

#include "public/platform/WebCORS.h"

#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "net/http/http_util.h"
#include "platform/loader/cors/CORS.h"
#include "platform/loader/fetch/FetchUtils.h"
#include "platform/loader/fetch/ResourceLoaderOptions.h"
#include "platform/loader/fetch/ResourceRequest.h"
#include "platform/loader/fetch/ResourceResponse.h"
#include "platform/network/http_names.h"
#include "platform/weborigin/KURL.h"
#include "platform/weborigin/SchemeRegistry.h"
#include "platform/wtf/text/StringBuilder.h"
#include "platform/wtf/text/WTFString.h"
#include "public/platform/WebSecurityOrigin.h"
#include "public/platform/WebURLResponse.h"
#include "url/gurl.h"
#include "url/url_util.h"

using network::mojom::CORSError;

namespace blink {

namespace WebCORS {

namespace {

// Fetch API Spec: https://fetch.spec.whatwg.org/#cors-preflight-fetch-0
String CreateAccessControlRequestHeadersHeader(
    const WebHTTPHeaderMap& headers) {
  Vector<String> filtered_headers;
  for (const auto& header : headers.GetHTTPHeaderMap()) {
    if (CORS::IsCORSSafelistedHeader(header.key, header.value)) {
      // Exclude CORS-safelisted headers.
      continue;
    }
    // TODO(hintzed) replace with EqualIgnoringASCIICase()
    if (DeprecatedEqualIgnoringCase(header.key, "referer")) {
      // When the request is from a Worker, referrer header was added by
      // WorkerThreadableLoader. But it should not be added to
      // Access-Control-Request-Headers header.
      continue;
    }
    filtered_headers.push_back(header.key.DeprecatedLower());
  }
  if (!filtered_headers.size())
    return g_null_atom;

  // Sort header names lexicographically.
  std::sort(filtered_headers.begin(), filtered_headers.end(),
            WTF::CodePointCompareLessThan);
  StringBuilder header_buffer;
  for (const String& header : filtered_headers) {
    if (!header_buffer.IsEmpty())
      header_buffer.Append(",");
    header_buffer.Append(header);
  }

  return header_buffer.ToString();
}

// A parser for the value of the Access-Control-Expose-Headers header.
class HTTPHeaderNameListParser {
  STACK_ALLOCATED();

 public:
  explicit HTTPHeaderNameListParser(const String& value)
      : value_(value), pos_(0) {}

  // Tries parsing |value_| expecting it to be conforming to the #field-name
  // ABNF rule defined in RFC 7230. Returns with the field-name entries stored
  // in |output| when successful. Otherwise, returns with |output| kept empty.
  //
  // |output| must be empty.
  void Parse(WebHTTPHeaderSet& output) {
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

      const CString& name = value_.Substring(token_start, token_size).Ascii();
      output.emplace(name.data(), name.length());

      ConsumeSpaces();

      if (pos_ == value_.length())
        return;

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
      if (pos_ == value_.length())
        return;

      UChar c = value_[pos_];
      if (c != ' ' && c != '\t')
        return;
      ++pos_;
    }
  }

  // Consumes zero or more tchars from value_.
  void ConsumeTokenChars() {
    while (true) {
      if (pos_ == value_.length())
        return;

      UChar c = value_[pos_];
      if (c > 0x7F || !net::HttpUtil::IsTokenChar(c))
        return;
      ++pos_;
    }
  }

  const String value_;
  size_t pos_;
};

}  // namespace

base::Optional<CORSError> HandleRedirect(
    WebSecurityOrigin& current_security_origin,
    WebURLRequest& new_request,
    const WebURL redirect_response_url,
    const int redirect_response_status_code,
    const WebHTTPHeaderMap& redirect_response_header,
    network::mojom::FetchCredentialsMode credentials_mode,
    ResourceLoaderOptions& options) {
  const KURL& last_url = redirect_response_url;
  const KURL& new_url = new_request.Url();

  WebSecurityOrigin& new_security_origin = current_security_origin;

  // TODO(tyoshino): This should be fixed to check not only the last one but
  // all redirect responses.
  if (!current_security_origin.CanRequest(last_url)) {
    base::Optional<CORSError> redirect_error =
        CORS::CheckRedirectLocation(new_url);
    if (redirect_error)
      return redirect_error;

    KURL redirect_response_kurl = redirect_response_url;
    base::Optional<CORSError> access_error =
        CORS::CheckAccess(redirect_response_kurl, redirect_response_status_code,
                          redirect_response_header.GetHTTPHeaderMap(),
                          credentials_mode, *current_security_origin.Get());
    if (access_error)
      return access_error;

    scoped_refptr<const SecurityOrigin> last_origin =
        SecurityOrigin::Create(last_url);
    // Set request's origin to a globally unique identifier as specified in
    // the step 10 in https://fetch.spec.whatwg.org/#http-redirect-fetch.
    if (!last_origin->CanRequest(new_url)) {
      options.security_origin = SecurityOrigin::CreateUnique();
      new_security_origin = options.security_origin;
    }
  }

  if (!current_security_origin.CanRequest(new_url)) {
    new_request.ClearHTTPHeaderField(WebString(HTTPNames::Suborigin));
    new_request.SetHTTPHeaderField(WebString(HTTPNames::Origin),
                                   new_security_origin.ToString());
    if (!new_security_origin.Suborigin().IsEmpty()) {
      new_request.SetHTTPHeaderField(WebString(HTTPNames::Suborigin),
                                     new_security_origin.Suborigin());
    }

    options.cors_flag = true;
  }
  return base::nullopt;
}

WebURLRequest CreateAccessControlPreflightRequest(
    const WebURLRequest& request) {
  const KURL& request_url = request.Url();

  DCHECK(request_url.User().IsEmpty());
  DCHECK(request_url.Pass().IsEmpty());

  WebURLRequest preflight_request(request_url);
  preflight_request.SetHTTPMethod(HTTPNames::OPTIONS);
  preflight_request.SetHTTPHeaderField(HTTPNames::Access_Control_Request_Method,
                                       request.HttpMethod());
  preflight_request.SetPriority(request.GetPriority());
  preflight_request.SetRequestContext(request.GetRequestContext());
  preflight_request.SetFetchCredentialsMode(
      network::mojom::FetchCredentialsMode::kOmit);
  preflight_request.SetServiceWorkerMode(
      WebURLRequest::ServiceWorkerMode::kNone);
  preflight_request.SetHTTPReferrer(request.HttpHeaderField(HTTPNames::Referer),
                                    request.GetReferrerPolicy());

  if (request.IsExternalRequest()) {
    preflight_request.SetHTTPHeaderField(
        HTTPNames::Access_Control_Request_External, "true");
  }

  String request_headers = CreateAccessControlRequestHeadersHeader(
      request.ToResourceRequest().HttpHeaderFields());
  if (request_headers != g_null_atom) {
    preflight_request.SetHTTPHeaderField(
        HTTPNames::Access_Control_Request_Headers, request_headers);
  }

  return preflight_request;
}

WebHTTPHeaderSet ExtractCorsExposedHeaderNamesList(
    network::mojom::FetchCredentialsMode credentials_mode,
    const WebURLResponse& response) {
  // If a response was fetched via a service worker, it will always have
  // CorsExposedHeaderNames set from the Access-Control-Expose-Headers header.
  // For requests that didn't come from a service worker, just parse the CORS
  // header.
  if (response.WasFetchedViaServiceWorker()) {
    WebHTTPHeaderSet header_set;
    for (const auto& header : response.CorsExposedHeaderNames())
      header_set.emplace(header.Ascii().data(), header.Ascii().length());
    return header_set;
  }

  WebHTTPHeaderSet header_set;
  HTTPHeaderNameListParser parser(response.HttpHeaderField(
      WebString(HTTPNames::Access_Control_Expose_Headers)));
  parser.Parse(header_set);

  if (credentials_mode != network::mojom::FetchCredentialsMode::kInclude &&
      header_set.find("*") != header_set.end()) {
    header_set.clear();
    for (const auto& header :
         response.ToResourceResponse().HttpHeaderFields()) {
      CString name = header.key.Ascii();
      header_set.emplace(name.data(), name.length());
    }
  }
  return header_set;
}

bool IsOnAccessControlResponseHeaderWhitelist(const WebString& name) {
  DEFINE_THREAD_SAFE_STATIC_LOCAL(
      WebHTTPHeaderSet, allowed_cross_origin_response_headers,
      ({
          "cache-control", "content-language", "content-type", "expires",
          "last-modified", "pragma",
      }));
  return allowed_cross_origin_response_headers.find(name.Ascii().data()) !=
         allowed_cross_origin_response_headers.end();
}

WebString ListOfCORSEnabledURLSchemes() {
  return SchemeRegistry::ListOfCORSEnabledURLSchemes();
}

bool ContainsOnlyCORSSafelistedOrForbiddenHeaders(const WebHTTPHeaderMap& map) {
  return FetchUtils::ContainsOnlyCORSSafelistedOrForbiddenHeaders(
      map.GetHTTPHeaderMap());
}

// No-CORS requests are allowed for all these contexts, and plugin contexts with
// private permission when we set ServiceWorkerMode to None in
// PepperURLLoaderHost.
bool IsNoCORSAllowedContext(
    WebURLRequest::RequestContext context,
    WebURLRequest::ServiceWorkerMode service_worker_mode) {
  switch (context) {
    case WebURLRequest::kRequestContextAudio:
    case WebURLRequest::kRequestContextFavicon:
    case WebURLRequest::kRequestContextFetch:
    case WebURLRequest::kRequestContextImage:
    case WebURLRequest::kRequestContextObject:
    case WebURLRequest::kRequestContextScript:
    case WebURLRequest::kRequestContextSharedWorker:
    case WebURLRequest::kRequestContextVideo:
    case WebURLRequest::kRequestContextWorker:
      return true;
    case WebURLRequest::kRequestContextPlugin:
      return service_worker_mode == WebURLRequest::ServiceWorkerMode::kNone;
    default:
      return false;
  }
}

}  // namespace WebCORS

}  // namespace blink
