// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// This file contains the archive file analysis implementation for download
// protection, which runs in a sandboxed utility process.

#include "chrome/services/file_util/public/cpp/archive_analyzer_results.h"

namespace chrome {

ArchiveAnalyzerResults::ArchiveAnalyzerResults() = default;

ArchiveAnalyzerResults::ArchiveAnalyzerResults(
    const ArchiveAnalyzerResults& other) = default;

ArchiveAnalyzerResults::~ArchiveAnalyzerResults() {}

}  // namespace chrome
