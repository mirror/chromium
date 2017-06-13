// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/android/timezone_utils.h"

#include <sys/system_properties.h>

#include "base/logging.h"
#include "base/strings/string16.h"
#include "base/strings/utf_string_conversions.h"

namespace base {
namespace android {

std::string GetDefaultTimeZoneId() {
  char value[PROP_VALUE_MAX + 1];
  int length = __system_property_get("persist.sys.timezone", value);
  DCHECK(length != 0);
  return std::string(value, length);
}

}  // namespace android
}  // namespace base
