// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/test/ash_test_suite.h"
#include "base/bind.h"
#include "base/path_service.h"
#include "base/test/launcher/unit_test_launcher.h"
#include "base/test/test_discardable_memory_allocator.h"
#include "base/test/test_suite.h"
#include "mojo/edk/embedder/embedder.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/base/ui_base_paths.h"
#include "ui/gl/test/gl_surface_test_support.h"

namespace {

class UiArcTestSuite : public ash::AshTestSuite {
 public:
  UiArcTestSuite(int argc, char** argv) : ash::AshTestSuite(argc, argv) {}
  ~UiArcTestSuite() override = default;

 private:
  DISALLOW_COPY_AND_ASSIGN(UiArcTestSuite);
};

}  // namespace

int main(int argc, char** argv) {
  UiArcTestSuite test_suite(argc, argv);

  mojo::edk::Init();
  return base::LaunchUnitTests(
      argc, argv,
      base::Bind(&UiArcTestSuite::Run, base::Unretained(&test_suite)));
}
