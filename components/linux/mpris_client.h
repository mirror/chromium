// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_LINUX_MPRIS_CLIENT_H_
#define COMPONENTS_LINUX_MPRIS_CLIENT_H_

#include "base/macros.h"
#include "base/memory/scoped_refptr.h"

namespace dbus {
class Bus;
}

// A D-Bus client conforming to the MPRIS spec:
// https://specifications.freedesktop.org/mpris-spec/latest/
// It is currently under development (https://crbug.com/804121)
class MprisClient {
 public:
  MprisClient();
  ~MprisClient();

 private:
  DISALLOW_COPY_AND_ASSIGN(MprisClient);
};

#endif  // COMPONENTS_LINUX_MPRIS_CLIENT_H_
