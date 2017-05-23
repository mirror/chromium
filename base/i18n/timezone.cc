// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/i18n/timezone.h"

#include <memory>
#include <string>

#include "third_party/icu/source/i18n/unicode/timezone.h"

namespace base {

std::string CountryCodeForCurrentTimezone() {
  std::unique_ptr<icu::TimeZone> zone(icu::TimeZone::createDefault());
  icu::UnicodeString id;
  char region_code[4];
  UErrorCode status = U_ZERO_ERROR;
  int length = zone->getRegion(zone->getID(id), region_code, 4, status);
  return U_SUCCESS(status)
             ? std::string(region_code, static_cast<size_t>(length))
             : std::string();
}

}  // namespace base
