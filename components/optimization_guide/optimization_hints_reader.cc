// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/optimization_guide/optimization_hints_reader.h"

#include "base/files/file.h"

namespace optimization_guide {

UnindexedHintsInfo::UnindexedHintsInfo(const base::Version& hints_version,
                                       const base::FilePath& hints_path)
    : hints_version(hints_version),
      hints_path(hints_path) {}

UnindexedHintsInfo::~UnindexedHintsInfo() {}

OptimizationHintsReader::OptimizationHintsReader() {}

OptimizationHintsReader::~OptimizationHintsReader() {}

bool OptimizationHintsReader::ReadHints(
    const UnindexedHintsInfo& unindexed_hints_info) {
  if (!unindexed_hints_info.hints_version.IsValid()) {
    return false;
  }
  base::File unindexed_hints_file(
      unindexed_hints_info.hints_path,
      base::File::FLAG_OPEN | base::File::FLAG_READ);
  if (!unindexed_hints_file.IsValid()) {
    VLOG(1) << "Failed opening unindexed hints.";
    return false;
  }
  // TODO(crbug.com/777892): Actually index hints.
  return true;
}

}  // namespace optimization_guide
