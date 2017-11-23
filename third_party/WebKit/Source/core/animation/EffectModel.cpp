// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/animation/EffectModel.h"

namespace blink {
bool EffectModel::StringToCompositeOperation(String op,
                                             CompositeOperation& result) {
  if (op == "add") {
    result = kCompositeAdd;
    return true;
  }
  if (op == "replace") {
    result = kCompositeReplace;
    return true;
  }
  return false;
}

String EffectModel::CompositeOperationToString(CompositeOperation op) {
  switch (op) {
    case EffectModel::kCompositeAdd:
      return "add";
    case EffectModel::kCompositeReplace:
      return "replace";
    default:
      NOTREACHED();
      return "";
  }
}
}  // namespace blink
