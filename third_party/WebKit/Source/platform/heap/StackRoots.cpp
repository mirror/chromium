/*
 * Copyright (C) 2013 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "platform/heap/StackRoots.h"

#include "platform/heap/Heap.h"
#include "platform/wtf/StackUtil.h"

namespace blink {

#ifdef ADDRESS_SANITIZER
// When we are running under AddressSanitizer with
// detect_stack_use_after_return=1 then stack marker obtained from
// stack marker will point into a fake stack.  Detect this case by checking if
// it falls in between current stack frame and stack start and use an arbitrary
// high enough value for it.  Don't adjust stack marker in any other case to
// match behavior of code running without AddressSanitizer.
NO_SANITIZE_ADDRESS static void* AdjustStackMarkerForAdressSanitizer(
    void* stack_marker) {
  Address start = reinterpret_cast<Address>(WTF::GetStackStart());
  Address end = reinterpret_cast<Address>(&start);
  CHECK_LT(end, start);

  if (end <= stack_marker && stack_marker < start)
    return stack_marker;

  // 256 is as good an approximation as any else.
  const size_t kBytesToCopy = sizeof(Address) * 256;
  if (static_cast<size_t>(start - end) < kBytesToCopy)
    return start;

  return end + kBytesToCopy;
}
#endif

// TODO(haraken): The first void* pointer is unused. Remove it.
using PushAllRegistersCallback = void (*)(void*, StackRoots*, intptr_t*);
extern "C" void PushAllRegisters(void*, StackRoots*, PushAllRegistersCallback);

static void DidPushAllRegisters(void*,
                                StackRoots* stack_roots,
                                intptr_t* stack_end) {
  stack_roots->RecordStackEnd(stack_end);
}

StackRoots::StackRoots(ThreadHeap* heap)
    : heap_(heap),
#if defined(ADDRESS_SANITIZER)
      asan_fake_stack_(__asan_get_current_fake_stack()),
#endif
      start_of_stack_(reinterpret_cast<intptr_t*>(WTF::GetStackStart())),
      end_of_stack_(reinterpret_cast<intptr_t*>(WTF::GetStackStart())) {
}

NO_SANITIZE_ADDRESS
void StackRoots::CopyStackUntilMarker(void* marker) {
  Address* to = reinterpret_cast<Address*>(marker);
  Address* from = reinterpret_cast<Address*>(end_of_stack_);
  CHECK_LT(from, to);
  CHECK_LE(to, reinterpret_cast<Address*>(start_of_stack_));
  size_t slot_count = static_cast<size_t>(to - from);
// Catch potential performance issues.
#if defined(LEAK_SANITIZER) || defined(ADDRESS_SANITIZER)
  // ASan/LSan use more space on the stack and we therefore
  // increase the allowed stack copying for those builds.
  DCHECK_LT(slot_count, 2048u);
#else
  DCHECK_LT(slot_count, 1024u);
#endif

  DCHECK(!stack_copy_.size());
  stack_copy_.resize(slot_count);
  for (size_t i = 0; i < slot_count; ++i) {
    stack_copy_[i] = from[i];
  }
}

// Stack scanning may overrun the bounds of local objects and/or race with
// other threads that use this stack.
NO_SANITIZE_ADDRESS
NO_SANITIZE_THREAD
void StackRoots::Visit(Visitor* visitor) {
  int marker_obj = 1;
  void* marker = &marker_obj;
#ifdef ADDRESS_SANITIZER
  marker = AdjustStackMarkerForAdressSanitizer(marker);
#endif
  PushAllRegisters(nullptr, this, DidPushAllRegisters);
  CopyStackUntilMarker(marker);

  Address* start = reinterpret_cast<Address*>(start_of_stack_);
  // We should stop the stack scanning at the marker to not touch active parts
  // of the stack. Anything interesting beyond that point is in the stack copy.
  Address* current = reinterpret_cast<Address*>(marker);
  if (!current) {
    current = reinterpret_cast<Address*>(end_of_stack_);
  }

  // Ensure that current is aligned by address size otherwise the loop below
  // will read past start address.
  current = reinterpret_cast<Address*>(reinterpret_cast<intptr_t>(current) &
                                       ~(sizeof(Address) - 1));

  for (; current < start; ++current) {
    Address ptr = *current;
#if defined(MEMORY_SANITIZER)
    // |ptr| may be uninitialized by design. Mark it as initialized to keep
    // MSan from complaining.
    // Note: it may be tempting to get rid of |ptr| and simply use |current|
    // here, but that would be incorrect. We intentionally use a local
    // variable because we don't want to unpoison the original stack.
    __msan_unpoison(&ptr, sizeof(ptr));
#endif
    heap_->CheckAndMarkPointer(visitor, ptr);
    VisitAsanFakeStackForPointer(visitor, ptr);
  }

  for (Address ptr : stack_copy_) {
#if defined(MEMORY_SANITIZER)
    // See the comment above.
    __msan_unpoison(&ptr, sizeof(ptr));
#endif
    heap_->CheckAndMarkPointer(visitor, ptr);
    VisitAsanFakeStackForPointer(visitor, ptr);
  }

  stack_copy_.clear();
  marker_ = nullptr;
}

NO_SANITIZE_ADDRESS
void StackRoots::VisitAsanFakeStackForPointer(Visitor* visitor, Address ptr) {
#if defined(ADDRESS_SANITIZER)
  Address* start = reinterpret_cast<Address*>(start_of_stack_);
  Address* end = reinterpret_cast<Address*>(end_of_stack_);
  Address* fake_frame_start = nullptr;
  Address* fake_frame_end = nullptr;
  Address* maybe_fake_frame = reinterpret_cast<Address*>(ptr);
  Address* real_frame_for_fake_frame = reinterpret_cast<Address*>(
      __asan_addr_is_in_fake_stack(asan_fake_stack_, maybe_fake_frame,
                                   reinterpret_cast<void**>(&fake_frame_start),
                                   reinterpret_cast<void**>(&fake_frame_end)));
  if (real_frame_for_fake_frame) {
    // This is a fake frame from the asan fake stack.
    if (real_frame_for_fake_frame > end && start > real_frame_for_fake_frame) {
      // The real stack address for the asan fake frame is
      // within the stack range that we need to scan so we need
      // to visit the values in the fake frame.
      for (Address* p = fake_frame_start; p < fake_frame_end; ++p)
        heap_->CheckAndMarkPointer(visitor, *p);
    }
  }
#endif
}

}  // namespace blink
