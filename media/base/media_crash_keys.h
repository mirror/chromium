// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_BASE_MEDIA_CRASH_KEYS_H_
#define MEDIA_BASE_MEDIA_CRASH_KEYS_H_

#include "media/base/media_export.h"
#include "media/media_features.h"

namespace media {
namespace crash_keys {

#if BUILDFLAG(ENABLE_LIBRARY_CDMS)
// Version of the library Content Decryption Module (CDM) loaded.
MEDIA_EXPORT extern const char kCdmVersion[];
#endif

}  // namespace crash_keys
}  // namespace media

#endif  // MEDIA_BASE_MEDIA_CRASH_KEYS_H_
