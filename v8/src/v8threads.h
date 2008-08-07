// Copyright 2008 Google Inc. All Rights Reserved.
// <<license>>

#ifndef V8_V8THREADS_H_
#define V8_V8THREADS_H_

namespace v8 { namespace internal {


class ThreadState {
 public:
  // Iterate over in-use states.
  static ThreadState* FirstInUse();
  // Returns NULL after the last one.
  ThreadState* Next();

  enum List {FREE_LIST, IN_USE_LIST};

  void LinkInto(List list);
  void Unlink();

  static ThreadState* GetFree();

  // Get data area for archiving a thread.
  char* data() { return data_; }
 private:
  ThreadState();

  void AllocateSpace();

  char* data_;
  ThreadState* next_;
  ThreadState* previous_;
  // In the following two lists there is always at least one object on the list.
  // The first object is a flying anchor that is only there to simplify linking
  // and unlinking.
  // Head of linked list of free states.
  static ThreadState* free_anchor_;
  // Head of linked list of states in use.
  static ThreadState* in_use_anchor_;
};


class ThreadManager : public AllStatic {
 public:
  static void Lock();
  static void Unlock();

  static void ArchiveThread();
  static bool RestoreThread();

  static void Iterate(ObjectVisitor* v);
  static void MarkCompactPrologue();
  static void MarkCompactEpilogue();
  static bool IsLockedByCurrentThread() { return mutex_owner_.IsSelf(); }
 private:
  static void EagerlyArchiveThread();

  static Mutex* mutex_;
  static ThreadHandle mutex_owner_;
  static ThreadHandle lazily_archived_thread_;
  static ThreadState* lazily_archived_thread_state_;
};


class ContextSwitcher: public Thread {
 public:
  void Run();
  static void StartPreemption(int every_n_ms);
  static void StopPreemption();
  static void PreemptionReceived();
 private:
  explicit ContextSwitcher(int every_n_ms);
  void WaitForPreemption();
  void Stop();
  Semaphore* preemption_semaphore_;
  bool keep_going_;
  int sleep_ms_;
};

} }  // namespace v8::internal

#endif  // V8_V8THREADS_H_
