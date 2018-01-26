// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/quic/platform/impl/quic_url_utils_impl.h"

#include "url/gurl.h"

using std::string;

namespace net {

// static
string QuicUrlUtilsImpl::HostName(QuicStringPiece url) {
  return GURL(url).host();
}

// static
bool QuicUrlUtilsImpl::IsValidUrl(QuicStringPiece url) {
  return GURL(url).is_valid();
}

// static
std::string QuicUrlUtilsImpl::CanonicalizeUrl(QuicStringPiece url) {
  url::Parsed parsed;
  url::ParseStandardURL(url.data(), url.size(), &parsed);

  std::string canon_url;
  url::StdStringCanonOutput canon_output(&canon_url);
  url::Parsed output_parsed;
  bool success = url::CanonicalizeStandardURL(
      url.data(), url.size(), parsed, nullptr, &canon_output, &output_parsed);
  if (!success) {
    return std::string();
  }
  canon_output.Complete();
  return canon_url;
}

}  // namespace net
