// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform/loader/fetch/ClientHintsPreferences.h"

#include "platform/RuntimeEnabledFeatures.h"
#include "platform/network/HTTPParsers.h"
#include "platform/weborigin/KURL.h"

namespace blink {

namespace {

// Mapping from WebClientHintsType to the header value for enabling the
// corresponding client hint. The ordering should match the ordering of enums in
// WebClientHintsType.
static constexpr const char* kHeaderMapping[] = {"device-ram", "dpr", "width",
                                                 "viewport-width"};

static_assert(kWebClientHintsTypeLast + 1 == arraysize(kHeaderMapping),
              "unhandled client hint type");

void ParseAcceptChHeader(const String& header_value,
                         bool enabled_types[kWebClientHintsTypeLast + 1]) {
  CommaDelimitedHeaderSet accept_client_hints_header;
  ParseCommaDelimitedHeader(header_value, accept_client_hints_header);

  for (size_t i = 0; i < kWebClientHintsTypeLast + 1; ++i)
    enabled_types[i] = accept_client_hints_header.Contains(kHeaderMapping[i]);

  enabled_types[kWebClientHintsTypeDeviceRam] =
      enabled_types[kWebClientHintsTypeDeviceRam] &&
      RuntimeEnabledFeatures::DeviceRAMHeaderEnabled();
}

}  // namespace

ClientHintsPreferences::ClientHintsPreferences() {}

void ClientHintsPreferences::UpdateFrom(
    const ClientHintsPreferences& preferences) {
  for (size_t i = 0; i < kWebClientHintsTypeLast + 1; ++i)
    enabled_types_[i] = preferences.enabled_types_[i];
}

void ClientHintsPreferences::UpdateFromAcceptClientHintsHeader(
    const String& header_value,
    Context* context) {
  if (!RuntimeEnabledFeatures::ClientHintsEnabled() || header_value.IsEmpty())
    return;

  bool new_enabled_types[kWebClientHintsTypeLast + 1] = {};

  ParseAcceptChHeader(header_value, new_enabled_types);

  for (size_t i = 0; i < kWebClientHintsTypeLast + 1; ++i)
    enabled_types_[i] = enabled_types_[i] || new_enabled_types[i];

  if (context) {
    for (size_t i = 0; i < kWebClientHintsTypeLast + 1; ++i) {
      if (enabled_types_[i])
        context->CountClientHints(static_cast<WebClientHintsType>(i));
    }
  }
}

void ClientHintsPreferences::UpdatePersistentHintsFromHeaders(
    const String& accept_ch_header_value,
    const String& accept_ch_lifetime_header_value,
    const KURL& url,
    Context* context,
    bool enabled_types[kWebClientHintsTypeLast + 1],
    int64_t* persist_duration_seconds) {
  *persist_duration_seconds = -1;

  if (!RuntimeEnabledFeatures::ClientHintsEnabled() ||
      !RuntimeEnabledFeatures::ClientHintsPersistentEnabled() ||
      accept_ch_header_value.IsEmpty() ||
      accept_ch_lifetime_header_value.IsEmpty()) {
    return;
  }

  if (url.Protocol() != "https") {
    // Only HTTPS domains are allowed to persist client hints.
    return;
  }

  bool conversion_ok = false;
  *persist_duration_seconds =
      accept_ch_lifetime_header_value.ToInt64Strict(&conversion_ok);
  if (!conversion_ok || *persist_duration_seconds < 0) {
    *persist_duration_seconds = -1;
    return;
  }

  if (context)
    context->CountPersistentClientHintHeaders();

  ParseAcceptChHeader(accept_ch_header_value, enabled_types);
}

}  // namespace blink
