// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_PAYMENTS_CORE_UTIL_JSON_UTIL_H_
#define COMPONENTS_PAYMENTS_CORE_UTIL_JSON_UTIL_H_

#include <memory>
#include <string>

#include "base/values.h"

namespace payments {

std::unique_ptr<base::Value> ReadJSONStringToValue(const std::string& json);

}  // namespace payments

#endif  // COMPONENTS_PAYMENTS_CORE_UTIL_JSON_UTIL_H_
