// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform/loader/fetch/ClientHintsPreferences.h"

#include "platform/RuntimeEnabledFeatures.h"
#include "platform/network/HTTPParsers.h"

namespace blink {

namespace {

void ParseAcceptChHeader(const String& header_value,
                         bool enabled_types[kWebClientHintsTypeNumValues]) {
  CommaDelimitedHeaderSet accept_client_hints_header;
  ParseCommaDelimitedHeader(header_value, accept_client_hints_header);

  // change to switch-case.

  enabled_types[kWebClientHintsTypeDeviceRam] =
      RuntimeEnabledFeatures::DeviceRAMHeaderEnabled() &&
      accept_client_hints_header.Contains("device-ram");

  enabled_types[kWebClientHintsTypeDPR] =
      accept_client_hints_header.Contains("dpr");

  enabled_types[kWebClientHintsTypeResourceWidth] =
      accept_client_hints_header.Contains("width");

  enabled_types[kWebClientHintsTypeViewportWidth] =
      accept_client_hints_header.Contains("viewport-width");
}

}  // namespace

ClientHintsPreferences::ClientHintsPreferences() : enabled_types_{false} {}

void ClientHintsPreferences::UpdateFrom(
    const ClientHintsPreferences& preferences) {
  for (size_t i = 0; i < kWebClientHintsTypeNumValues; ++i)
    enabled_types_[i] = preferences.enabled_types_[i];
}

void ClientHintsPreferences::UpdateFromAcceptClientHintsHeader(
    const String& header_value,
    Context* context) {
  if (!RuntimeEnabledFeatures::ClientHintsEnabled() || header_value.IsEmpty())
    return;

  ParseAcceptChHeader(header_value, enabled_types_);

  // change to switch-case.

  if (enabled_types_[kWebClientHintsTypeDeviceRam]) {
    if (context)
      context->CountClientHintsDeviceRAM();
  }

  if (enabled_types_[kWebClientHintsTypeDPR]) {
    if (context)
      context->CountClientHintsDPR();
  }

  if (enabled_types_[kWebClientHintsTypeResourceWidth]) {
    if (context)
      context->CountClientHintsResourceWidth();
  }

  if (enabled_types_[kWebClientHintsTypeViewportWidth]) {
    if (context)
      context->CountClientHintsViewportWidth();
  }
}

void ClientHintsPreferences::UpdatePersistentFromAcceptClientHintsHeader(
    const String& accept_ch_header_value,
    const String& accept_ch_lifetime_header_value,
    const KURL& url,
    bool enabled_types[kWebClientHintsTypeNumValues],
    int64_t* persist_duration_seconds) {
  *persist_duration_seconds = -1;
  /*
  LOG(WARNING) << "xxx UpdatePersistentFromAcceptClientHintsHeader cp0 url="
               << url.GetString();
               */

  if (!RuntimeEnabledFeatures::ClientHintsEnabled() ||
      accept_ch_header_value.IsEmpty() ||
      accept_ch_lifetime_header_value.IsEmpty() ||
      !RuntimeEnabledFeatures::ClientHintsPersistenceEnabled()) {
    return;
  }

  bool conversion_ok = false;
  *persist_duration_seconds =
      accept_ch_lifetime_header_value.ToInt64Strict(&conversion_ok);
  if (!conversion_ok) {
    *persist_duration_seconds = -1;
    return;
  }

  ParseAcceptChHeader(accept_ch_header_value, enabled_types);

  /*
    if (url.GetString().Find("nytimes.com") != kNotFound ||
        url.GetString().Find("nyt.com") != kNotFound ||
        url.GetString().Find("check.googlezip.net") != kNotFound) {


      LOG(WARNING) << "xxx UpdatePersistentFromAcceptClientHintsHeader setting "
                      "pref cp1 url="
                   << url.GetString();

      *persist_duration_seconds = 600;
      enabled_types[kWebClientHintsTypeDPR] = true;
    }
    */

  /*
    LOG(WARNING) << "xxx "
                    "ClientHintsPreferences::"
                    "UpdatePersistentFromAcceptClientHintsHeader url="
                 << url.GetString();
                 */
}

}  // namespace blink
