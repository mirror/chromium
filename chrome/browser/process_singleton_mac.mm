// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/process_singleton.h"

#include <Carbon/Carbon.h>
#include <CoreServices/CoreServices.h>

#include "base/mac/scoped_aedesc.h"

namespace {

// Forwards |theAppleEvent| to an already-running Chromium process.
OSErr HandleGURLEvent(const AppleEvent* theAppleEvent,
                      AppleEvent* reply,
                      SRefCon handlerRefcon) {
  // Forwarding the message without |kAENoReply| hangs us, so assume that a
  // reply is unnecessary as an invariant.
  DCHECK_EQ(reply->descriptorType, typeNull);

  pid_t forwarding_pid = *(reinterpret_cast<pid_t*>(handlerRefcon));
  base::mac::ScopedAEDesc<AEAddressDesc> address;
  if (AEGetAttributeDesc(theAppleEvent, keyAddressAttr, typeWildCard,
                         address.OutPointer()) == noErr) {
    base::mac::ScopedAEDesc<> other_process_pid;
    // Create an address descriptor for the running process.
    AECreateDesc(typeKernelProcessID, &forwarding_pid, sizeof(forwarding_pid),
                 other_process_pid.OutPointer());

    // Change the event's address to the running process.
    AppleEvent* event = const_cast<AppleEvent*>(theAppleEvent);
    if (AEPutAttributeDesc(event, keyAddressAttr, other_process_pid) == noErr) {
      // Send the message back out.
      AESendMessage(event, reply, kAENoReply, kNoTimeOut);
      return noErr;
    }
  }
  return noErr;
}
}

bool ProcessSingleton::ForwardOpenURLEvent(pid_t forwarding_pid) {
  if (AEInstallEventHandler(kInternetEventClass, kAEGetURL,
                            NewAEEventHandlerUPP(HandleGURLEvent),
                            &forwarding_pid, false) != noErr) {
    return false;
  }
  const EventTypeSpec spec = {kEventClassAppleEvent, kEventAppleEvent};
  EventRef eventRef;
  if (ReceiveNextEvent(1, &spec, kEventDurationMillisecond, true, &eventRef) ==
      noErr) {
    AEProcessEvent(eventRef);
    ReleaseEvent(eventRef);
    return true;
  }
  return false;
}
