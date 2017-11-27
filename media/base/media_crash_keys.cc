// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/base/media_crash_keys.h"

namespace media {
namespace crash_keys {

#if BUILDFLAG(ENABLE_LIBRARY_CDMS)
const char kCdmVersion[] = "cdm-version";
#endif

}  // namespace crash_keys
}  // namespace media
