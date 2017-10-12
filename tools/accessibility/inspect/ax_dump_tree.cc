// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <string>

#include "base/at_exit.h"
#include "base/command_line.h"
#include "base/strings/string_number_conversions.h"
#include "tools/accessibility/inspect/ax_tree_server.h"

char kPidSwitch[] = "pid";
char kWindowSwitch[] = "window";
char kOptsSwitch[] = "opts";

// Convert whether in 0x hex format or decimal.
bool StringToInt(std::string str, int* result) {
  bool is_hex = str[0] == '0' && str[1] == 'x';
  return is_hex ? base::HexStringToInt(str, result)
                : base::StringToInt(str, result);
}

int main(int argc, char** argv) {
  base::AtExitManager at_exit_manager;

  base::CommandLine::Init(argc, argv);

  base::string16 opts_path = base::ASCIIToUTF16(
      base::CommandLine::ForCurrentProcess()->GetSwitchValueASCII(kOptsSwitch));

  std::string window_str =
      base::CommandLine::ForCurrentProcess()->GetSwitchValueASCII(
          kWindowSwitch);
  if (!window_str.empty()) {
    int window;
    if (StringToInt(window_str, &window)) {
      printf("Tree for window: %s\n", window_str.c_str());
      gfx::AcceleratedWidget widget(reinterpret_cast<HWND>(window));
      std::unique_ptr<content::AXTreeServer> server(
          new content::AXTreeServer(widget, opts_path));
      return 0;
    }
  }
  std::string pid_str =
      base::CommandLine::ForCurrentProcess()->GetSwitchValueASCII(kPidSwitch);
  if (!pid_str.empty()) {
    int pid;
    if (StringToInt(pid_str, &pid)) {
      printf("Tree for process id: %s\n", pid_str.c_str());
      base::ProcessId process_id = static_cast<base::ProcessId>(pid);
      std::unique_ptr<content::AXTreeServer> server(
          new content::AXTreeServer(process_id, opts_path));
    }
  }
  return 0;
}
