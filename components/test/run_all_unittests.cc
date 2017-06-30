// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/test/components_test_suite.h"
#include <iostream>

int main(int argc, char** argv) {
  std::cout << "Reached Unit Test Entry Point!" << std::endl;
  return base::LaunchUnitTests(argc, argv, GetLaunchCallback(argc, argv));
}
