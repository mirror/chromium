// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PURE_VIRTUAL_INTERFACE_
#define PURE_VIRTUAL_INTERFACE_

#include <stddef.h>

struct Interface {
  virtual ~Interface();
  virtual void SayHello() = 0;

  static void OnDestroyed(Interface*);

  static Interface* CreateImplementation();
};

inline Interface::~Interface() {
  OnDestroyed(this);
}

#endif  // PURE_VIRTUAL_INTERFACE_
