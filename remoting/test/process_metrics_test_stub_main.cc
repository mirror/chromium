// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/threading/platform_thread.h"
#include "base/time/time.h"

int main() {
  base::PlatformThread::Sleep(base::TimeDelta::Max());
}
