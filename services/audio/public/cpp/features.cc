// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/audio/public/cpp/features.h"

#include "build/build_config.h"

namespace audio {

const base::Feature kSystemInfoViaService{"AudioSystemInfoViaService",
                                          base::FEATURE_ENABLED_BY_DEFAULT};

}  // namespace audio
