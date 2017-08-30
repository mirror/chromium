// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/frame/DeprecationReport.h"

#include <memory>

namespace blink {

std::unique_ptr<base::Value> DeprecationReport::ToValue() {
  auto value = std::make_unique<base::DictionaryValue>();
  value->SetString("message", message().Ascii().data());
  value->SetString("sourceFile", sourceFile().Ascii().data());
  value->SetInteger("lineNumber", lineNumber());
  return value;
}

DEFINE_TRACE(DeprecationReport) {
  ReportBody::Trace(visitor);
}

}  // namespace blink
