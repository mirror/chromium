// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/url_formatter/elide_url.h"

#include <stddef.h>

#include "base/i18n/rtl.h"
#include "base/logging.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_piece.h"
#include "base/strings/string_split.h"
#include "base/strings/utf_string_conversions.h"
#include "build/build_config.h"
#include "components/url_formatter/url_formatter.h"
#include "net/base/escape.h"
#include "net/base/registry_controlled_domains/registry_controlled_domain.h"
#include "url/gurl.h"
#include "url/origin.h"
#include "url/url_constants.h"

#include "ui/gfx/text_elider.h"  // nogncheck
#include "ui/gfx/text_utils.h"  // nogncheck

namespace {

#if !defined(OS_ANDROID)
const base::char16 kDot = '.';

// Build a path using |num_elements| from |elements|. The second-last element is
// removed until only the last element remains.
base::string16 BuildPathFromComponents(
    const std::vector<base::string16>& elements,
    size_t num_elements) {
  DCHECK_LT(num_elements, elements.size());
  DCHECK_GT(num_elements, 0u);

  // Build path from first |num_elements - 1| elements.
  base::string16 path;
  path += gfx::kForwardSlash;
  for (size_t i = 0; i < num_elements - 1; ++i)
    path += elements[i] + gfx::kForwardSlash;

  // Add ellipsis if necessary.
  if (num_elements < elements.size())
    path += base::string16(gfx::kEllipsisUTF16) + gfx::kForwardSlash;

  path += elements.back();
  return path;
}

// Adjust |parsed| to describe a range within |original_length|, of length
// |new_length| at |offset|.
void AdjustParsed(int original_length,
                  int new_length,
                  int offset,
                  url::Parsed* parsed) {
  DCHECK_GE(new_length, 0);
  DCHECK_GE(original_length, 0);
  DCHECK_GE(offset, 0);
  DCHECK_LE(offset + new_length, original_length);

  if (offset == 0 && original_length == new_length)
    return;

  const std::vector<url::Component*> components = {
      &(parsed->scheme), &(parsed->username), &(parsed->password),
      &(parsed->host),   &(parsed->port),     &(parsed->path),
      &(parsed->query),  &(parsed->ref),
  };

  for (auto* component : components) {
    if (!component->is_valid())
      continue;
    int begin = component->begin - offset;
    int end = component->end() - offset;
    begin = std::max(std::min(begin, new_length), 0);
    end = std::max(std::min(end, new_length), 0);
    component->begin = begin;
    component->len = end - begin;
    // If we reduced a component to nothing, invalidate it.
    if (component->len == 0) {
      component->begin = 0;
      component->len = -1;
    }
  }
}

// Elide a URL string with ellipsis at either the head or tail end, and adjust
// |parsed| accordingly. This allows a formatted URL to be elided while
// maintaining the Parsed description of the result.
base::string16 ElideParsedUrlString(const base::string16& original,
                                    const gfx::FontList& font_list,
                                    float available_pixel_width,
                                    gfx::ElideBehavior elide_behavior,
                                    url::Parsed* parsed) {
  DCHECK(elide_behavior == gfx::ELIDE_TAIL ||
         elide_behavior == gfx::ELIDE_HEAD);

  base::string16 elided = gfx::ElideText(original, font_list,
                                         available_pixel_width, elide_behavior);
  if (elided == original)
    return elided;

  // If elision reduced the string to a tiny remaining fragment, standardize it.
  if (elided == (base::string16(gfx::kEllipsisUTF16) + gfx::kEllipsisUTF16))
    elided = base::string16(gfx::kEllipsisUTF16);

  int offset =
      (elide_behavior == gfx::ELIDE_HEAD) ? original.size() - elided.size() : 0;
  AdjustParsed(original.size(), elided.size(), offset, parsed);
  return elided;
}

// Attempt to elide a URL by shortening its path on a per-element basis.
base::string16 ElideParsedUrlStringPath(const base::string16& original,
                                        const gfx::FontList& font_list,
                                        float available_pixel_width,
                                        url::Parsed* parsed) {
  DCHECK_GT(parsed->path.len, 0);

  base::string16 prefix = original.substr(0, parsed->path.begin);
  base::string16 suffix =
      original.substr(parsed->path.end(), original.size() - parsed->path.end());

  // If the path fits without the query and ref, use basic eliding.
  base::string16 shortest = original.substr(0, parsed->path.end());
  if (!suffix.empty())
    shortest += base::string16(gfx::kEllipsisUTF16);
  if (available_pixel_width >= gfx::GetStringWidthF(shortest, font_list)) {
    return ElideParsedUrlString(original, font_list, available_pixel_width,
                                gfx::ELIDE_TAIL, parsed);
  }

  // At this point, we know we'll be showing a partial path.  Chop up the path
  // into pieces, and progressively remove segments until there are none, as the
  // caller has already guaranteed that completely removing the path allows the
  // URL to fit.
  base::string16 path = original.substr(parsed->path.begin, parsed->path.len);
  std::vector<base::string16> path_elements =
      base::SplitString(path, base::string16(1, gfx::kForwardSlash),
                        base::TRIM_WHITESPACE, base::SPLIT_WANT_ALL);

  // TODO - do we actually want all part?  Presumably we can throw away empty
  // parts, as they would have already beend formatted out?

  // Trim empty leading and trailing parts.
  if (path_elements.size() > 0 && path_elements.front().empty())
    path_elements.erase(path_elements.begin());
  if (path_elements.size() > 0 && path_elements.back().empty()) {
    path_elements.pop_back();
    path_elements.back() += gfx::kForwardSlash;
  }

  if (path_elements.empty()) {
    return ElideParsedUrlString(original, font_list, available_pixel_width,
                                gfx::ELIDE_TAIL, parsed);
  }

  // If there is more than one path element, try removing elements.
  for (size_t i = path_elements.size() - 1; i > 0; --i) {
    base::string16 new_path = BuildPathFromComponents(path_elements, i);
    base::string16 new_url = prefix + new_path;
    if (!suffix.empty()) {
      new_url += base::string16(gfx::kEllipsisUTF16);
    }
    if (available_pixel_width >= gfx::GetStringWidthF(new_url, font_list))
      return ElideParsedUrlString(prefix + new_path + suffix, font_list,
                                  available_pixel_width, gfx::ELIDE_TAIL,
                                  parsed);
  }

  return ElideParsedUrlString(original, font_list, available_pixel_width,
                              gfx::ELIDE_TAIL, parsed);
}

// Splits the hostname in the |url| into sub-strings for the full hostname,
// the domain (TLD+1), and the subdomain (everything leading the domain).
void SplitHost(const GURL& url,
               base::string16* url_host,
               base::string16* url_domain,
               base::string16* url_subdomain) {
  // GURL stores IDN hostnames in punycode.  Convert back to Unicode for
  // display to the user.  (IDNToUnicode() will only perform this conversion
  // if it's safe to display this host/domain in Unicode.)
  *url_host = url_formatter::IDNToUnicode(url.host());

  // Get domain and registry information from the URL.
  std::string domain_puny =
      net::registry_controlled_domains::GetDomainAndRegistry(
          url, net::registry_controlled_domains::EXCLUDE_PRIVATE_REGISTRIES);
  *url_domain = domain_puny.empty() ?
      *url_host : url_formatter::IDNToUnicode(domain_puny);

  // Add port if required.
  if (!url.port().empty()) {
    *url_host += base::UTF8ToUTF16(":" + url.port());
    *url_domain += base::UTF8ToUTF16(":" + url.port());
  }

  // Get sub domain.
  const size_t domain_start_index = url_host->find(*url_domain);
  base::string16 kWwwPrefix = base::UTF8ToUTF16("www.");
  if (domain_start_index != base::string16::npos)
    *url_subdomain = url_host->substr(0, domain_start_index);
  if ((*url_subdomain == kWwwPrefix || url_subdomain->empty() ||
       url.SchemeIsFile())) {
    url_subdomain->clear();
  }
}
#endif  // !defined(OS_ANDROID)

bool ShouldShowScheme(base::StringPiece scheme,
                      const url_formatter::SchemeDisplay scheme_display) {
  switch (scheme_display) {
    case url_formatter::SchemeDisplay::SHOW:
      return true;

    case url_formatter::SchemeDisplay::OMIT_HTTP_AND_HTTPS:
      return scheme != url::kHttpsScheme && scheme != url::kHttpScheme;

    case url_formatter::SchemeDisplay::OMIT_CRYPTOGRAPHIC:
      return scheme != url::kHttpsScheme && scheme != url::kWssScheme;
  }

  return true;
}

// TODO(jshin): Come up with a way to show Bidi URLs 'safely' (e.g. wrap up
// the entire url with {LSI, PDI} and individual domain labels with {FSI, PDI}).
// See http://crbug.com/650760 . For now, fall back to punycode if there's a
// strong RTL character.
base::string16 HostForDisplay(base::StringPiece host_in_puny) {
  base::string16 host = url_formatter::IDNToUnicode(host_in_puny);
  return base::i18n::StringContainsStrongRTLChars(host) ?
      base::ASCIIToUTF16(host_in_puny) : host;
}

}  // namespace

namespace url_formatter {

#if !defined(OS_ANDROID)

// TODO(pkasting): http://crbug.com/77883 This whole function gets
// kerning/ligatures/etc. issues potentially wrong by assuming that the width of
// a rendered string is always the sum of the widths of its substrings.  Also I
// suspect it could be made simpler.
base::string16 ElideUrl(const GURL& url,
                        const gfx::FontList& font_list,
                        float available_pixel_width) {
  url::Parsed parsed;
  const base::string16 url_string = url_formatter::FormatUrl(
      url, url_formatter::kFormatUrlOmitDefaults, net::UnescapeRule::SPACES,
      &parsed, nullptr, nullptr);

  return SecurelyElideFormattedUrl(url, url_string, font_list,
                                   available_pixel_width, true, &parsed);
}

base::string16 SimpleElideUrl(const GURL& url,
                              const gfx::FontList& font_list,
                              float available_pixel_width,
                              url::Parsed* parsed) {
  const base::string16 url_string = url_formatter::FormatUrl(
      url, url_formatter::kFormatUrlOmitDefaults, net::UnescapeRule::SPACES,
      parsed, nullptr, nullptr);

  return SecurelyElideFormattedUrl(url, url_string, font_list,
                                   available_pixel_width, false, parsed);
}

base::string16 ElideHost(const GURL& url,
                         const gfx::FontList& font_list,
                         float available_pixel_width) {
  base::string16 url_host;
  base::string16 url_domain;
  base::string16 url_subdomain;
  SplitHost(url, &url_host, &url_domain, &url_subdomain);

  const float pixel_width_url_host = gfx::GetStringWidthF(url_host, font_list);
  if (available_pixel_width >= pixel_width_url_host)
    return url_host;

  if (url_subdomain.empty())
    return url_domain;

  const float pixel_width_url_domain =
      gfx::GetStringWidthF(url_domain, font_list);
  float subdomain_width = available_pixel_width - pixel_width_url_domain;
  if (subdomain_width <= 0)
    return base::string16(gfx::kEllipsisUTF16) + kDot + url_domain;

  return gfx::ElideText(url_host, font_list, available_pixel_width,
                        gfx::ELIDE_HEAD);
}

#endif  // !defined(OS_ANDROID)

base::string16 FormatUrlForSecurityDisplay(const GURL& url,
                                           const SchemeDisplay scheme_display) {
  if (!url.is_valid() || url.is_empty() || !url.IsStandard())
    return url_formatter::FormatUrl(url);

  const base::string16 colon(base::ASCIIToUTF16(":"));
  const base::string16 scheme_separator(
      base::ASCIIToUTF16(url::kStandardSchemeSeparator));

  if (url.SchemeIsFile()) {
    return base::ASCIIToUTF16(url::kFileScheme) + scheme_separator +
           base::UTF8ToUTF16(url.path());
  }

  if (url.SchemeIsFileSystem()) {
    const GURL* inner_url = url.inner_url();
    if (inner_url->SchemeIsFile()) {
      return base::ASCIIToUTF16(url::kFileSystemScheme) + colon +
             FormatUrlForSecurityDisplay(*inner_url) +
             base::UTF8ToUTF16(url.path());
    }
    return base::ASCIIToUTF16(url::kFileSystemScheme) + colon +
           FormatUrlForSecurityDisplay(*inner_url);
  }

  const GURL origin = url.GetOrigin();
  base::StringPiece scheme = origin.scheme_piece();
  base::StringPiece host = origin.host_piece();

  base::string16 result;
  if (ShouldShowScheme(scheme, scheme_display))
    result = base::UTF8ToUTF16(scheme) + scheme_separator;
  result += HostForDisplay(host);

  const int port = origin.IntPort();
  const int default_port = url::DefaultPortForScheme(
      scheme.data(), static_cast<int>(scheme.length()));
  if (port != url::PORT_UNSPECIFIED && port != default_port)
    result += colon + base::UTF8ToUTF16(origin.port_piece());

  return result;
}

base::string16 FormatOriginForSecurityDisplay(
    const url::Origin& origin,
    const SchemeDisplay scheme_display) {
  base::StringPiece scheme = origin.scheme();
  base::StringPiece host = origin.host();
  if (scheme.empty() && host.empty())
    return base::string16();

  const base::string16 colon(base::ASCIIToUTF16(":"));
  const base::string16 scheme_separator(
      base::ASCIIToUTF16(url::kStandardSchemeSeparator));

  base::string16 result;
  if (ShouldShowScheme(scheme, scheme_display))
    result = base::UTF8ToUTF16(scheme) + scheme_separator;
  result += HostForDisplay(host);

  int port = static_cast<int>(origin.port());
  const int default_port = url::DefaultPortForScheme(
      scheme.data(), static_cast<int>(scheme.length()));
  if (port != 0 && port != default_port)
    result += colon + base::UintToString16(origin.port());

  return result;
}

#if 0
  Simplified ElideUrl (proposed):

1. Cut off scheme (if http).
2. Elide query+fragment from the end.
3. Cut off path segments from second-last, backwards, until all but the last path segment is cut.
4. Cut off the last path segment.
5. Elide the scheme+domain from the start.

Omnibox:

1. Cut off scheme (if http).
2. Elide query+fragment from the end.
3. Elide the path from the end.
4. Elide the scheme+domain from the start.
#endif

base::string16 SecurelyElideFormattedUrl(
    const GURL& url,
    const base::string16& original_url_string,
    const gfx::FontList& font_list,
    float available_pixel_width,
    bool preserve_filename,
    url::Parsed* parsed) {
  DCHECK_NE(parsed, nullptr);

  base::string16 url_string = original_url_string;

  if (available_pixel_width <= 0)
    return url_string;

  // If non-standard, or a file URL, return plain eliding.
  if (!url.IsStandard() || url.SchemeIsFile())
    return ElideParsedUrlString(url_string, font_list, available_pixel_width,
                                gfx::ELIDE_TAIL, parsed);

  // If entire string fits, return it.
  if (available_pixel_width >= gfx::GetStringWidthF(url_string, font_list))
    return url_string;

  base::string16 trailing_elision =
      gfx::kForwardSlash + base::string16(gfx::kEllipsisUTF16);

  // Check if there is anything after the hostname (path, query, ref).
  // TODO(cjgrant): Properly handle having no host.
  DCHECK(parsed->host.is_valid());
  bool characters_after_host =
      (parsed->host.end() < static_cast<int>(url_string.size()));

  if (characters_after_host) {
    // Return general elided text if fully eliding the path/query/ref fits.
    base::string16 shortest =
        url_string.substr(0, parsed->host.end()) + trailing_elision;
    if (available_pixel_width >= gfx::GetStringWidthF(shortest, font_list)) {
      if (preserve_filename && parsed->path.len > 0) {
        return ElideParsedUrlStringPath(url_string, font_list,
                                        available_pixel_width, parsed);
      }
      return ElideParsedUrlString(url_string, font_list, available_pixel_width,
                                  gfx::ELIDE_TAIL, parsed);
    }
  }

  if (parsed->host.begin > 0) {
    // Remove the scheme (anything left of the host) and update Parsed
    // accordingly.
    int original_length = url_string.size();
    int offset = parsed->host.begin;
    url_string = url_string.substr(parsed->host.begin,
                                   url_string.size() - parsed->host.begin);
    AdjustParsed(original_length, url_string.size(), offset, parsed);
    parsed->scheme.begin = 0;
    parsed->scheme.len = -1;

    // Try to fit part of the path/query/ref again, but don't treat the path
    // specially this time as we've only gained a few characters.
    if (characters_after_host) {
      base::string16 shortest =
          url_string.substr(0, parsed->host.len) + trailing_elision;
      if (available_pixel_width >= gfx::GetStringWidthF(shortest, font_list)) {
        return ElideParsedUrlString(url_string, font_list,
                                    available_pixel_width, gfx::ELIDE_TAIL,
                                    parsed);
      }
    }
  }

  // Fully elide everything after the host, as there is no chance we'll show it.
  int original_length = url_string.size();
  url_string = url_string.substr(0, parsed->host.end());
  if (characters_after_host)
    url_string += trailing_elision;
  AdjustParsed(original_length, url_string.size(), 0, parsed);

  // Only host remains, so elide from the left.
  return ElideParsedUrlString(url_string, font_list, available_pixel_width,
                              gfx::ELIDE_HEAD, parsed);
}

}  // namespace url_formatter
