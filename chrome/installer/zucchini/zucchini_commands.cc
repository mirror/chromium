// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stddef.h>

#include "chrome/installer/zucchini/zucchini_commands.h"

#include "base/logging.h"

namespace {

// TODO(huangs): File I/O utilities.

}  // namespace

namespace command {

zucchini::status::Code Gen(const base::CommandLine& command_line,
                           const std::vector<base::FilePath>& fnames) {
  CHECK_EQ(static_cast<size_t>(kGenNumFileParams), fnames.size());
  // TODO(etiennep): Implement.
  return zucchini::status::kStatusSuccess;
}

zucchini::status::Code Apply(const base::CommandLine& command_line,
                             const std::vector<base::FilePath>& fnames) {
  CHECK_EQ(static_cast<size_t>(kApplyNumFileParams), fnames.size());
  // TODO(etiennep): Implement.
  return zucchini::status::kStatusSuccess;
}

}  // namespace command
