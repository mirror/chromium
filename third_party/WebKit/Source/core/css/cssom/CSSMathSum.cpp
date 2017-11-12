// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/css/cssom/CSSMathSum.h"

namespace blink {

/* static */
CSSMathSum* CSSMathSum::Create(const HeapVector<CSSNumberish>& args,
                               ExceptionState& exception_state) {
  if (args.IsEmpty()) {
    exception_state.ThrowDOMException(kSyntaxError, "Arguments can't be empty");
    return nullptr;
  }

  CSSNumericValueVector values = CSSNumberishesToNumericValues(args);

  bool error = false;
  CSSNumericValueType final_type =
      CSSMathVariadic::TypeCheck(values, CSSNumericValueType::Add, error);
  if (error) {
    exception_state.ThrowTypeError("Incompatible types");
    return nullptr;
  }

  return new CSSMathSum(CSSNumericArray::Create(std::move(values)), final_type);
}

}  // namespace blink
