// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_OPTIMIZATION_GUIDE_OPTIMIZATION_HINTS_READER_H_
#define COMPONENTS_OPTIMIZATION_GUIDE_OPTIMIZATION_HINTS_READER_H_

#include <string>

#include "base/files/file_path.h"
#include "base/macros.h"
#include "base/version.h"

namespace optimization_guide {

// Encapsulates information about a version of unindexed hints on disk.
struct UnindexedHintsInfo {
  UnindexedHintsInfo(
      const base::Version& hints_version, const base::FilePath& hints_path);
  ~UnindexedHintsInfo();

  // The version of the hints content.
  const base::Version hints_version;

  // The path to the file containing the unindexed hints.
  const base::FilePath hints_path;
};

// Reads and indexes the hints downloaded from the Component Updater as part of
// the Optimization Hints component.
class OptimizationHintsReader {
 public:
  OptimizationHintsReader();

  virtual ~OptimizationHintsReader();

  // Reads and indexes hints from the given unindexed hints, unless its
  // |hints_version| matches that of the most recently parsed version, in which
  // case it does nothing.
  //
  // Should always be run on a background priority task, as it does File I/O.
  bool ReadHints(const UnindexedHintsInfo& unindexed_hints_info);

 private:
  DISALLOW_COPY_AND_ASSIGN(OptimizationHintsReader);
};

}  // namespace optimization_guide

#endif  // COMPONENTS_OPTIMIZATION_GUIDE_OPTIMIZATION_HINTS_READER_H_
