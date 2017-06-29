// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/base/decode_status.h"

#include <ostream>

namespace media {

std::ostream& operator<<(std::ostream& os, const DecodeStatus& status) {
  switch (status) {
    case DecodeStatus::kOk:
      os << "DecodeStatus::kOk";
      break;
    case DecodeStatus::kAborted:
      os << "DecodeStatus::kAborted";
      break;
    case DecodeStatus::kError:
      os << "DecodeStatus::kError";
      break;
  }
  return os;
}

}  // namespace media
