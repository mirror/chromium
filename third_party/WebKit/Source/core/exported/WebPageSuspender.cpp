// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "public/web/WebPageSuspender.h"

#include "core/page/ScopedPageSuspender.h"

namespace blink {

WebPageSuspender::WebPageSuspender() : private_(new ScopedPageSuspender) {}

WebPageSuspender::~WebPageSuspender() = default;

}  // namespace blink
