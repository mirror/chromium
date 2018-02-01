// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/animation/EffectModel.h"

#include "bindings/core/v8/ExceptionState.h"
#include "core/animation/KeyframeEffectOptions.h"
#include "platform/runtime_enabled_features.h"

namespace blink {
EffectModel::CompositeOperation EffectModel::StringToCompositeOperation(
    String composite_string) {
  DCHECK(composite_string == "replace" || composite_string == "add" ||
         composite_string == "accumulate");
  // We have to be careful to keep backwards compatible behavior here; no valid
  // value of CompositeOperation should throw, even if it is unsupported in
  // Chrome currently. Values we do not support should just be turned into
  // kCompositeReplace.
  //
  // See http://crbug.com/806139.
  if (RuntimeEnabledFeatures::CSSAdditiveAnimationsEnabled() &&
      composite_string == "add") {
    return kCompositeAdd;
  }
  return kCompositeReplace;
}

String EffectModel::CompositeOperationToString(CompositeOperation composite) {
  switch (composite) {
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
