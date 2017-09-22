// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/at_exit.h"
#include "base/command_line.h"
#include "base/run_loop.h"
#include "base/test/scoped_task_environment.h"
#include "base/win/scoped_com_initializer.h"
#include "content/browser/accessibility/accessibility_event_recorder.h"

namespace content {

void DumpEvents() {
  base::test::ScopedTaskEnvironment scoped_task_environment(
      base::test::ScopedTaskEnvironment::MainThreadType::UI);
  base::win::ScopedCOMInitializer com_initializer;

  printf("Listening for events\n");
  std::unique_ptr<AccessibilityEventRecorder>(
      AccessibilityEventRecorder::Create(nullptr));
  base::RunLoop run_loop;
  run_loop.Run();
}

}  // namespace content

int main(int argc, char** argv) {
  base::AtExitManager at_exit_manager;
  base::CommandLine::Init(argc, argv);

  content::DumpEvents();
  return 0;
}
