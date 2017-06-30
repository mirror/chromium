// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ios/web/public/global_state/ios_global_state_configuration.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace ios_global_state {

base::StringPiece TaskSchedulerName() {
  return "CronetIos";
}

}  // namespace ios_global_state
