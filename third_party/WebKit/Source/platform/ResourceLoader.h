// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ResourceLoader_h
#define ResourceLoader_h

#include "platform/PlatformExport.h"
#include "platform/wtf/text/WTFString.h"

namespace blink {

class ResourceLoader {
 public:
  static PLATFORM_EXPORT String GetResourceAsString(int);
};

}  // namespace blink

#endif  // ResourceLoader_h
