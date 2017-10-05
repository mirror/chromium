// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/payments/core/util/json_util.h"

#include "base/json/json_reader.h"

namespace payments {

std::unique_ptr<base::Value> ReadJSONStringToValue(const std::string& json) {
  return base::JSONReader().ReadToValue(json);
}

}  // namespace payments
