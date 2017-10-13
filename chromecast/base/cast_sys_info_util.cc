// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromecast/base/cast_sys_info_util.h"

#include "base/strings/string_util.h"

namespace chromecast {

std::string GetDeviceModelId(const std::string& manufacturer,
                             const std::string& product_name,
                             const std::string& device_model) {
  return base::JoinString({manufacturer, product_name, device_model}, ".");
}

}  // namespace chromecast
