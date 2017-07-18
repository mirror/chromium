// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ClientHintsPreferences_h
#define ClientHintsPreferences_h

#include "platform/PlatformExport.h"
#include "platform/wtf/Allocator.h"
#include "platform/wtf/text/WTFString.h"
#include "public/platform/WebClientHintsType.h"

namespace blink {

class KURL;

class PLATFORM_EXPORT ClientHintsPreferences {
  DISALLOW_NEW();

 public:
  class Context {
   public:
    virtual void CountClientHints(WebClientHintsType) = 0;
    virtual void CountPersistentClientHintHeaders() = 0;

   protected:
    virtual ~Context() {}
  };

  ClientHintsPreferences();

  void UpdateFrom(const ClientHintsPreferences&);
  void UpdateFromAcceptClientHintsHeader(const String& header_value, Context*);

  bool ShouldSend(WebClientHintsType type) const {
    return enabled_types_[type];
  }
  void SetShouldSendForTesting(WebClientHintsType type) {
    enabled_types_[type] = true;
  }

  // Parses the client hints headers, and populates |enabled_types| with the
  // client hint preferences that should be persisted for duration
  // |persist_duration_seconds|. |persist_duration_seconds| should be non-null.
  // If there are no client hints that need to be persisted,
  // |persist_duration_seconds| is set to -1, otherwise it is set to the
  // duration (in clients) for which the client hint preferences should be
  // persisted.
  static void UpdatePersistentHintsFromHeaders(
      const String& accept_ch_header_value,
      const String& accept_ch_lifetime_header_value,
      const KURL&,
      Context*,
      bool enabled_types[kWebClientHintsTypeLast + 1],
      int64_t* persist_duration_seconds);

 private:
  bool enabled_types_[kWebClientHintsTypeLast + 1] = {};
};

}  // namespace blink

#endif
