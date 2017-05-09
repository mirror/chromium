// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "mojo/public/cpp/bindings/sync_handle_registry.h"

#include <unordered_map>

#include "base/lazy_instance.h"
#include "base/logging.h"
#include "base/sequence_token.h"
#include "base/stl_util.h"
#include "base/threading/sequenced_worker_pool.h"
#include "base/threading/thread_local.h"
#include "mojo/public/c/system/core.h"

namespace mojo {
namespace {

struct WorkerPoolSequenceTokenHasher {
  size_t operator()(
      const base::SequencedWorkerPool::SequenceToken& value) const {
    return std::hash<decltype(value.ToInternalValue())>()(
        value.ToInternalValue());
  }
};

struct WorkerPoolSequenceTokenEquals {
  size_t operator()(const base::SequencedWorkerPool::SequenceToken& a,
                    const base::SequencedWorkerPool::SequenceToken& b) const {
    return a.Equals(b);
  }
};

struct SequenceTokenHasher {
  size_t operator()(const base::SequenceToken& value) const {
    return std::hash<decltype(value.ToInternalValue())>()(
        value.ToInternalValue());
  }
};

// Provides access to a sequence-local SyncHandleRegistry pointer. For
// standalone threads, thread-local storage is used. Otherwise, global maps
// keyed by SequenceToken are used if one is set for the current thread. Note
// that both types of SequenceToken are used: base::SequenceToken and
// base::SequencedWorkerPool::SequenceToken, in that order.
//
// TODO(crbug.com/695727): Use SequenceLocalStorage instead when it's ready.
class SequenceLocalPointer {
 public:
  template <typename Factory>
  scoped_refptr<SyncHandleRegistry> GetOrCreate(Factory create) {
    SyncHandleRegistry** registry_pointer_storage = GetRegistryPointerStorage();
    if (!registry_pointer_storage) {
      scoped_refptr<SyncHandleRegistry> registry(thread_local_storage_.Get());
      if (registry)
        return registry;

      registry = create();
      thread_local_storage_.Set(registry.get());
      return registry;
    }

    scoped_refptr<SyncHandleRegistry> registry(*registry_pointer_storage);
    if (!registry) {
      registry = create();
      *registry_pointer_storage = registry.get();
    }
    return registry;
  }

  void Clear() {
    auto sequence_token = base::SequenceToken::GetForCurrentThread();
    if (sequence_token.IsValid()) {
      base::AutoLock l(lock_);
      size_t erased = sequence_to_registry_map_.erase(sequence_token);
      DCHECK(erased);
      return;
    }
    auto worker_pool_sequence_token =
        base::SequencedWorkerPool::GetSequenceTokenForCurrentThread();
    if (worker_pool_sequence_token.IsValid()) {
      base::AutoLock l(lock_);
      size_t erased = worker_pool_sequence_to_registry_map_.erase(
          worker_pool_sequence_token);
      DCHECK(erased);
      return;
    }
    thread_local_storage_.Set(nullptr);
  }

 private:
  // Returns a pointer to the storage for a |SyncHandleRegistry*| for the
  // current sequence. Note that the returned pointer may be safely read and
  // written without |lock_| held since this entry in the map is only accessed
  // on the current sequence.
  SyncHandleRegistry** GetRegistryPointerStorage() {
    auto sequence_token = base::SequenceToken::GetForCurrentThread();
    if (sequence_token.IsValid()) {
      base::AutoLock l(lock_);
      return &sequence_to_registry_map_[sequence_token];
    }
    auto worker_pool_sequence_token =
        base::SequencedWorkerPool::GetSequenceTokenForCurrentThread();
    if (worker_pool_sequence_token.IsValid()) {
      base::AutoLock l(lock_);
      return &worker_pool_sequence_to_registry_map_[worker_pool_sequence_token];
    }
    return nullptr;
  }

  base::ThreadLocalPointer<SyncHandleRegistry> thread_local_storage_;

  // Guards |sequence_to_registry_map_| and
  // |worker_pool_sequence_to_registry_map_|.
  base::Lock lock_;

  std::unordered_map<base::SequenceToken,
                     SyncHandleRegistry*,
                     SequenceTokenHasher>
      sequence_to_registry_map_;

  // TODO(sammc): Remove this once base::SequenceToken is used everywhere.
  std::unordered_map<base::SequencedWorkerPool::SequenceToken,
                     SyncHandleRegistry*,
                     WorkerPoolSequenceTokenHasher,
                     WorkerPoolSequenceTokenEquals>
      worker_pool_sequence_to_registry_map_;
};

base::LazyInstance<SequenceLocalPointer>::Leaky g_current_sync_handle_registry =
    LAZY_INSTANCE_INITIALIZER;

}  // namespace

// static
scoped_refptr<SyncHandleRegistry> SyncHandleRegistry::current() {
  return g_current_sync_handle_registry.Pointer()->GetOrCreate(
      []() { return make_scoped_refptr(new SyncHandleRegistry()); });
}

bool SyncHandleRegistry::RegisterHandle(const Handle& handle,
                                        MojoHandleSignals handle_signals,
                                        const HandleCallback& callback) {
  DCHECK(sequence_checker_.CalledOnValidSequence());

  if (base::ContainsKey(handles_, handle))
    return false;

  MojoResult result = wait_set_.AddHandle(handle, handle_signals);
  if (result != MOJO_RESULT_OK)
    return false;

  handles_[handle] = callback;
  return true;
}

void SyncHandleRegistry::UnregisterHandle(const Handle& handle) {
  DCHECK(sequence_checker_.CalledOnValidSequence());
  if (!base::ContainsKey(handles_, handle))
    return;

  MojoResult result = wait_set_.RemoveHandle(handle);
  DCHECK_EQ(MOJO_RESULT_OK, result);
  handles_.erase(handle);
}

bool SyncHandleRegistry::RegisterEvent(base::WaitableEvent* event,
                                       const base::Closure& callback) {
  auto result = events_.insert({event, callback});
  DCHECK(result.second);
  MojoResult rv = wait_set_.AddEvent(event);
  if (rv == MOJO_RESULT_OK)
    return true;
  DCHECK_EQ(MOJO_RESULT_ALREADY_EXISTS, rv);
  return false;
}

void SyncHandleRegistry::UnregisterEvent(base::WaitableEvent* event) {
  auto it = events_.find(event);
  DCHECK(it != events_.end());
  events_.erase(it);
  MojoResult rv = wait_set_.RemoveEvent(event);
  DCHECK_EQ(MOJO_RESULT_OK, rv);
}

bool SyncHandleRegistry::Wait(const bool* should_stop[], size_t count) {
  DCHECK(sequence_checker_.CalledOnValidSequence());

  size_t num_ready_handles;
  Handle ready_handle;
  MojoResult ready_handle_result;

  scoped_refptr<SyncHandleRegistry> preserver(this);
  while (true) {
    for (size_t i = 0; i < count; ++i)
      if (*should_stop[i])
        return true;

    // TODO(yzshen): Theoretically it can reduce sync call re-entrancy if we
    // give priority to the handle that is waiting for sync response.
    base::WaitableEvent* ready_event = nullptr;
    num_ready_handles = 1;
    wait_set_.Wait(&ready_event, &num_ready_handles, &ready_handle,
                   &ready_handle_result);
    if (num_ready_handles) {
      DCHECK_EQ(1u, num_ready_handles);
      const auto iter = handles_.find(ready_handle);
      iter->second.Run(ready_handle_result);
    }

    if (ready_event) {
      const auto iter = events_.find(ready_event);
      DCHECK(iter != events_.end());
      iter->second.Run();
    }
  };

  return false;
}

SyncHandleRegistry::SyncHandleRegistry() = default;

SyncHandleRegistry::~SyncHandleRegistry() {
  DCHECK(sequence_checker_.CalledOnValidSequence());
  g_current_sync_handle_registry.Pointer()->Clear();
}

}  // namespace mojo
