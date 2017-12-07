// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/profile_resetter/triggered_profile_resetter.h"

void TriggeredProfileResetter::Activate() {
#if DCHECK_IS_ON()
  activate_called_ = true;
#endif  // DCHECK_IS_ON()
}
