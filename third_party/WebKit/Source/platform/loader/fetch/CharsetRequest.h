// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CharsetRequest_h
#define CharsetRequest_h

#include "platform/PlatformExport.h"
#include "platform/wtf/text/TextEncoding.h"

namespace blink {

// Charset requested when fetching.
// Currently used only by TextResource and its subclasses.
//
// Actual charset used for decoding can be different from |encoding_| because
// of encoding detection by TextResourceDecoder.
class PLATFORM_EXPORT CharsetRequest final {
 public:
  explicit CharsetRequest(
      const WTF::TextEncoding& encoding = WTF::TextEncoding())
      : CharsetRequest(false, encoding) {}
  static CharsetRequest CreateAlwaysUseUTF8ForText() {
    return CharsetRequest(true, WTF::UTF8Encoding());
  }

  bool AlwaysUseUTF8() const { return always_use_utf8_; }
  const WTF::TextEncoding& Encoding() const { return encoding_; }

 private:
  CharsetRequest(bool always_use_utf8, const WTF::TextEncoding& encoding)
      : always_use_utf8_(always_use_utf8), encoding_(encoding) {}

  bool always_use_utf8_;
  WTF::TextEncoding encoding_;
};

}  // namespace blink

#endif
