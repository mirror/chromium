// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <string>

#include "base/at_exit.h"
#include "base/bind.h"
#include "base/command_line.h"
#include "base/run_loop.h"
#include "base/strings/string_number_conversions.h"
#include "base/test/scoped_task_environment.h"
#include "content/browser/accessibility/accessibility_event_recorder.h"

namespace content {

AccessibilityEventRecorder* recorder;

void OnEvent(std::string event) {
  printf("Event %s\n", event.c_str());
}

void DumpEvents(int pid) {
  printf("PID: %d\n", pid);
  base::test::ScopedTaskEnvironment scoped_task_environment(
      base::test::ScopedTaskEnvironment::MainThreadType::UI);

  recorder = AccessibilityEventRecorder::Create(nullptr, pid);
  recorder->ListenToEvents(base::BindRepeating(&OnEvent));

  // Alternatively, open a Windows dialog box and run until OK is pushed:
  // MessageBox(NULL, L"MessageBox Text", L"MessageBox caption", MB_OK);
  base::RunLoop run_loop;
  run_loop.Run();
}

void OnExit(void* instance) {
  delete recorder;
}

}  // namespace content

char kPidSwitch[] = "pid";

int main(int argc, char** argv) {
  base::AtExitManager at_exit_manager;
  // TODO(aleventhal) Want callback after Ctrl+C or some global keystroke:
  // base::AtExitManager::RegisterCallback(content::OnExit, nullptr);

  base::CommandLine::Init(argc, argv);
  std::string pid_str =
      base::CommandLine::ForCurrentProcess()->GetSwitchValueASCII(kPidSwitch);
  int pid = 0;
  if (!pid_str.empty())
    base::StringToInt(pid_str, &pid);

  content::DumpEvents(pid);
  return 0;
}
