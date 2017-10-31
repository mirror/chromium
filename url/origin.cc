// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "url/origin.h"

#include <stdint.h>
#include <string.h>

#include <new>
#include <tuple>
#include <utility>

#include "base/logging.h"
#include "base/strings/string_number_conversions.h"
#include "url/gurl.h"
#include "url/url_canon.h"
#include "url/url_canon_stdstring.h"
#include "url/url_constants.h"
#include "url/url_util.h"

namespace url {

namespace {

GURL AddSuboriginToUrl(const GURL& url, const std::string& suborigin) {
  GURL::Replacements replacements;
  if (url.scheme() == kHttpScheme) {
    replacements.SetSchemeStr(kHttpSuboriginScheme);
  } else {
    DCHECK(url.scheme() == kHttpsScheme);
    replacements.SetSchemeStr(kHttpsSuboriginScheme);
  }
  std::string new_host = suborigin + "." + url.host();
  replacements.SetHostStr(new_host);
  return url.ReplaceComponents(replacements);
}

}  // namespace

Origin::Origin()
    : unique_id_(base::UnguessableToken::Create()), unique_(true) {}

Origin Origin::Create(const GURL& url) {
  if (!url.is_valid() || (!url.IsStandard() && !url.SchemeIsBlob()))
    return Origin();

  SchemeHostPort scheme_host_port;
  std::string suborigin;

  if (url.SchemeIsFileSystem()) {
    scheme_host_port = SchemeHostPort(*url.inner_url());
  } else if (url.SchemeIsBlob()) {
    // If we're dealing with a 'blob:' URL, https://url.spec.whatwg.org/#origin
    // defines the origin as the origin of the URL which results from parsing
    // the "path", which boils down to everything after the scheme. GURL's
    // 'GetContent()' gives us exactly that.
    scheme_host_port = SchemeHostPort(GURL(url.GetContent()));
  } else if (url.SchemeIsSuborigin()) {
    GURL::Replacements replacements;
    if (url.scheme() == kHttpSuboriginScheme) {
      replacements.SetSchemeStr(kHttpScheme);
    } else {
      DCHECK(url.scheme() == kHttpsSuboriginScheme);
      replacements.SetSchemeStr(kHttpsScheme);
    }

    std::string host = url.host();
    size_t suborigin_end = host.find(".");
    bool no_dot = suborigin_end == std::string::npos;
    std::string new_host(
        no_dot ? ""
               : host.substr(suborigin_end + 1,
                             url.host().length() - suborigin_end - 1));
    replacements.SetHostStr(new_host);

    scheme_host_port = SchemeHostPort(url.ReplaceComponents(replacements));

    bool invalid_suborigin = no_dot || suborigin_end == 0;
    if (invalid_suborigin || scheme_host_port.IsInvalid())
      return Origin();
    suborigin = host.substr(0, suborigin_end);
  } else {
    scheme_host_port = SchemeHostPort(url);
  }

  if (scheme_host_port.IsInvalid())
    return Origin();

  return Origin(std::move(scheme_host_port), std::move(suborigin));
}

Origin::Origin(SchemeHostPort scheme_host_port, std::string suborigin)
    : tuple_(std::move(scheme_host_port), std::move(suborigin)),
      unique_(false) {
  DCHECK(!tuple_.scheme_host_port.IsInvalid());
}

Origin::Origin(const Origin& other) {
  CopyConstructFrom(other);
}

Origin& Origin::operator=(const Origin& other) {
  Cleanup();
  CopyConstructFrom(other);
  return *this;
}

Origin::Origin(Origin&& other) {
  MoveConstructFrom(std::move(other));
}

Origin& Origin::operator=(Origin&& other) {
  Cleanup();
  MoveConstructFrom(std::move(other));
  return *this;
}

Origin::~Origin() {
  Cleanup();
}

// static
Origin Origin::UnsafelyCreateOriginWithoutNormalization(
    base::StringPiece scheme,
    base::StringPiece host,
    uint16_t port,
    base::StringPiece suborigin) {
  SchemeHostPort scheme_host_port(scheme.as_string(), host.as_string(), port,
                                  SchemeHostPort::CHECK_CANONICALIZATION);
  if (scheme_host_port.IsInvalid())
    return Origin();
  return Origin(std::move(scheme_host_port), suborigin.as_string());
}

Origin Origin::CreateFromNormalizedTupleWithSuborigin(
    base::StringPiece scheme,
    base::StringPiece host,
    uint16_t port,
    base::StringPiece suborigin) {
  SchemeHostPort scheme_host_port(scheme.as_string(), host.as_string(), port,
                                  SchemeHostPort::ALREADY_CANONICALIZED);
  if (scheme_host_port.IsInvalid())
    return Origin();
  return Origin(std::move(scheme_host_port), suborigin.as_string());
}

std::string Origin::Serialize() const {
  if (unique())
    return "null";

  if (scheme() == kFileScheme)
    return "file://";

  return tuple_.Serialize();
}

Origin Origin::GetPhysicalOrigin() const {
  if (unique_ || tuple_.suborigin.empty())
    return *this;

  return Origin(tuple_.scheme_host_port, std::string());
}

GURL Origin::GetURL() const {
  if (unique())
    return GURL();

  if (scheme() == kFileScheme)
    return GURL("file:///");

  return tuple_.GetURL();
}

bool Origin::IsSameOriginWith(const Origin& other) const {
  if (!unique_ && !other.unique_) {
    return tuple_ == other.tuple_;
  } else if (unique_ && other.unique_) {
    return unique_id_ == other.unique_id_;
  }
  return false;
}

bool Origin::IsSamePhysicalOriginWith(const Origin& other) const {
  return GetPhysicalOrigin().IsSameOriginWith(other.GetPhysicalOrigin());
}

bool Origin::DomainIs(base::StringPiece canonical_domain) const {
  return !unique_ &&
         url::DomainIs(tuple_.scheme_host_port.host(), canonical_domain);
}

bool Origin::operator<(const Origin& other) const {
  if (!unique_ && !other.unique_) {
    return tuple_ < other.tuple_;
  } else if (unique_ && other.unique_) {
    return unique_id_ < other.unique_id_;
  }
  // non-unique always sorts before unique
  return unique_ < other.unique_;
}

Origin::OriginTuple::OriginTuple(SchemeHostPort scheme_host_port,
                                 std::string suborigin)
    : scheme_host_port(std::move(scheme_host_port)),
      suborigin(std::move(suborigin)) {}

GURL Origin::OriginTuple::GetURL() const {
  GURL tuple_url(scheme_host_port.GetURL());

  if (!suborigin.empty())
    return AddSuboriginToUrl(tuple_url, suborigin);

  return tuple_url;
}

std::string Origin::OriginTuple::Serialize() const {
  if (!suborigin.empty()) {
    GURL url_with_suborigin =
        AddSuboriginToUrl(scheme_host_port.GetURL(), suborigin);
    return SchemeHostPort(url_with_suborigin).Serialize();
  }
  return scheme_host_port.Serialize();
}

bool Origin::OriginTuple::operator==(const OriginTuple& other) const {
  return scheme_host_port.Equals(other.scheme_host_port) &&
         suborigin == other.suborigin;
}

bool Origin::OriginTuple::operator<(const OriginTuple& other) const {
  return std::tie(scheme_host_port, suborigin) <
         std::tie(other.scheme_host_port, other.suborigin);
}

void Origin::Cleanup() {
  if (!unique_)
    tuple_.~OriginTuple();
  // Nothing needed if |unique_| is true, since |unique_id_| is a POD.
}

void Origin::CopyConstructFrom(const Origin& other) {
  unique_ = other.unique_;
  if (!unique_)
    new (&tuple_) OriginTuple(other.tuple_);
  else
    unique_id_ = other.unique_id_;
}

void Origin::MoveConstructFrom(Origin&& other) {
  unique_ = other.unique_;
  if (!unique_) {
    new (&tuple_) OriginTuple(std::move(other.tuple_));
  } else {
    // base::UnguessableToken is a POD so std::move() doesn't really do
    // anything; use it anyway for consistency.
    unique_id_ = std::move(other.unique_id_);
  }
}

std::ostream& operator<<(std::ostream& out, const url::Origin& origin) {
  return out << origin.Serialize();
}

bool IsSameOriginWith(const GURL& a, const GURL& b) {
  return Origin::Create(a).IsSameOriginWith(Origin::Create(b));
}

bool IsSamePhysicalOriginWith(const GURL& a, const GURL& b) {
  return Origin::Create(a).IsSamePhysicalOriginWith(Origin::Create(b));
}

}  // namespace url
