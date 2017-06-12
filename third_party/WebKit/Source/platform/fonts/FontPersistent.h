// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef FontPersistent_h
#define FontPersistent_h

#include "platform/PlatformExport.h"
#include "platform/fonts/FontCache.h"
#include "platform/wtf/StdLibExtras.h"

namespace blink {

class PLATFORM_EXPORT FontPersistent {
  WTF_MAKE_NONCOPYABLE(FontPersistent);

 public:
  static FontPersistent* Get();

  static FontCache* GetFontCache();

 private:
  friend class WTF::ThreadSpecific<FontPersistent>;

  FontPersistent();

  FontCache font_cache;
};

}  // namespace blink

#endif
