// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/log_util.h"

#include "base/logging.h"
#include "base/macros.h"
#include "base/strings/string_piece.h"

namespace logging {

ScopedLogDuration::ScopedLogDuration(base::StringPiece message)
    : message_(message), start_time_(base::TimeTicks::Now()) {
  LOG(INFO) << "Starting " << message_ << " [this=" << this << "]";
}

ScopedLogDuration::~ScopedLogDuration() {
  LOG(INFO) << "Ending " << message_ << " [this=" << this << "] "
            << " [dt="
            << (base::TimeTicks::Now() - start_time_).InMillisecondsF() << " ms]";
}

}  // namespace logging
