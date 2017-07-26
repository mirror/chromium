// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/common/cookie_traits.h"

namespace mojo {

content::mojom::CookiePriority
EnumTraits<content::mojom::CookiePriority, net::CookiePriority>::ToMojom(
    net::CookiePriority input) {
  switch (input) {
    case net::COOKIE_PRIORITY_LOW:
      return content::mojom::CookiePriority::LOW;
    case net::COOKIE_PRIORITY_MEDIUM:
      return content::mojom::CookiePriority::MEDIUM;
    case net::COOKIE_PRIORITY_HIGH:
      return content::mojom::CookiePriority::HIGH;
  }
  NOTREACHED();
  return static_cast<content::mojom::CookiePriority>(input);
}

bool EnumTraits<content::mojom::CookiePriority, net::CookiePriority>::FromMojom(
    content::mojom::CookiePriority input,
    net::CookiePriority* output) {
  switch (input) {
    case content::mojom::CookiePriority::LOW:
      *output = net::CookiePriority::COOKIE_PRIORITY_LOW;
      return true;
    case content::mojom::CookiePriority::MEDIUM:
      *output = net::CookiePriority::COOKIE_PRIORITY_MEDIUM;
      return true;
    case content::mojom::CookiePriority::HIGH:
      *output = net::CookiePriority::COOKIE_PRIORITY_HIGH;
      return true;
  }
  return false;
}

content::mojom::CookieSameSite
EnumTraits<content::mojom::CookieSameSite, net::CookieSameSite>::ToMojom(
    net::CookieSameSite input) {
  switch (input) {
    case net::CookieSameSite::NO_RESTRICTION:
      return content::mojom::CookieSameSite::NO_RESTRICTION;
    case net::CookieSameSite::LAX_MODE:
      return content::mojom::CookieSameSite::LAX_MODE;
    case net::CookieSameSite::STRICT_MODE:
      return content::mojom::CookieSameSite::STRICT_MODE;
  }
  NOTREACHED();
  return static_cast<content::mojom::CookieSameSite>(input);
}

bool EnumTraits<content::mojom::CookieSameSite, net::CookieSameSite>::FromMojom(
    content::mojom::CookieSameSite input,
    net::CookieSameSite* output) {
  switch (input) {
    case content::mojom::CookieSameSite::NO_RESTRICTION:
      *output = net::CookieSameSite::NO_RESTRICTION;
      return true;
    case content::mojom::CookieSameSite::LAX_MODE:
      *output = net::CookieSameSite::LAX_MODE;
      return true;
    case content::mojom::CookieSameSite::STRICT_MODE:
      *output = net::CookieSameSite::STRICT_MODE;
      return true;
  }
  return false;
}

content::mojom::CookieSameSiteFilter
EnumTraits<content::mojom::CookieSameSiteFilter,
           net::CookieOptions::SameSiteCookieMode>::
    ToMojom(net::CookieOptions::SameSiteCookieMode input) {
  switch (input) {
    case net::CookieOptions::SameSiteCookieMode::INCLUDE_STRICT_AND_LAX:
      return content::mojom::CookieSameSiteFilter::INCLUDE_STRICT_AND_LAX;
    case net::CookieOptions::SameSiteCookieMode::INCLUDE_LAX:
      return content::mojom::CookieSameSiteFilter::INCLUDE_LAX;
    case net::CookieOptions::SameSiteCookieMode::DO_NOT_INCLUDE:
      return content::mojom::CookieSameSiteFilter::DO_NOT_INCLUDE;
  }
  NOTREACHED();
  return static_cast<content::mojom::CookieSameSiteFilter>(input);
}

bool EnumTraits<content::mojom::CookieSameSiteFilter,
                net::CookieOptions::SameSiteCookieMode>::
    FromMojom(content::mojom::CookieSameSiteFilter input,
              net::CookieOptions::SameSiteCookieMode* output) {
  switch (input) {
    case content::mojom::CookieSameSiteFilter::INCLUDE_STRICT_AND_LAX:
      *output = net::CookieOptions::SameSiteCookieMode::INCLUDE_STRICT_AND_LAX;
      return true;
    case content::mojom::CookieSameSiteFilter::INCLUDE_LAX:
      *output = net::CookieOptions::SameSiteCookieMode::INCLUDE_LAX;
      return true;
    case content::mojom::CookieSameSiteFilter::DO_NOT_INCLUDE:
      *output = net::CookieOptions::SameSiteCookieMode::DO_NOT_INCLUDE;
      return true;
  }
  return false;
}

}  // namespace mojo
