// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMECAST_TRACING_FTRACE_H_
#define CHROMECAST_TRACING_FTRACE_H_

#include <string>
#include <vector>

#include "base/files/scoped_file.h"
#include "base/strings/string_piece.h"

namespace chromecast {
namespace tracing {

// Returns true if |category| is valid for system tracing.
bool IsValidCategory(base::StringPiece category);

// Starts ftrace for the specified categories.
//
// This must be paired with StopFtrace() or the kernel will continue
// tracing indefinitely. Returns true if successful.
bool StartFtrace(const std::vector<std::string>& categories);

// Writes time synchronization marker.
//
// This is used by trace-viewer to align ftrace clock with userspace
// tracing. Since CLOCK_MONOTONIC is used in both cases, this merely
// writes a zero offset. Call it at the end of tracing right before
// StopFtrace(). Returns true if successful.
bool WriteFtraceTimeSyncMarker();

// Stops ftrace.
//
// If tracing isn't started, this call has no effect. Returns true if
// successful.
bool StopFtrace();

// Opens ftrace buffer for reading.
base::ScopedFD GetFtraceData();

// Clears ftrace buffer. Returns true if successful.
bool ClearFtrace();

}  // namespace tracing
}  // namespace chromecast

#endif  // CHROMECAST_TRACING_FTRACE_H_
