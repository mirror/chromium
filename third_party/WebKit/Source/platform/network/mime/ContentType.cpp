/*
 * Copyright (C) 2006, 2008 Apple Inc.  All rights reserved.
 * Copyright (C) 2008 Torch Mobile Inc. All rights reserved.
 * (http://www.torchmobile.com/)
 * Copyright (C) 2009 Google Inc.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "platform/network/mime/ContentType.h"

namespace blink {

ContentType::ContentType(const String& content_type) {
  type_ = content_type.StripWhiteSpace();
}

String ContentType::Parameter(const String& parameter_name) const {
  // a MIME type can have one or more "param=value" after a semi-colon, and
  // separated from each other by semi-colons
  size_t pos = type_.find(';');
  if (pos == kNotFound)
    return String();
  ++pos;

  // TODO(crbug.com/674329): Fix this to match only strings that are placed at
  // the parameter name portion.
  pos = type_.FindIgnoringASCIICase(parameter_name, pos);
  if (pos == kNotFound)
    return String();
  pos += parameter_name.length();

  pos = type_.find('=', pos);
  if (pos == kNotFound)
    return String();
  ++pos;

  // TODO(crbug.com/674329): Fix this to skip only whitespace-ish characters.
  size_t quote = type_.find('\"', pos);
  if (quote == kNotFound) {
    // TODO(crbug.com/674329): Fix this to accept only valid characters for the
    // parameter value part.
    size_t end = type_.find(';', pos);
    if (end == kNotFound)
      end = type_.length();
    return type_.Substring(pos, end - pos).StripWhiteSpace();
  }
  pos = quote + 1;

  size_t end = type_.find('\"', pos);
  if (end == kNotFound)
    return String();

  return type_.Substring(pos, end - pos).StripWhiteSpace();
}

String ContentType::GetType() const {
  // "type" can have parameters after a semi-colon, strip them
  size_t pos = type_.find(';');
  if (pos == kNotFound)
    return type_;

  return type_.Left(pos).StripWhiteSpace();
}

}  // namespace blink
