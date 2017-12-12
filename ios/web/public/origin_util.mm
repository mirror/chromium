// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/web/public/origin_util.h"

#import <WebKit/WebKit.h>

#include "base/numerics/safe_conversions.h"
#include "base/stl_util.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/sys_string_conversions.h"
#include "net/base/url_util.h"
#include "url/gurl.h"
#include "url/scheme_host_port.h"
#include "url/url_util.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace web {

bool IsOriginSecure(const GURL& url) {
  if (url.SchemeIsCryptographic() || url.SchemeIsFile())
    return true;

  if (url.SchemeIsFileSystem() && url.inner_url() &&
      IsOriginSecure(*url.inner_url())) {
    return true;
  }

  if (base::ContainsValue(url::GetSecureSchemes(), url.scheme()))
    return true;

  if (net::IsLocalhost(url.HostNoBracketsPiece()))
    return true;

  return false;
}

GURL GURLOriginWithWKSecurityOrigin(WKSecurityOrigin* origin) {
  if (!origin)
    return GURL();

  GURL::Replacements rep;
  rep.SetSchemeStr(base::SysNSStringToUTF8(origin.protocol));
 // rep.SetHostStr(base::SysNSStringToUTF8(origin.host));
  if (origin.port) {
    uint16_t port = base::checked_cast<uint16_t>(origin.port);
    rep.SetPortStr(base::UintToString(port));
  }
  return GURL(base::SysNSStringToUTF8(origin.host)).ReplaceComponents(rep);
}

}  // namespace web
