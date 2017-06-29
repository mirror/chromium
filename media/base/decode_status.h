// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_BASE_DECODE_STATUS_H_
#define MEDIA_BASE_DECODE_STATUS_H_

#include <iosfwd>

#include "media/base/media_export.h"

namespace media {

enum class DecodeStatus {
  kOk = 0,   // Everything went as planned.
  kAborted,  // Read was aborted due to a Reset() during a pending read.
  kError,    // The decoder encountered an error.
  kDecodeStatusMax = kError,
};

// Writes the name of a DecodeStatus to a stream.
MEDIA_EXPORT std::ostream& operator<<(std::ostream& os,
                                      const DecodeStatus& status);

}  // namespace media

#endif  // MEDIA_BASE_DECODE_STATUS_H_
