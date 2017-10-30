// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ScopedVirtualTimePauser_h
#define ScopedVirtualTimePauser_h

#include "platform/PlatformExport.h"
#include "platform/wtf/WeakPtr.h"

namespace blink {
namespace scheduler {
class WebViewSchedulerImpl;
}  // namespace scheduler

// A move only RAII style helper which makes it easier for subsystems to pause
// virtual time while performing an asynchronous operation.
class PLATFORM_EXPORT ScopedVirtualTimePauser {
 public:
  explicit ScopedVirtualTimePauser(
      WTF::WeakPtr<scheduler::WebViewSchedulerImpl>);

  ScopedVirtualTimePauser();
  ~ScopedVirtualTimePauser();

  ScopedVirtualTimePauser(ScopedVirtualTimePauser&& other);
  ScopedVirtualTimePauser& operator=(ScopedVirtualTimePauser&& other);

  ScopedVirtualTimePauser(const ScopedVirtualTimePauser&) = delete;
  ScopedVirtualTimePauser& operator=(const ScopedVirtualTimePauser&) = delete;

  // Virtual time will be paused if any ScopedVirtualTimePauser votes to pause
  // it, and only unpaused only if all ScopedVirtualTimePausers are either
  // destroyed or vote to unpause.
  void PauseVirtualTime(bool paused);

 private:
  bool paused_ = false;
  WTF::WeakPtr<scheduler::WebViewSchedulerImpl> scheduler_;
};

}  // namespace blink

#endif  // THIRD_PARTY_WEBKIT_SOURCE_PLATFORM_SCHEDULER_RENDERER_SCOPED_VIRTUAL_TIME_PAUSER_H_
