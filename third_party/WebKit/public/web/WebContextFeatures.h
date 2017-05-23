// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WebContextFeatures_h
#define WebContextFeatures_h

#include "public/platform/WebCommon.h"
#include "v8/include/v8.h"

namespace blink {

class WebContextFeatures {
 public:
  BLINK_EXPORT static void EnableMojoJS(v8::Local<v8::Context>, bool);

 private:
  WebContextFeatures();
};

}  // namespace blink

#endif  // WebContextFeatures_h
