// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/test/ash_test_suite.h"
#include "base/bind.h"
#include "base/debug/debugger.h"
#include "base/process/launch.h"
#include "base/test/launcher/unit_test_launcher.h"
#include "build/build_config.h"

#if !defined(OS_IOS)
#include "mojo/edk/embedder/embedder.h"
#endif

namespace exo {
namespace {

class ExoPerfTestSuite : public ash::AshTestSuite {
 public:
  ExoPerfTestSuite(int argc, char** argv) : ash::AshTestSuite(argc, argv) {}

  // Overriden from ash::AshTestSuite:
  void Initialize() override {
    ash::AshTestSuite::Initialize();
    if (!base::debug::BeingDebugged())
      base::RaiseProcessToHighPriority();
  }
};

}  // namespace
}  // namespace exo

int main(int argc, char** argv) {
  exo::ExoPerfTestSuite test_suite(argc, argv);

#if !defined(OS_IOS)
  mojo::edk::Init();
#endif

  return base::LaunchUnitTestsSerially(
      argc, argv,
      base::Bind(&ash::AshTestSuite::Run, base::Unretained(&test_suite)));
}
