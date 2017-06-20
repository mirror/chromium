// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WebTriggeringEventInfo_h
#define WebTriggeringEventInfo_h

namespace blink {

// Extra info sometimes associated with a navigation. Mirrors
// the TriggeringEventInfoEnum.
enum WebTriggeringEventInfo {
  kWebTriggeringEventInfoUnknown,

  // The navigation was not triggered via a JS Event.
  kWebTriggeringEventInfoNotFromEvent,

  // The navigation was triggered via a JS event with isTrusted() == true.
  kWebTriggeringEventInfoFromTrustedEvent,

  // The navigation was triggered via a JS event with isTrusted() == false.
  kWebTriggeringEventInfoFromUntrustedEvent,

  kWebTriggeringEventInfoLast,
};

}  // namespace blink

#endif  // WebTriggeringEventInfo_h
