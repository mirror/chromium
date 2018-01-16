// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/quic/platform/impl/quic_url_utils_impl.h"

#include "url/gurl.h"
#include "url/origin.h"
#include "url/url_canon.h"

using std::string;

namespace net {

// static
string QuicUrlUtilsImpl::GetPushPromiseUrl(QuicStringPiece scheme,
                                           QuicStringPiece authority,
                                           QuicStringPiece path) {
  // RFC 7540, Section 8.1.2.3: The ":path" pseudo-header field includes the
  // path and query parts of the target URI (the "path-absolute" production
  // and optionally a '?' character followed by the "query" production (see
  // Sections 3.3 and 3.4 of RFC3986). A request in asterisk form includes the
  // value '*' for the ":path" pseudo-header field.
  //
  // This pseudo-header field MUST NOT be empty for "http" or "https" URIs;
  // "http" or "https" URIs that do not contain a path MUST include a value of
  // '/'. The exception to this rule is an OPTIONS request for an "http" or
  // "https" URI that does not include a path component; these MUST include a
  // ":path" pseudo-header with a value of '*' (see RFC7230, Section 5.3.4).
  //
  // In addition to the above restriction from RFC 7540, note that RFC3986
  // defines the "path-absolute" construction as starting with a '/'
  // character.
  if (path.empty() || (path != "*" && path[0] != '/')) {
    return std::string();
  }

  // Validate the scheme; this is to ensure a scheme of "foo://bar" is not
  // parsed as a URL of "foo://bar://baz" when combined with a host of "baz".
  std::string canonical_scheme;
  url::StdStringCanonOutput canon_output(&canonical_scheme);
  url::Component canon_component;
  url::Component scheme_component(0, scheme.size());

  bool success = url::CanonicalizeScheme(scheme.data(), scheme_component,
                                         &canon_output, &canon_component);
  if (!success || !canon_component.is_nonempty() ||
      canon_component.begin != 0) {
    return std::string();
  }
  canonical_scheme.resize(canon_component.len + 1);

  // Attempt to construct the scheme, host, and port tuple. The following
  // restrictions are enforced:
  // - The URL must be parsable (e.g. to reject invalid inputs)
  // - The Push URL must be HTTP or HTTPS (this is not strictly a security
  //   check, but applies to the assumptions that follow). It also restricts
  //   the use of the HTTP suborigin, since those should never be
  //   wire-visible.
  // - RFC 7540, Section 8.1.2.3: The authority MUST NOT include the
  //   deprecated "userinfo" subcomponent for "http" or "https" schemed URIs.
  // - To guard against malicious values for |authority|, ensure that no
  //   trailing data is parsed (path, query, ref).
  //
  // url::CanonicalizeScheme() will have added the ":" to |canonical_scheme|.
  GURL origin_url(canonical_scheme + "//" + authority.as_string());
  if (!origin_url.is_valid() || !origin_url.SchemeIsHTTPOrHTTPS() ||
      origin_url.SchemeIsSuborigin() || origin_url.has_username() ||
      origin_url.has_password() ||
      (origin_url.has_path() && origin_url.path_piece() != "/") ||
      origin_url.has_query() || origin_url.has_ref()) {
    return std::string();
  }

  // Attempt to parse the path.
  std::string spec = origin_url.GetWithEmptyPath().spec();
  spec.pop_back();  // Remove the '/', as ":path" must contain it (or *).
  if (path != "*") {
    // Note: |path| MUST contain at least a '/' at this point, due to the
    // above checks.
    path.AppendToString(&spec);
  }

  // Attempt to parse the full URL, with the path as well. Ensure that there
  // are no ambiguities introduced to the authority, and ensure there is no
  // fragment to the query.
  GURL full_url(spec);
  if (!full_url.is_valid() || full_url.has_ref() ||
      !url::Origin::Create(full_url).IsSameOriginWith(
          url::Origin::Create(origin_url))) {
    return std::string();
  }

  return full_url.spec();
}

// static
string QuicUrlUtilsImpl::HostName(QuicStringPiece url) {
  return GURL(url).host();
}

// static
bool QuicUrlUtilsImpl::IsValidUrl(QuicStringPiece url) {
  return GURL(url).is_valid();
}

}  // namespace net
