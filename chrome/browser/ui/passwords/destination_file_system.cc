// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/passwords/destination_file_system.h"

#include <utility>

#include "base/files/file_util.h"

DestinationFileSystem::DestinationFileSystem(base::FilePath destination_file)
    : destination_file_(std::move(destination_file)) {}

bool DestinationFileSystem::Write(const std::string& data) {
  return base::WriteFile(destination_file_, data.c_str(), data.size());
}
