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

bool IsValidCategory(base::StringPiece category);

bool StartFtrace(std::vector<std::string> categories);

bool WriteFtraceTimeSyncMarker();

bool StopFtrace();

base::ScopedFD GetFtraceData();

bool ClearFtrace();

}  // namespace tracing
}  // namespace chromecast

#endif  // CHROMECAST_TRACING_FTRACE_H_
