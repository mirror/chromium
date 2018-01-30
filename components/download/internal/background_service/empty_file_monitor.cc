// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/download/internal/background_service/empty_file_monitor.h"

namespace download {

EmptyFileMonitor::EmptyFileMonitor() = default;

EmptyFileMonitor::~EmptyFileMonitor() = default;

void EmptyFileMonitor::Initialize(const InitCallback& callback) {
  if (!callback.is_null())
    callback.Run(true /* success */);
}

void EmptyFileMonitor::DeleteUnknownFiles(
    const Model::EntryList& known_entries,
    const std::vector<DriverEntry>& known_driver_entries) {}

void EmptyFileMonitor::CleanupFilesForCompletedEntries(
    const Model::EntryList& entries,
    const base::RepeatingClosure& completion_callback) {
  if (!completion_callback.is_null())
    completion_callback.Run();
}

void EmptyFileMonitor::DeleteFiles(
    const std::set<base::FilePath>& files_to_remove,
    stats::FileCleanupReason reason) {}

void EmptyFileMonitor::HardRecover(const InitCallback& callback) {
  if (!callback.is_null())
    callback.Run(true /* success */);
}

}  // namespace download
