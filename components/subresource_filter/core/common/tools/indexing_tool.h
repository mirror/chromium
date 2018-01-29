// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_SUBRESOURCE_FILTER_CORE_COMMON_TOOLS_INDEXING_TOOL_H_
#define COMPONENTS_SUBRESOURCE_FILTER_CORE_COMMON_TOOLS_INDEXING_TOOL_H_

#include "base/files/file_path.h"

namespace subresource_filter {

// Given |unindexed_path| which is a path to an unindexed rulest, writes
// the indexed (flatbuffer) version to |indexed_path|.
void ConvertToIndexedRuleset(const base::FilePath& unindexed_path,
                             const base::FilePath& indexed_path);

}  // namespace subresource_filter

#endif  // COMPONENTS_SUBRESOURCE_FILTER_CORE_COMMON_TOOLS_INDEXING_TOOL_H_
