// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/viz/common/surfaces/local_surface_id.h"

#include "base/strings/stringprintf.h"

namespace viz {

std::string LocalSurfaceId::ToString() const {
  std::string nonce = VLOG_IS_ON(1)
                    ? nonce_.ToString()
                    : ToShortString(nonce_.GetHighForSerialization(),
                                    nonce_.GetLowForSerialization());

  return base::StringPrintf("LocalSurfaceId(%d, %d, %s" PRIu64 ")",
                            parent_sequence_number_, child_sequence_number_,
                            nonce.c_str());
}

std::string LocalSurfaceId::ToShortString(uint64_t high, uint64_t low) const {
  return (high > UINT32_MAX && low > UINT32_MAX)
             ? nonce_.ToString()
             : base::StringPrintf("%04" PRIX64 "..%04" PRIX64, high, low);
}

std::ostream& operator<<(std::ostream& out,
                         const LocalSurfaceId& local_surface_id) {
  return out << local_surface_id.ToString();
}

}  // namespace viz
