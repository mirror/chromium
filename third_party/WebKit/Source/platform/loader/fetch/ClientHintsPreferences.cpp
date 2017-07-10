// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform/loader/fetch/ClientHintsPreferences.h"

#include "platform/RuntimeEnabledFeatures.h"
#include "platform/network/HTTPParsers.h"

namespace blink {

namespace {

void ParseAcceptChHeader(const String& header_value,
                         bool enabled_types[kWebClientHintsTypeLast + 1]) {
  CommaDelimitedHeaderSet accept_client_hints_header;
  ParseCommaDelimitedHeader(header_value, accept_client_hints_header);

  for (size_t i = 0; i < kWebClientHintsTypeLast + 1; ++i) {
    switch (i) {
      case kWebClientHintsTypeDeviceRam:
        enabled_types[i] = RuntimeEnabledFeatures::DeviceRAMHeaderEnabled() &&
                           accept_client_hints_header.Contains("device-ram");
        break;
      case kWebClientHintsTypeDpr:
        enabled_types[i] = accept_client_hints_header.Contains("dpr");
        break;
      case kWebClientHintsTypeResourceWidth:
        enabled_types[i] = accept_client_hints_header.Contains("width");
        break;
      case kWebClientHintsTypeViewportWidth:
        enabled_types[i] =
            accept_client_hints_header.Contains("viewport-width");
        break;
    }
  }
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

  ParseAcceptChHeader(header_value, enabled_types_);

  if (context) {
    for (size_t i = 0; i < kWebClientHintsTypeLast + 1; ++i) {
      if (enabled_types_[i])
        context->CountClientHints(static_cast<WebClientHintsType>(i));
    }
  }
}

}  // namespace blink
