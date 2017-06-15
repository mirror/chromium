// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/offline_pages/core/prefetch/prefetch_types.h"

namespace offline_pages {

RenderPageInfo::RenderPageInfo() {}

RenderPageInfo::RenderPageInfo(const RenderPageInfo& other) = default;

bool PrefetchURL::operator<(const PrefetchURL& other) const {
  if (client_id == other.client_id)
    return url < other.url;

  return client_id < other.client_id;
}

}  // namespace offline_pages
