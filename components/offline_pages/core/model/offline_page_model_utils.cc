// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/offline_pages/core/model/offline_page_model_utils.h"

#include <string>

#include "base/logging.h"
#include "components/offline_pages/core/offline_page_item.h"

namespace offline_pages {

namespace model_utils {

// TODO(carlosk): Canonicalize this suffix adding logic which is already
// duplicated in many different places around the codebase.
std::string AddHistogramSuffix(const ClientId& client_id,
                               const char* histogram_name) {
  if (client_id.name_space.empty()) {
    NOTREACHED();
    return histogram_name;
  }
  std::string adjusted_histogram_name(histogram_name);
  adjusted_histogram_name += ".";
  adjusted_histogram_name += client_id.name_space;
  return adjusted_histogram_name;
}

}  // namespace model_utils

}  // namespace offline_pages
