// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ios/chrome/browser/reading_list/offline_url_utils.h"

#include "base/logging.h"
#include "base/md5.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversions.h"
#include "components/reading_list/ios/offline_url_utils.h"
#include "ios/chrome/browser/chrome_url_constants.h"
#include "net/base/url_util.h"

namespace {
const char kVirtualURLQueryParam[] = "virtualURL";
}

namespace reading_list {

GURL DistilledURLForPath(const base::FilePath& distilled_path,
                         const GURL& virtual_url) {
  if (distilled_path.empty()) {
    return GURL();
  }
  GURL page_url(kChromeUIOfflineURL);
  GURL::Replacements replacements;
  replacements.SetPathStr(distilled_path.value());
  page_url = page_url.ReplaceComponents(replacements);
  if (virtual_url.is_valid()) {
    page_url = net::AppendQueryParameter(page_url, kVirtualURLQueryParam,
                                         virtual_url.spec());
  }
  return page_url;
}

GURL VirtualURLForDistilledURL(const GURL& distilled_url) {
  std::string virtual_url_string;
  if (net::GetValueForKeyInQuery(distilled_url, kVirtualURLQueryParam,
                                 &virtual_url_string)) {
    GURL virtual_url = GURL(virtual_url_string);
    if (virtual_url.is_valid()) {
      return virtual_url;
    }
  }
  return distilled_url;
}

GURL FileURLForDistilledURL(const GURL& distilled_url,
                            const base::FilePath& offline_path,
                            GURL* resources_root_url) {
  if (!distilled_url.is_valid()) {
    return GURL();
  }
  DCHECK(distilled_url.SchemeIs(kChromeUIScheme));
  GURL file_url(base::StringPrintf("%s%s", url::kFileScheme,
                                   url::kStandardSchemeSeparator) +
                offline_path.value() + distilled_url.path());
  if (resources_root_url) {
    *resources_root_url = file_url.Resolve(".");
  }
  return file_url;
}

bool IsOfflineURL(const GURL& url) {
  return url.SchemeIs(kChromeUIScheme) && url.host() == kChromeUIOfflineHost;
}

base::string16 StripSchemeFromOnlineURL(const base::string16& online_url,
                                        size_t* removed_chars) {
  base::string16 https_scheme = base::UTF8ToUTF16(base::StringPrintf(
      "%s%s", url::kHttpsScheme, url::kStandardSchemeSeparator));
  if (base::StartsWith(online_url, https_scheme,
                       base::CompareCase::SENSITIVE)) {
    if (removed_chars) {
      *removed_chars = https_scheme.length();
    }
    return online_url.substr(https_scheme.length());
  }
  // http:// scheme should already have been trimmed at this point.
  // DCHECK to detect formatting changes in omnibox.
  DCHECK(!base::StartsWith(
      online_url, base::UTF8ToUTF16(base::StringPrintf(
                      "%s%s", url::kHttpScheme, url::kStandardSchemeSeparator)),
      base::CompareCase::SENSITIVE));
  if (removed_chars) {
    *removed_chars = 0;
  }
  return online_url;
}
}
