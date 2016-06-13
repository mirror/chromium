// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_POWER_SAVE_BLOCKER_IMPL_H_
#define CONTENT_BROWSER_POWER_SAVE_BLOCKER_IMPL_H_

#include <string>

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "base/sequenced_task_runner.h"
#include "base/single_thread_task_runner.h"
#include "build/build_config.h"
#include "content/public/browser/power_save_blocker.h"

#if defined(OS_ANDROID)
#include "ui/android/view_android.h"
#endif  // OS_ANDROID

namespace content {

class WebContents;

class PowerSaveBlockerImpl : public PowerSaveBlocker {
 public:
  PowerSaveBlockerImpl(
      PowerSaveBlockerType type,
      Reason reason,
      const std::string& description,
      scoped_refptr<base::SequencedTaskRunner> ui_task_runner,
      scoped_refptr<base::SingleThreadTaskRunner> blocking_task_runner);
  ~PowerSaveBlockerImpl() override;

#if defined(OS_ANDROID)
  // On Android, the kPowerSaveBlockPreventDisplaySleep type of
  // PowerSaveBlocker should associated with a View, so the blocker can be
  // removed by the platform.
  void InitDisplaySleepBlocker(
      const base::WeakPtr<ui::ViewAndroid>& view_android);
#endif

 private:
  class Delegate;

  // Implementations of this class may need a second object with different
  // lifetime than the RAII container, or additional storage. This member is
  // here for that purpose. If not used, just define the class as an empty
  // RefCounted (or RefCountedThreadSafe) like so to make it compile:
  // class PowerSaveBlocker::Delegate
  //     : public base::RefCounted<PowerSaveBlocker::Delegate> {
  //  private:
  //   friend class base::RefCounted<Delegate>;
  //   ~Delegate() {}
  // };
  scoped_refptr<Delegate> delegate_;

#if defined(USE_X11)
  // Since display sleep prevention also implies system suspend prevention, for
  // the Linux FreeDesktop API case, there needs to be a second delegate to
  // block system suspend when screen saver / display sleep is blocked.
  scoped_refptr<Delegate> freedesktop_suspend_delegate_;
#endif

  scoped_refptr<base::SequencedTaskRunner> ui_task_runner_;
  scoped_refptr<base::SequencedTaskRunner> blocking_task_runner_;

  DISALLOW_COPY_AND_ASSIGN(PowerSaveBlockerImpl);
};

}  // namespace content

#endif  // CONTENT_BROWSER_POWER_SAVE_BLOCKER_IMPL_H_
