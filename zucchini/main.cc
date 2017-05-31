// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/command_line.h"
#include "zucchini/main_utils.h"

int main(int argc, const char* argv[]) {
  ResourceUsageTracker tracker;
  base::CommandLine::Init(argc, argv);
  const base::CommandLine& command_line =
      *base::CommandLine::ForCurrentProcess();

  // Instantiate Command registry and register Zucchini features.
  CommandRegistry registry;
  registry.Register(&kCommandGen);
  registry.Register(&kCommandApply);
  registry.Register(&kCommandRead);
  registry.Register(&kCommandDetect);
  registry.Register(&kCommandMatch);
  registry.Register(&kCommandCrc32);

  registry.RunOrExit(command_line);

  if (!command_line.HasSwitch(kSwitchQuiet))
    tracker.Print();
  return 0;
}
