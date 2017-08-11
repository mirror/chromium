// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <iostream>

#include "base/command_line.h"
#include "remoting/host/evaluate_capability.h"
#include "remoting/host/host_exit_codes.h"
#include "remoting/host/switches.h"

namespace {
// This function is for test purpose only. It writes some random texts to both
// stdout and stderr, and returns a random value 234.
int EvaluateTest() {
  std::cout << "In EvaluateTest(): Line 1\n"
               "In EvaluateTest(): Line 2";
  std::cerr << "In EvaluateTest(): Error Line 1\n"
               "In EvaluateTest(): Error Line 2";
  return 234;
}

// This function is for test purpose only. It triggers an assertion failure.
int EvaluateCrash() {
  NOTREACHED();
  return 0;
}

// This function is for test purpose only. It forwards the evaluation request to
// a new process and returns what it returns.
int EvaluateForward() {
  std::string output;
  int result = remoting::EvaluateCapability(
      remoting::kEvaluateForward, &output);
  std::cout << output;
  return result;
}

int EvaluateCapabilityLocally(const std::string& type) {
  if (type == remoting::kEvaluateTest) {
    return EvaluateTest();
  }
  if (type == remoting::kEvaluateCrash) {
    return EvaluateCrash();
  }
  if (type == remoting::kEvaluateForward) {
    return EvaluateForward();
  }

  return remoting::EvaluateCapabilityLocally(type);
}

}  // namespace

int main(int argc, char** argv) {
  base::CommandLine::Init(argc, argv);

  const base::CommandLine* command_line =
    base::CommandLine::ForCurrentProcess();
  if (command_line->HasSwitch(remoting::kEvaluateCapabilitySwitchName)) {
    return EvaluateCapabilityLocally(command_line->GetSwitchValueASCII(
        remoting::kEvaluateCapabilitySwitchName));
  }

  return remoting::kInvalidCommandLineExitCode;
}
