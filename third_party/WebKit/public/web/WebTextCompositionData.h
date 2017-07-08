// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WebTextCompositionData_h
#define WebTextCompositionData_h

#include <vector>

#include "public/web/WebCompositionUnderline.h"

namespace blink {

struct WebTextCompositionData {
  WebTextCompositionData() = default;

  explicit WebTextCompositionData(const WebVector<WebCompositionUnderline>& cu)
      : composition_underlines(cu.begin(), cu.end()) {}

  // TODO: use WebVector here?
  std::vector<WebCompositionUnderline> composition_underlines;
};

}  // namespace blink

#endif
