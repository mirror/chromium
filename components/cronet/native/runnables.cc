// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/cronet/native/runnables.h"

namespace cronet {

void OnceClosureRunnable::Run() {
  std::move(task_).Run();
}

}  // namespace cronet
