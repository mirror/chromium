// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "simple_api.h"

#include "base/synchronization/condition_variable.h"
#include "base/synchronization/lock.h"
#include "base/synchronization/waitable_event.h"
#include "base/threading/platform_thread.h"

#if !defined(USE_CONDITION_VARIABLES) && !defined(USE_WAITABLE_EVENTS)
#error \
    "One of the following macros must be defined:\
  USE_CONDITION_VARIABLES or USE_WAITABLE_EVENTS"
#endif

// An implementation of the simple:: classes that uses a worker thread to do
// all the work. Communication happens with a WaitableEvent.

namespace {

// Convenience sub-class of base::WaitableEvent to avoid too much typing.
// The event uses automatic reset and is not signaled in its initial state.
class MyEvent : public base::WaitableEvent {
 public:
  MyEvent()
      : base::WaitableEvent(base::WaitableEvent::ResetPolicy::AUTOMATIC,
                            base::WaitableEvent::InitialState::NOT_SIGNALED) {}
};

// CondVarChannel and EventChannel both implement a tiny synchronization
// channel between the main thread and the worker one.
//
// CondVarChannel uses a mutex and two condition variables to signal
// requests and replies. As such, the main thread should call the following
// methods:
//
//    - SenderStart():
//        Call this before anything else to prepare the sending thread.
//
//    - SenderRequestAndWait(command, param):
//        Where |command| is a liberal integer describing which specific
//        request to execute, and |param| being a generic pointer to a
//        command-specific parameter block.
//
//        This waits until the worker thread has completed. This assumes that
//        the result can be read from the |param| block that was passed to
//        the function, as such, there is no need to pass data back from
//        the worker thread to the main one here.
//
//        In other words, it means that the |param| block cannot be destroyed
//        until this method is called.
//
//    - SenderStop(command):
//        Where |command| is a liberal integer that must be interpreted
//        by the worker thread as a request to quit. This shall trigger
//        a WorkerStop() call.
//
//
// On the other hand, the worker thread should call the following methods:
//
//    - WorkerStart():
//        Must be called once by the worker thread to prepare.
//
//    - WorkerWaitForRequest(&command, &param):
//        Wait for a new request from the main thread. On return,
//        |*command| and |*param| will be set to the values that were
//        passed from the other thread when it called SendRequest().
//
//    - WorkerSignalReply():
//        Once the work thread has completed the request, it must call this
//        function to signal so. Even if it is about to exit.
//
//    - WorkerStop():
//        Must be called by the worker thread just before exiting.
//
// EventChannel implements the same methods, but instead uses a pair of
// base::WaitableEvent instances to do the same job. While it is simpler in
// its implementation, it is actually noticeably slower on Linux and Android!
// (e.g. 7400ns vs 6100ns on Linux).
//

class CondVarChannel {
 public:
  CondVarChannel() : lock_(), in_cond_(&lock_), out_cond_(&lock_) {}

  void SenderStart() { lock_.Acquire(); }

  void SenderRequestAndWait(int command, void* param) {
    command_ = command;
    param_ = param;
    available_ = 1;
    in_cond_.Signal();
    while (available_ != 0) {
      out_cond_.Wait();
    }
  }

  void SenderStop(int command) {
    command_ = command;
    param_ = nullptr;
    available_ = 1;
    in_cond_.Signal();
    lock_.Release();
  }

  void WorkerStart() { lock_.Acquire(); }

  void WorkerWaitForRequest(int* command, void** param) {
    while (available_ == 0) {
      in_cond_.Wait();
    }
    *command = command_;
    *param = param_;
  }

  void WorkerSignalReply() {
    available_ = 0;
    out_cond_.Signal();
  }

  void WorkerStop() {
    WorkerSignalReply();
    lock_.Release();
  }

 private:
  base::Lock lock_;
  base::ConditionVariable in_cond_;
  base::ConditionVariable out_cond_;

  int available_ = 0;
  int command_ = 0;
  void* param_ = 0;
};

struct EventChannel {
 public:
  EventChannel() = default;

  void SenderStart() {}

  void SenderRequestAndWait(int command, void* param) {
    command_ = command;
    param_ = param;
    in_event_.Signal();
    out_event_.Wait();
  }

  void SenderStop(int command) {
    command_ = command;
    param_ = nullptr;
    in_event_.Signal();
  }

  void WorkerStart() {}

  void WorkerWaitForRequest(int* command, void** param) {
    in_event_.Wait();
    *command = command_;
    *param = param_;
  }

  void WorkerSignalReply() { out_event_.Signal(); }

  void WorkerStop() { WorkerSignalReply(); }

 private:
  MyEvent in_event_;
  MyEvent out_event_;

  int command_ = 0;
  void* param_ = 0;
};

class WorkerThread : public base::PlatformThread::Delegate {
 public:
  WorkerThread() {
    base::PlatformThread::Create(0, this, &thread_handle_);
    channel_.SenderStart();
  }

  ~WorkerThread() override {
    channel_.SenderStop(static_cast<int>(Command::Quit));
    base::PlatformThread::Join(thread_handle_);
  }

  int32_t MathAdd(int32_t x, int32_t y) {
    MathAddInfo info = {x, y};
    SendRequestAndWaitForReply(Command::MathAdd, &info);
    return info.sum;
  }

  std::string EchoPing(const std::string& str) {
    std::string result;
    EchoPingInfo info = {str, &result};
    SendRequestAndWaitForReply(Command::EchoPing, &info);
    return result;
  }

  void ThreadMain() override {
    bool quit = false;
    channel_.WorkerStart();
    while (!quit) {
      int command;
      void* param;
      channel_.WorkerWaitForRequest(&command, &param);

      switch (static_cast<Command>(command)) {
        case Command::Quit: {
          quit = true;
          break;
        }

        case Command::MathAdd: {
          // Perform addition
          MathAddInfo* info = reinterpret_cast<MathAddInfo*>(param);
          info->sum = info->x + info->y;
          break;
        }

        case Command::EchoPing: {
          EchoPingInfo* info = reinterpret_cast<EchoPingInfo*>(param);
          *info->result = info->input;
          break;
        }
      }
      channel_.WorkerSignalReply();
    }
    channel_.WorkerStop();
  }

 private:
  struct MathAddInfo {
    int32_t x, y;
    int32_t sum;
  };

  struct EchoPingInfo {
    const std::string& input;
    std::string* result;
  };

  enum class Command {
    Quit,
    MathAdd,
    EchoPing,
  };

  void SendRequestAndWaitForReply(Command command, void* param) {
    channel_.SenderRequestAndWait(static_cast<int>(command), param);
  }

#if defined(USE_CONDITION_VARIABLES)
  CondVarChannel channel_;
#elif defined(USE_WAITABLE_EVENTS)
  EventChannel channel_;
#else
#error "Define one of the USE_XXXX macros required by this source file"
#endif

  base::PlatformThreadHandle thread_handle_;
};

static WorkerThread sWorkerThread;

class MyMath : public simple::Math {
 public:
  int32_t Add(int32_t x, int32_t y) override {
    return sWorkerThread.MathAdd(x, y);
  }
};

class MyEcho : public simple::Echo {
 public:
  std::string Ping(const std::string& str) override {
    return sWorkerThread.EchoPing(str);
  }
};

}  // namespace

simple::Math* createMath() {
  return new MyMath();
}
simple::Echo* createEcho() {
  return new MyEcho();
}
const char* createAbstract() {
  return "Background PlatformThread with "
#if defined(USE_CONDITION_VARIABLES)
         "condition variables"
#elif defined(USE_WAITABLE_EVENTS)
         "waitable events"
#else
         "magic(!!)"
#endif
         " for sync.";
}
