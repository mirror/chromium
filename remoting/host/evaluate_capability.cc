// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "remoting/host/evaluate_capability.h"

#include <iostream>

#include "base/base_paths.h"
#include "base/command_line.h"
#include "base/files/file_path.h"
#include "base/logging.h"
#include "base/path_service.h"
#include "base/process/kill.h"
#include "base/process/launch.h"
#include "build/build_config.h"
#include "remoting/host/host_exit_codes.h"
#include "remoting/host/ipc_constants.h"
#include "remoting/host/switches.h"

namespace remoting {

namespace {

// This function is for test purpose only. It writes some random texts to both
// stdout and stderr, and returns a random value 123.
int EvaluateTest() {
  std::cout << "In EvaluateTest(): Line 1\n"
               "In EvaluateTest(): Line 2";
  std::cerr << "In EvaluateTest(): Error Line 1\n"
               "In EvaluateTest(): Error Line 2";
  return 123;
}

// This function is for test purpose only. It triggers an assertion failure.
int EvaluateCrash() {
  NOTREACHED();
  return 0;
}

int EvaluateCapabilityAgainst(const base::FilePath& path,
                              const std::string& type,
                              std::string* output) {
  base::CommandLine command(path);
  command.AppendSwitchASCII(kProcessTypeSwitchName,
                            kProcessTypeEvaluateCapability);
  command.AppendSwitchASCII(kEvaluateCapabilitySwitchName, type);

  int exit_code;
  std::string dummy_output;
  if (output == nullptr) {
    output = &dummy_output;
  }

  // base::GetAppOutputWithExitCode() usually returns false when receiving
  // "unknown" exit code. But we forward the |exit_code| through return value,
  // so the return of base::GetAppOutputWithExitCode() should be ignored.
  base::GetAppOutputWithExitCode(command, output, &exit_code);
  return exit_code;
}

}  // namespace

int EvaluateCapabilityAgainstHostBinary(const std::string& type,
                                        std::string* output /* = nullptr */) {
  base::FilePath path;
  bool result = base::PathService::Get(base::DIR_EXE, &path);
  DCHECK(result);
#if defined(OS_WIN)
  path = path.Append(FILE_PATH_LITERAL("remoting_host.exe"));
#elif defined(OS_LINUX)
  path = path.Append(FILE_PATH_LITERAL("chrome-remote-desktop-host"));
#elif defined(OS_MACOSX)
  path = path.Append(FILE_PATH_LITERAL(
      "remoting_me2me_host.app/Contents/MacOS/remoting_me2me_host"));
#else
  static_assert(false,
                "GetHostBinaryFilePath is not defined for current platform.");
#endif
  return EvaluateCapabilityAgainst(path, type, output);
}

int EvaluateCapabilityLocally(const std::string& type) {
  if (type == kEvaluateTest) {
    return EvaluateTest();
  }
  if (type == kEvaluateCrash) {
    return EvaluateCrash();
  }

  return kInvalidCommandLineExitCode;
}

int EvaluateCapabilityForkedly(const std::string& type,
                               std::string* output /* = nullptr */) {
#if defined(OS_WIN)
  // On Windows, remoting_console.exe is always required to receive console
  // output and exit code.
  return EvaluateCapabilityAgainstHostBinary(type, output);
#else
  base::FilePath current_binary;
  bool result = base::PathService::Get(base::FILE_EXE, &current_binary);
  DCHECK(result);

  return EvaluateCapabilityAgainst(current_binary, type, output);
#endif
}

}  // namespace remoting
