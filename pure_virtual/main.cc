// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/logging.h"
#include "pure_virtual/interface.h"

int main() {
  Interface* inter = Interface::CreateImplementation();
  delete inter;
  LOG(ERROR) << "Calling SayHello() on dead pointer " << inter;
  inter->SayHello();
  return 0;
}
