// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform/fonts/FontPersistent.h"

#include "platform/fonts/FontCache.h"
#include "platform/wtf/StdLibExtras.h"
#include "platform/wtf/ThreadSpecific.h"

namespace blink {

FontPersistent* FontPersistent::Get() {
  DEFINE_THREAD_SAFE_STATIC_LOCAL(ThreadSpecific<FontPersistent>,
                                  font_persistent, ());
  return font_persistent;
}

FontPersistent::FontPersistent() {}

FontCache* FontPersistent::GetFontCache() {
  return &(Get()->font_cache);
}

}  // namespace blink
