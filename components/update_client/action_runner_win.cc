// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/update_client/action_runner.h"

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/command_line.h"
#include "base/location.h"
#include "base/sequenced_task_runner.h"

namespace update_client {

void ActionRunner::RunCommand(const base::CommandLine& command_line) {
  DCHECK(task_runner_->RunsTasksInCurrentSequence());
}

base::CommandLine ActionRunner::MakeCommandLine(
    const base::FilePath& unpack_path) const {
  base::CommandLine command_line(unpack_path);
  return command_line;
}

}  // namespace update_client
