// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/offline_pages/core/prefetch/test_prefetch_importer.h"

namespace offline_pages {

TestPrefetchImporter::TestPrefetchImporter() : PrefetchImporter(nullptr) {}

TestPrefetchImporter::~TestPrefetchImporter() = default;

void TestPrefetchImporter::ImportArchive(const PrefetchArchiveInfo& archive) {}

std::list<int64_t> TestPrefetchImporter::GetOngoingImports() const {
  return std::list<int64_t>();
}

}  // namespace offline_pages
