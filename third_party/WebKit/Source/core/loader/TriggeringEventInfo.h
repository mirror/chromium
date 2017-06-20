// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef TriggeringEventInfo_h
#define TriggeringEventInfo_h

namespace blink {

// Extra info sometimes associated with a navigation.
enum TriggeringEventInfo {
  kTriggeringEventInfoUnknown,

  // The navigation was not triggered via a JS Event.
  kTriggeringEventInfoNotFromEvent,

  // The navigation was triggered via a JS event with isTrusted() == true.
  kTriggeringEventInfoFromTrustedEvent,

  // The navigation was triggered via a JS event with isTrusted() == false.
  kTriggeringEventInfoFromUntrustedEvent,

  kTriggeringEventInfoLast,
};

}  // namespace blink

#endif  // TriggeringEventInfo_h
