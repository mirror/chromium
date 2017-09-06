// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/frame/InterventionReport.h"

namespace blink {

base::Value InterventionReport::ToValue() {
  base::Value value(base::Value::Type::DICTIONARY);
  value.SetKey("message", base::Value(message().Ascii().data()));
  value.SetKey("sourceFile", base::Value(sourceFile().Ascii().data()));
  value.SetKey("lineNumber", base::Value(static_cast<int>(lineNumber())));
  return value;
}

DEFINE_TRACE(InterventionReport) {
  ReportBody::Trace(visitor);
}

}  // namespace blink
