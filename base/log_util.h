// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BASE_LOG_UTIL_H_
#define BASE_LOG_UTIL_H_

#include <string>

#include "base/base_export.h"
#include "base/macros.h"
#include "base/strings/string_piece_forward.h"
#include "base/time/time.h"

namespace logging {

class BASE_EXPORT ScopedLogDuration {
 public:
  explicit ScopedLogDuration(base::StringPiece message);
  ~ScopedLogDuration();

 private:
  std::string message_;
  base::TimeTicks start_time_;

  DISALLOW_COPY_AND_ASSIGN(ScopedLogDuration);
};

} // namespace logging

#endif  // BASE_LOG_UTIL_H_
