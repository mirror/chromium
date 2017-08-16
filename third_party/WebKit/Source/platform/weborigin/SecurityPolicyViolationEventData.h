// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SecurityPolicyViolationEventData_h
#define SecurityPolicyViolationEventData_h

#include "platform/wtf/Vector.h"
#include "platform/wtf/text/WTFString.h"

namespace blink {

struct SecurityPolicyViolationEventData {
  String blocked_uri;
  int32_t column_number = 0;
  String disposition;
  String document_uri;
  String effective_directive;
  int32_t line_number = 0;
  String original_policy;
  String referrer;
  String sample;
  String source_file;
  uint16_t status_code = 0;
  String violated_directive;
};

using SecurityViolationEventDataContainer =
    Vector<SecurityPolicyViolationEventData>;

}  // namespace blink

#endif  // SecurityPolicyViolationEventData_h
