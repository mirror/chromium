// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/viz/common/surfaces/local_surface_id.h"

#include "base/strings/stringprintf.h"

namespace viz {

std::string LocalSurfaceId::ToString() const {
  std::string nonce = nonce_.ToString();
  if (!VLOG_IS_ON(1) && (int)nonce.size() == 16) {
    nonce = ExtractPart(0, 7, nonce) + ".." + ExtractPart(8, 15, nonce);
  }

  return base::StringPrintf("LocalSurfaceId(%d, %d, %s)",
                            parent_sequence_number_, child_sequence_number_,
                            nonce.c_str());
}

std::string LocalSurfaceId::ExtractPart(int from,
                                        int to,
                                        const std::string& token) const {
  std::string ret;
  for (int i = to; i >= from; --i) {
    if (token[i] == '0')
      break;
    ret += token[i];
  }
  while ((int)ret.size() < 4)
    ret += '0';

  reverse(ret.begin(), ret.end());
  return ret;
}

std::ostream& operator<<(std::ostream& out,
                         const LocalSurfaceId& local_surface_id) {
  return out << local_surface_id.ToString();
}

}  // namespace viz
