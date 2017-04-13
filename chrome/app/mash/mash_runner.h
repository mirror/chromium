// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_APP_MASH_MASH_RUNNER_H_
#define CHROME_APP_MASH_MASH_RUNNER_H_

#include <memory>

#include "base/macros.h"
#include "services/service_manager/public/interfaces/service.mojom.h"

// Called during chrome --mash startup instead of ContentMain().
int MashMain();

#endif  // CHROME_APP_MASH_MASH_RUNNER_H_
