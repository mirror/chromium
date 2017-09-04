// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WebPageSuspender_h
#define WebPageSuspender_h

#include "public/platform/WebCommon.h"

#include <memory>

namespace blink {

class ScopedPageSuspender;

class WebPageSuspender final {
 public:
  explicit WebPageSuspender();
  ~WebPageSuspender();

 private:
  std::unique_ptr<ScopedPageSuspender> private_;
};

}  // namespace blink

#endif  // WebPageSuspender_h
