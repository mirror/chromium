// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_LINUX_MPRIS_CLIENT_H_
#define CHROME_BROWSER_LINUX_MPRIS_CLIENT_H_

#include "base/macros.h"

// A D-Bus client conforming to the MPRIS spec:
// https://specifications.freedesktop.org/mpris-spec/latest/
// It is current under development (https://crbug.com/804121)
class MprisClient {
 public:
  MprisClient();
  ~MprisClient();

 private:
  DISALLOW_COPY_AND_ASSIGN(MprisClient);
};

#endif  // CHROME_BROWSER_LINUX_MPRIS_CLIENT_H_
