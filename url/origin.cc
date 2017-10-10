// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "url/origin.h"

#include <stdint.h>
#include <string.h>

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
    : unique_(true), unique_id_(base::UnguessableToken::Create()) {}

Origin::Origin(const GURL& url) : unique_(true) {
  if (!url.is_valid() || (!url.IsStandard() && !url.SchemeIsBlob())) {
    unique_id_ = base::UnguessableToken::Create();
    return;
  }

  SchemeHostPort tuple;
  std::string suborigin;
  if (url.SchemeIsFileSystem()) {
    tuple = SchemeHostPort(*url.inner_url());
  } else if (url.SchemeIsBlob()) {
    // If we're dealing with a 'blob:' URL, https://url.spec.whatwg.org/#origin
    // defines the origin as the origin of the URL which results from parsing
    // the "path", which boils down to everything after the scheme. GURL's
    // 'GetContent()' gives us exactly that.
    tuple = SchemeHostPort(GURL(url.GetContent()));
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

    tuple = SchemeHostPort(url.ReplaceComponents(replacements));

    bool invalid_suborigin = no_dot || suborigin_end == 0;
    if (invalid_suborigin || tuple.IsInvalid())
      return;

    suborigin = host.substr(0, suborigin_end);
  } else {
    tuple = SchemeHostPort(url);
  }

  unique_ = tuple.IsInvalid();
  if (unique_) {
    unique_id_ = base::UnguessableToken::Create();
  } else {
    tuple_ = std::move(tuple);
    suborigin_ = std::move(suborigin);
  }
}

Origin::Origin(const Origin& rhs) : unique_(rhs.unique_) {
  if (unique_) {
    unique_id_ = rhs.unique_id_;
  } else {
    tuple_ = rhs.tuple_;
    suborigin_ = rhs.suborigin_;
  }
}

Origin& Origin::operator=(const Origin& rhs) {
  unique_ = rhs.unique_;
  if (unique_) {
    unique_id_ = rhs.unique_id_;
  } else {
    tuple_ = rhs.tuple_;
    suborigin_ = rhs.suborigin_;
  }
  return *this;
}

Origin::Origin(Origin&& rhs) : unique_(rhs.unique_) {
  if (unique_) {
    unique_id_ = rhs.unique_id_;
  } else {
    tuple_ = std::move(rhs.tuple_);
    suborigin_ = std::move(rhs.suborigin_);
  }
}

Origin& Origin::operator=(Origin&& rhs) {
  if (unique_) {
    unique_id_ = rhs.unique_id_;
  } else {
    tuple_ = std::move(rhs.tuple_);
    suborigin_ = std::move(rhs.suborigin_);
  }
  return *this;
}

Origin::Origin(base::StringPiece scheme,
               base::StringPiece host,
               uint16_t port,
               base::StringPiece suborigin,
               SchemeHostPort::ConstructPolicy policy)
    : Origin(scheme.as_string(),
             host.as_string(),
             port,
             suborigin.as_string(),
             policy) {}

Origin::Origin(std::string scheme,
               std::string host,
               uint16_t port,
               std::string suborigin,
               SchemeHostPort::ConstructPolicy policy) {
  SchemeHostPort tuple(std::move(scheme), std::move(host), port, policy);
  unique_ = tuple.IsInvalid();
  if (unique_) {
    unique_id_ = base::UnguessableToken::Create();
  } else {
    tuple_ = std::move(tuple);
    suborigin_ = std::move(suborigin);
  }
}

Origin::~Origin() {
  if (!unique_) {
    tuple_.~SchemeHostPort();
    suborigin_.~basic_string();
  }
}

// static
Origin Origin::UnsafelyCreateOriginWithoutNormalization(
    base::StringPiece scheme,
    base::StringPiece host,
    uint16_t port,
    base::StringPiece suborigin) {
  return Origin(scheme, host, port, suborigin,
                SchemeHostPort::CHECK_CANONICALIZATION);
}

Origin Origin::CreateFromNormalizedTupleWithSuborigin(
    std::string scheme,
    std::string host,
    uint16_t port,
    std::string suborigin) {
  return Origin(std::move(scheme), std::move(host), port, std::move(suborigin),
                SchemeHostPort::ALREADY_CANONICALIZED);
}

std::string Origin::Serialize() const {
  if (unique())
    return "null";

  if (scheme() == kFileScheme)
    return "file://";

  if (!suborigin_.empty()) {
    GURL url_with_suborigin = AddSuboriginToUrl(tuple_.GetURL(), suborigin_);
    return SchemeHostPort(url_with_suborigin).Serialize();
  }

  return tuple_.Serialize();
}

Origin Origin::GetPhysicalOrigin() const {
  if (suborigin_.empty())
    return *this;

  return Origin(tuple_.GetURL());
}

GURL Origin::GetURL() const {
  if (unique())
    return GURL();

  if (scheme() == kFileScheme)
    return GURL("file:///");

  GURL tuple_url(tuple_.GetURL());

  if (!suborigin_.empty())
    return AddSuboriginToUrl(tuple_url, suborigin_);

  return tuple_url;
}

bool Origin::IsSameOriginWith(const Origin& other) const {
  if (unique_ || other.unique_) {
    return unique_ && other.unique_ ? unique_id_ == other.unique_id_ : false;
  }

  return tuple_.Equals(other.tuple_) && suborigin_ == other.suborigin_;
}

bool Origin::IsSamePhysicalOriginWith(const Origin& other) const {
  return GetPhysicalOrigin().IsSameOriginWith(other.GetPhysicalOrigin());
}

bool Origin::DomainIs(base::StringPiece canonical_domain) const {
  return !unique_ && url::DomainIs(tuple_.host(), canonical_domain);
}

bool Origin::operator<(const Origin& other) const {
  return std::tie(tuple_, suborigin_, unique_id_) <
         std::tie(other.tuple_, other.suborigin_, other.unique_id_);
}

std::ostream& operator<<(std::ostream& out, const url::Origin& origin) {
  return out << origin.Serialize();
}

bool IsSameOriginWith(const GURL& a, const GURL& b) {
  return Origin(a).IsSameOriginWith(Origin(b));
}

bool IsSamePhysicalOriginWith(const GURL& a, const GURL& b) {
  return Origin(a).IsSamePhysicalOriginWith(Origin(b));
}

}  // namespace url
