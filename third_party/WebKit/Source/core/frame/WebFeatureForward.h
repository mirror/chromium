// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WebFeatureForward_h
#define WebFeatureForward_h

#include <cstdint>

namespace blink {

namespace mojom {
// This is a heavy beast. Let's forward-declare it.
enum class WebFeature : int32_t;
}  // namespace mojom
using WebFeature = mojom::WebFeature;

}  // namespace blink

#endif  // WebFeatureForward_h
