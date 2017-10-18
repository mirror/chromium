// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_SHAPE_DETECTION_DETECTION_UTILS_WIN_H_
#define SERVICES_SHAPE_DETECTION_DETECTION_UTILS_WIN_H_

#include <comdef.h>
#include <iomanip>

namespace shape_detection {

// Helpers for printing HRESULTs.
struct PrintHr {
  PrintHr(HRESULT hr) : hr(hr) {}
  HRESULT hr;
};

std::ostream& operator<<(std::ostream& os, const PrintHr& phr) {
  std::ios_base::fmtflags ff = os.flags();
  os << _com_error(phr.hr).ErrorMessage() << " (0x" << std::hex
     << std::uppercase << std::setfill('0') << std::setw(8) << phr.hr << ")";
  os.flags(ff);
  return os;
}

}  // namespace shape_detection

#endif  // SERVICES_SHAPE_DETECTION_DETECTION_UTILS_WIN_H_