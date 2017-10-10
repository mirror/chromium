#include "pure_virtual/interface.h"

#include "base/logging.h"
// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

struct Implementation : Interface {
  void SayHello() override { LOG(ERROR) << "Hello!"; }

  // To make memory reuse less likely
  char data[777];
};

Interface* Interface::CreateImplementation() {
  return new Implementation();
}

void Interface::OnDestroyed(Interface* inter) {
  LOG(ERROR) << inter << " is now dead.";
}
