// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#ifndef AllowedByNosniff_h
#define AllowedByNosniff_h

#include "core/CoreExport.h"

namespace blink {

class CSSParserContext;
class ExecutionContext;
class ResourceResponse;

class CORE_EXPORT AllowedByNosniff {
 public:
  enum class MIMEMode { kStrict, kLax };

  static bool MimeTypeAsScript(ExecutionContext*, const ResourceResponse&);
  static bool MimeTypeAsStyle(const CSSParserContext*,
                              const ResourceResponse&,
                              MIMEMode);
};

}  // namespace blink

#endif
