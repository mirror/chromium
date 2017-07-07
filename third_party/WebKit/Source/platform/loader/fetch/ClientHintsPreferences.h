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

class PLATFORM_EXPORT ClientHintsPreferences {
  DISALLOW_NEW();

 public:
  class Context {
   public:
    virtual void CountClientHintsDeviceRAM() = 0;
    virtual void CountClientHintsDPR() = 0;
    virtual void CountClientHintsResourceWidth() = 0;
    virtual void CountClientHintsViewportWidth() = 0;

   protected:
    virtual ~Context() {}
  };

  ClientHintsPreferences();

  void UpdateFrom(const ClientHintsPreferences&);
  void UpdateFromAcceptClientHintsHeader(const String& header_value, Context*);

  // Parses the client hints headers, and populates |enabled_types| with the
  // client hint prefernces that should be persisted for duration
  // |persist_duration_seconds|. |persist_duration_seconds| should be non-null.
  // |persist_duration_seconds| will be set to -1 if no client hint needs to be
  // persisted.
  static void UpdatePersistentFromAcceptClientHintsHeader(
      const String& accept_ch_header_value,
      const String& accept_ch_lifetime_header_value,
      bool enabled_types[kWebClientHintsTypeNumValues],
      int64_t* persist_duration_seconds);

  bool ShouldSend(WebClientHintsType type) const {
    return enabled_types_[type];
  }
  void SetShouldSend(WebClientHintsType type, bool should) {
    enabled_types_[type] = should;
  }

 private:
  bool enabled_types_[kWebClientHintsTypeNumValues];
};

}  // namespace blink

#endif
