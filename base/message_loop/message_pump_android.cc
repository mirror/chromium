// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/message_loop/message_pump_android.h"

#include <android/looper.h>
#include <fcntl.h>
#include <jni.h>
#include <sys/eventfd.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <unistd.h>
#include <utility>

#include "base/android/jni_android.h"
#include "base/android/scoped_java_ref.h"
#include "base/callback_helpers.h"
#include "base/feature_list.h"
#include "base/lazy_instance.h"
#include "base/logging.h"
#include "base/message_loop/message_loop.h"
#include "base/run_loop.h"
#include "jni/SystemMessageHandler_jni.h"

// Android stripped sys/timerfd.h out of their platform headers, so we have to
// use syscall to make use of timerfd. Once the min API level is 20, we can
// directly use timerfd.h.
#ifndef __NR_timerfd_create
#error "Unable to find syscall for __NR_timerfd_create"
#endif

#ifndef TFD_TIMER_ABSTIME
#define TFD_TIMER_ABSTIME (1 << 0)
#endif

using base::android::JavaParamRef;
using base::android::ScopedJavaLocalRef;

namespace base {

namespace {

// TODO(mthiesse): Replace this with a feature so that we can implement a kill
// switch in case this causes problems, and a control group to measure the
// performance impact. We can't simply use a feature as you usually would
// because feature initialization happens well after the MessagePump is started
// and cannot be moved earlier.
static constexpr bool kUseNativeLooper = true;

// See sys/timerfd.h
int timerfd_create(int clockid, int flags) {
  return syscall(__NR_timerfd_create, clockid, flags);
}

// See sys/timerfd.h
int timerfd_settime(int ufc,
                    int flags,
                    const struct itimerspec* utmr,
                    struct itimerspec* otmr) {
  return syscall(__NR_timerfd_settime, ufc, flags, utmr, otmr);
}

static int nonDelayedLooperCallback(int fd, int events, void* data) {
  if (events & ALOOPER_EVENT_HANGUP)
    return 0;

  DCHECK(events & ALOOPER_EVENT_INPUT);
  MessagePumpForUI* pump = reinterpret_cast<MessagePumpForUI*>(data);
  pump->OnLooperCallback(false);
  return 1;  // continue listening for events
}

static int delayedLooperCallback(int fd, int events, void* data) {
  if (events & ALOOPER_EVENT_HANGUP)
    return 0;

  DCHECK(events & ALOOPER_EVENT_INPUT);
  MessagePumpForUI* pump = reinterpret_cast<MessagePumpForUI*>(data);
  pump->OnLooperCallback(true);
  return 1;  // continue listening for events
}

}  // namespace

// Initialization is done in Start().
MessagePumpForUI::MessagePumpForUI() = default;

MessagePumpForUI::~MessagePumpForUI() {
  if (!looper_)
    return;
  DCHECK(ALooper_forThread() == looper_);
  ALooper_removeFd(looper_, non_delayed_fd_);
  ALooper_removeFd(looper_, delayed_fd_);
  close(non_delayed_fd_);
  close(delayed_fd_);
  ALooper_release(looper_);
  looper_ = nullptr;
}

void MessagePumpForUI::OnLooperCallback(bool delayed) {
  if (ShouldAbort() || quit_)
    return;
  bool did_work = delegate_->DoWork();

  base::TimeTicks next_delayed_work_time;
  if (delegate_->DoDelayedWork(&next_delayed_work_time)) {
    uint64_t value;
    read(delayed_fd_, &value, sizeof(value));
    delayed_scheduled_time_ = base::TimeTicks();
  }
  if (!next_delayed_work_time.is_null()) {
    ScheduleDelayedWork(next_delayed_work_time);
  }
  if (!did_work && delayed) {
    DoIdleWork();
    return;
  }
  // No need to read from the fd (and clear it) if there are more tasks queued
  // up.
  if (--pending_tasks_ > 0)
    return;
  uint64_t value;
  read(non_delayed_fd_, &value, sizeof(value));
  // If we read a value > 1, it means we lost the race to clear the fd before a
  // new task was posted. This is okay, we can just write (add) the value back.
  if (--value > 0) {
    write(non_delayed_fd_, &value, sizeof(value));
  } else {
    DoIdleWork();
  }
}

// This is called by the java SystemMessageHandler whenever the message queue
// detects an idle state (as in, control returns to the looper and there are no
// tasks available to be run immediately).
// See the comments in DoRunLoopOnce for how this differs from the
// implementation on other platforms.
void MessagePumpForUI::DoIdleWork(JNIEnv* env,
                                  const JavaParamRef<jobject>& obj) {
  DCHECK(!use_native_looper_);
  DoIdleWork();
}

void MessagePumpForUI::DoIdleWork() {
  if (delegate_->DoIdleWork()) {
    // If we did Idle work, the MessageLoop may be waiting for a second call
    // to DoIdleWork to quit, but won't queue up any tasks. Queue up a fake task
    // to wake the looper.
    ScheduleWork();
  }
  if (!idle_callback_.is_null())
    base::ResetAndReturn(&idle_callback_).Run();
}

void MessagePumpForUI::DoRunLoopOnce(JNIEnv* env,
                                     const JavaParamRef<jobject>& obj,
                                     jboolean delayed) {
  if (delayed)
    delayed_scheduled_time_ = base::TimeTicks();

  // If the pump has been aborted, tasks may continue to be queued up, but
  // shouldn't run.
  if (ShouldAbort())
    return;

  // This is based on MessagePumpForUI::DoRunLoop() from desktop.
  // Note however that our system queue is handled in the java side.
  // In desktop we inspect and process a single system message and then
  // we call DoWork() / DoDelayedWork(). This is then wrapped in a for loop and
  // repeated until no work is left to do, at which point DoIdleWork is called.
  // On Android, the java message queue may contain messages for other handlers
  // that will be processed before calling here again.
  // This means that unlike Desktop, we can't wrap a for loop around this
  // function and keep processing tasks until we have no work left to do - we
  // have to return control back to the Android Looper after each message. This
  // also means we have to perform idle detection differently, which is why we
  // add an IdleHandler to the message queue in SystemMessageHandler.java, which
  // calls DoIdleWork whenever control returns back to the looper and there are
  // no tasks queued up to run immediately.
  delegate_->DoWork();
  if (ShouldAbort()) {
    // There is a pending JNI exception, return to Java so that the exception is
    // thrown correctly.
    return;
  }

  base::TimeTicks next_delayed_work_time;
  delegate_->DoDelayedWork(&next_delayed_work_time);
  if (ShouldAbort()) {
    // There is a pending JNI exception, return to Java so that the exception is
    // thrown correctly
    return;
  }

  if (!next_delayed_work_time.is_null())
    ScheduleDelayedWork(next_delayed_work_time);
}

void MessagePumpForUI::Run(Delegate* delegate) {
  NOTREACHED() << "UnitTests should rely on MessagePumpForUIStub in"
                  " test_stub_android.h";
}

void MessagePumpForUI::Initialize(bool use_native_looper) {
  // TODO(mthiesse): Replace this with a Feature check.
  use_native_looper_ = use_native_looper;

  if (use_native_looper_) {
    non_delayed_fd_ = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    DCHECK(TimeTicks::GetClock() == TimeTicks::Clock::LINUX_CLOCK_MONOTONIC);
    delayed_fd_ = timerfd_create(CLOCK_MONOTONIC, 0);
    int flags = fcntl(delayed_fd_, F_GETFL, 0);  // Is this necessary?
    fcntl(delayed_fd_, F_SETFL, flags | O_NONBLOCK | O_CLOEXEC);
    looper_ = ALooper_prepare(0);
    CHECK(looper_);
    // Add a reference to the looper so it isn't deleted on us.
    ALooper_acquire(looper_);
    ALooper_addFd(looper_, non_delayed_fd_, 0, ALOOPER_EVENT_INPUT,
                  nonDelayedLooperCallback, reinterpret_cast<void*>(this));
    ALooper_addFd(looper_, delayed_fd_, 0, ALOOPER_EVENT_INPUT,
                  delayedLooperCallback, reinterpret_cast<void*>(this));
  } else {
    // Note that even when posted tasks are handled natively, the IdleHandler
    // still has to live in java.
    JNIEnv* env = base::android::AttachCurrentThread();
    DCHECK(env);
    system_message_handler_obj_.Reset(
        Java_SystemMessageHandler_create(env, reinterpret_cast<jlong>(this)));
  }
}

void MessagePumpForUI::Start(Delegate* delegate) {
  DCHECK(!quit_);

  SetDelegate(delegate);
  run_loop_ = std::make_unique<RunLoop>();
  // Since the RunLoop was just created above, BeforeRun should be guaranteed to
  // return true (it only returns false if the RunLoop has been Quit already).
  if (!run_loop_->BeforeRun())
    NOTREACHED();

  DCHECK(system_message_handler_obj_.is_null());

  Initialize(kUseNativeLooper);
}

void MessagePumpForUI::Quit() {
  // This message pump doesn't support nesting, so it shouldn't be quit multiple
  // times.
  DCHECK(!quit_);

  quit_ = true;

  if (use_native_looper_) {
    // Clear any pending timer.
    struct itimerspec ts = {{0}, {0}};
    timerfd_settime(delayed_fd_, TFD_TIMER_ABSTIME, &ts, nullptr);
    // Clear the eventfd.
    int64_t value;
    read(non_delayed_fd_, &value, sizeof(value));
  }
  if (!system_message_handler_obj_.is_null()) {
    JNIEnv* env = base::android::AttachCurrentThread();
    DCHECK(env);

    Java_SystemMessageHandler_shutdown(env, system_message_handler_obj_);
    system_message_handler_obj_.Reset();
  }

  if (run_loop_) {
    run_loop_->AfterRun();
    run_loop_ = nullptr;
  }
}

void MessagePumpForUI::ScheduleWork() {
  if (quit_)
    return;
  if (use_native_looper_) {
    // No need to write to the fd if the poll to our fd will already indicate
    // we have work to do. So we only write on the 0->1 edge.
    // In this way, we can avoid talking to the kernel unnecessarily, especially
    // when under load and queueing up a lot of tasks.
    if (pending_tasks_++ > 0)
      return;
    uint64_t value = 1;
    write(non_delayed_fd_, &value, sizeof(value));
  } else {
    DCHECK(!system_message_handler_obj_.is_null());

    JNIEnv* env = base::android::AttachCurrentThread();
    DCHECK(env);

    Java_SystemMessageHandler_scheduleWork(env, system_message_handler_obj_);
  }
}

void MessagePumpForUI::ScheduleDelayedWork(const TimeTicks& delayed_work_time) {
  if (quit_)
    return;
  // In the java side, |SystemMessageHandler| keeps a single "delayed" message.
  // It's an expensive operation to |removeMessage| there, so this is optimized
  // to avoid those calls.
  //
  // At this stage, |delayed_work_time| can be:
  // 1) The same as previously scheduled: nothing to be done, move along. This
  // is the typical case, since this method is called for every single message.
  //
  // 2) Not previously scheduled: just post a new message in java.
  //
  // 3) Shorter than previously scheduled: far less common. In this case,
  // |removeMessage| and post a new one.
  //
  // 4) Longer than previously scheduled (or null): nothing to be done, move
  // along.
  if (!delayed_scheduled_time_.is_null() &&
      delayed_work_time >= delayed_scheduled_time_) {
    return;
  }
  DCHECK(!delayed_work_time.is_null());
  delayed_scheduled_time_ = delayed_work_time;
  if (use_native_looper_) {
    int64_t micros = delayed_work_time.since_origin().InMicroseconds();
    struct itimerspec ts;
    ts.it_interval.tv_sec = 0;  // Don't repeat.
    ts.it_interval.tv_nsec = 0;
    ts.it_value.tv_sec = micros / TimeTicks::kMicrosecondsPerSecond;
    ts.it_value.tv_nsec = (micros % TimeTicks::kMicrosecondsPerSecond) *
                          TimeTicks::kNanosecondsPerMicrosecond;

    timerfd_settime(delayed_fd_, TFD_TIMER_ABSTIME, &ts, nullptr);
  } else {
    DCHECK(!system_message_handler_obj_.is_null());

    JNIEnv* env = base::android::AttachCurrentThread();
    DCHECK(env);

    jlong millis =
        (delayed_work_time - TimeTicks::Now()).InMillisecondsRoundedUp();

    // Note that we're truncating to milliseconds as required by the java side,
    // even though delayed_work_time is microseconds resolution.
    Java_SystemMessageHandler_scheduleDelayedWork(
        env, system_message_handler_obj_, millis);
  }
}

bool MessagePumpForUI::IsIdle() {
  return atomic_load(&pending_tasks_) == 0 &&
         delayed_scheduled_time_ == base::TimeTicks();
}

void MessagePumpForUI::SetIdleCallback(base::OnceClosure callback) {
  if (use_native_looper_ && IsIdle()) {
    std::move(callback).Run();
    return;
  }
  idle_callback_ = std::move(callback);
}

}  // namespace base
