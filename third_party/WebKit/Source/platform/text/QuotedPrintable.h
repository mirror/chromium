/*
 * Copyright (C) 2011 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef QuotedPrintable_h
#define QuotedPrintable_h

#include "platform/PlatformExport.h"
#include "platform/wtf/Vector.h"

namespace blink {

// The original characters may be encoded a bit differently depending on where
// they live, header or body.
// 1) Soft line break: "=CRLF" should be used to break long line in body while
//    "CRLF+SPACE" should be used to break long line in header.
// 2) SPACE and TAB: they only need to be encoded if they appear at the end of
//    the body line. But they should always be encoded if they appear anywhere
//    in the header line.
enum class QuotedPrintableEncodeType {
  //  For characters in body, per RFC 2045.
  kForBody,
  //  For characters in header, per RFC 2047.
  kForHeader,
};

PLATFORM_EXPORT void QuotedPrintableEncode(const char*,
                                           size_t,
                                           QuotedPrintableEncodeType,
                                           size_t max_line_length,
                                           Vector<char>&);

PLATFORM_EXPORT void QuotedPrintableDecode(const char*, size_t, Vector<char>&);

}  // namespace blink

#endif  // QuotedPrintable_h
