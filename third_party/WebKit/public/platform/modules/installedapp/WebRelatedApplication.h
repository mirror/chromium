// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WebRelatedApplication_h
#define WebRelatedApplication_h

#include "public/platform/WebString.h"
#include "public/platform/WebURL.h"

namespace blink {

struct WebRelatedApplication {
  WebRelatedApplication() {}

  WebString platform;
  WebURL url;
  WebString id;
};

}  // namespace blink

#endif  // WebRelatedApplication_h
