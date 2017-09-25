// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WeakMemberSet_h
#define WeakMemberSet_h

#include <base/debug/stack_trace.h>
#include <base/gtest_prod_util.h>
#include "platform/heap/Handle.h"
#include "platform/heap/Heap.h"
#include "platform/wtf/Vector.h"

#include <algorithm>

namespace blink {

// You can enable via tracing via #define WEAK_MEMBER_SET_TRACING
// WEAK_MEMBER_SET_CAREFUL_GC switches on and off garbage collection at
// certain points, at the cost of some performance.
// WEAK_MEMBER_SET_DUMPSTATS gathers and prints statistics on the number
// of elements in the set and the number of times each operation has
// been done at each garbage collection.

#ifdef WEAK_MEMBER_SET_CAREFUL_GC
#define REALLY_ENTER_GC_FORBIDDEN() Allocator::EnterGCForbiddenScope()
#define REALLY_LEAVE_GC_FORBIDDEN() Allocator::LeaveGCForbiddenScope()
#define ENTER_GC_FORBIDDEN() Allocator::EnterGCForbiddenScope()
#define LEAVE_GC_FORBIDDEN() Allocator::LeaveGCForbiddenScope()
#else
#define REALLY_ENTER_GC_FORBIDDEN() Allocator::EnterGCForbiddenScope()
#define REALLY_LEAVE_GC_FORBIDDEN() Allocator::LeaveGCForbiddenScope()
#define ENTER_GC_FORBIDDEN()
#define LEAVE_GC_FORBIDDEN()
#endif

template <class T, class EqualTo>
struct HasEqualityOverloadImpl {
  template <class U, class V>
  static auto test(U*) -> decltype(std::declval<U>() == std::declval<V>());
  template <typename, typename>
  static auto test(...) -> std::false_type;

  using type = typename std::is_same<bool, decltype(test<T, EqualTo>(0))>::type;
};

template <class T, class EqualTo = T>
struct HasEqualityOverload : HasEqualityOverloadImpl<T, EqualTo>::type {};

template <class T>
struct WeakMemberLessThan {
  bool operator()(const WeakMember<T>& lhs, const WeakMember<T>& rhs) const {
    return lhs.Get() < rhs.Get();
  }

  bool operator()(const WeakMember<T>* lhs, const WeakMember<T>* rhs) const {
    uintptr_t lhsu = 0;
    uintptr_t rhsu = 0;

    if (lhs != nullptr)
      lhsu = (uintptr_t)lhs->Get();
    if (rhs != nullptr)
      rhsu = (uintptr_t)rhs->Get();
    return lhsu < rhsu;
  }
};

// Stores an unordered collection of WeakMember<T> references, compacting them
// after garbage collection. Enable WEAK_MEMBER_SET_DUMPSTATS to check.
//
// Only use this data structure in the following contexts:
//  1) Iteration performance is critical, i.e. situations where something is
//     iterated over hundreds of thousands of times.
//  2) The number of elements is relatively small (membership check is O(n)),
//     typically about 300 elements.
//  3) The number of deletions is relatively small (deletion is O(n)), typically
//     less than 2000 over the lifetime of the page.
//  4) The number of insertions is relatively small (may cause a O(n) copy and
//     reallocation if the inlineCapacity is exceeded), typically in the 1000s.
template <typename T, size_t inlineCapacity = 1>
class WeakMemberSet
    : public GarbageCollectedFinalized<WeakMemberSet<T, inlineCapacity>> {
 public:
  using Allocator = HeapAllocator;
  using ImplType = WTF::VectorBuffer<WeakMember<T>, inlineCapacity, Allocator>;

  static WeakMemberSet* Create() { return new WeakMemberSet(); }
  class WeakMemberSetStatistics {
   public:
    inline void dump() {
      LOG(INFO) << this << " * BEGIN " << __PRETTY_FUNCTION__
                << " statistics dump *";
      LOG(INFO) << this << " erased_ " << erased_;
      LOG(INFO) << this << " inserted_ " << inserted_;
      LOG(INFO) << this << " iterated_ " << iterated_ << " (with compaction) "
                << iterated_with_compact_ << " (without) "
                << iterated_no_compact_;
      LOG(INFO) << this << " contains_ " << contains_;
      LOG(INFO) << this << " at_" << at_;
      LOG(INFO) << this << " reallocated_ " << allocated_ << ", deallocated_ "
                << deallocated_;
      LOG(INFO) << this << " traced_ " << traced_;
      LOG(INFO) << this << " compactions_ " << compactions_;
      LOG(INFO) << this << " max_size_ " << max_size_;
      LOG(INFO) << this << " max_capacity_ " << max_capacity_;
      LOG(INFO) << this << " * END WeakMemberSet statistics dump *";
    }
    size_t at_ = 0;
    size_t at_with_compact_ = 0;
    size_t at_no_compact_ = 0;
    size_t erased_ = 0;
    size_t contains_ = 0;
    size_t inserted_ = 0;
    size_t compactions_ = 0;
    size_t iterated_ = 0;
    size_t iterated_with_compact_ = 0;
    size_t iterated_no_compact_ = 0;
    size_t allocated_ = 0;
    size_t deallocated_ = 0;
    size_t traced_ = 0;
    int max_size_ = 0;
    size_t max_capacity_ = 0;
  };

  class WeakMemberSetIterator {
    STACK_ALLOCATED();
    DISALLOW_ASSIGN(WeakMemberSetIterator);

   public:
    WeakMemberSetIterator(const ImplType& buf,
                          const size_t cur,
                          const size_t sz)
        : buf_(buf), cur_(cur), size_(sz) {
      __builtin_prefetch(buf.Buffer(), 0, 0);
    }

    ~WeakMemberSetIterator() {}

    inline bool operator==(const WeakMemberSetIterator& rhs) const {
      return (buf_.Buffer() == rhs.buf_.Buffer()) && (cur_ == rhs.cur_);
    }

    inline bool operator!=(const WeakMemberSetIterator& rhs) const {
      return (buf_.Buffer() != rhs.buf_.Buffer()) || (cur_ != rhs.cur_);
    }

    WeakMemberSetIterator& operator++() {
      DCHECK_NE(cur_, size_);
      if (cur_ < size_)
        __builtin_prefetch(buf_.Buffer() + cur_, 0, 0);
      ++cur_;
      return *this;
    }

    inline const WeakMember<T>& operator*() const { return Get(); }
    inline const WeakMember<T>* operator&() const {
      return buf_.Buffer() + cur_;
    }

    const WeakMember<T>& Get() const { return buf_.Buffer()[cur_]; }

   private:
    const ImplType& buf_;
    size_t cur_;
    const size_t size_;
  };

  using ValueType = WeakMember<T>;
  using iterator = WeakMemberSetIterator;

  using TypeOperations = WTF::VectorTypeOperations<WeakMember<T>>;

  WeakMemberSet() : size_(0) {}

  size_t size() const {
    if (!requires_compaction_) {
#ifdef WEAK_MEMBER_SET_TRACING
      LOG(INFO) << "size_ = " << size_;
#endif
      return ((size_t)size_);
    }
    size_t ret = 0;
    for (auto i = const_raw_begin(); i != const_raw_end(); i++) {
      if (*i != nullptr) {
        ret++;
      }
    }
#ifdef WEAK_MEMBER_SET_TRACING
    LOG(INFO) << "calculated size = " << size_;
#endif
    return ret;
  }

  size_t capacity() const { return buf_.capacity(); }

  bool erase(const T* what) {
#ifdef WEAK_MEMBER_SET_DUMPSTATS
    stats_.erased_++;
#endif

    WeakMember<T>* needle = find(what);
    if (needle == nullptr)
      return false;

    if (needle != raw_end()) {
      WTF::VectorMover<true, WeakMember<T>>::MoveOverlapping(needle + 1,
                                                             raw_end(), needle);
    }
    size_--;
    return true;
  }

  bool ContainsValue(const T* what) {
    // Contains() only operates on pointers, this checks the value.
    if (what == nullptr)
      return false;
    for (auto it = begin(); it != end(); ++it) {
      if (*it == nullptr)
        continue;
      if ((*it).Get() == what || *((*it).Get()) == *what) {
        return true;
      }
    }
    return false;
  }

  bool Contains(const T* what) {
#ifdef WEAK_MEMBER_SET_DUMPSTATS
    stats_.contains_++;
#endif

    return find(what) != nullptr;
  }

  void ReserveCapacity(size_t new_capacity) {
#ifdef WEAK_MEMBER_SET_DUMPSTATS
    if (stats_.max_capacity_ < new_capacity)
      stats_.max_capacity_ = new_capacity;
#endif
    size_t old_capacity = capacity();
    // Decide if we want to release any memory back to the heap
    // (e.g. if an animation has completed).
    if (UNLIKELY(new_capacity <= old_capacity)) {
      // Allow up to 25% wastage at the back (this does size/c < 0.75) before
      // we start to reclaim memory.
      if ((size_ << 2) >= ((old_capacity << 1) + old_capacity))
        return;
      // Can't go below or equal inlineCapacity otherwise the
      // routine won't release any memory.
      new_capacity =
          std::max(static_cast<size_t>(inlineCapacity + 1), new_capacity);
      if (new_capacity >= capacity()) {
        return;
      }
#ifdef WEAK_MEMBER_SET_TRACING
      LOG(INFO) << this << " decreasing capacity from " << old_capacity
                << " to " << new_capacity;
      base::debug::StackTrace().Print();
#endif
#ifdef WEAK_MEMBER_SET_DUMPSTATS
      stats_.deallocated_++;
#endif
      buf_.ShrinkBuffer(new_capacity);
    }
#ifdef WEAK_MEMBER_SET_DUMPSTATS
    stats_.allocated_++;
#endif
#ifdef WEAK_MEMBER_SET_TRACING
    LOG(INFO) << this << " increasing capacity from " << old_capacity << " to "
              << new_capacity;
    base::debug::StackTrace().Print();
#endif
    REALLY_ENTER_GC_FORBIDDEN();
    if (!raw_begin()) {
      LEAVE_GC_FORBIDDEN();
      buf_.AllocateBuffer(new_capacity);
      return;
    }
    if (Allocator::kIsGarbageCollected && buf_.ExpandBuffer(new_capacity)) {
      REALLY_LEAVE_GC_FORBIDDEN();
      return;
    }
#ifdef WEAK_MEMBER_SET_TRACING
    LOG(INFO) << this << " falling back to swap";
    base::debug::StackTrace().Print();
#endif

    // Allocate a new buffer, copy the old buffer over, deallocate.
    CHECK(!Allocator::IsObjectResurrectionForbidden());

    auto old_start = raw_begin();
    auto old_end = raw_end();
    buf_.AllocateExpandedBuffer(new_capacity);

    if (old_start != raw_begin()) {
      TypeOperations::Move(old_start, old_end, raw_begin());
      // Imitate ClearUnusedSlots.
      WTF::VectorUnusedSlotClearer<true, WeakMember<T>>::Clear(old_start,
                                                               old_end);
      buf_.DeallocateBuffer(old_start);
    }
    REALLY_LEAVE_GC_FORBIDDEN();
  }

  void ExpandCapacity(const size_t new_min_capacity) {
    size_t old_capacity = capacity();
    size_t expanded_capacity = old_capacity;
    expanded_capacity += (expanded_capacity >> 2) + 1;
    ReserveCapacity(std::max(
        new_min_capacity, std::max(static_cast<size_t>(WTF::kInitialVectorSize),
                                   expanded_capacity)));
  }

  void insert(T* val) {
    if (Contains(val)) {
      return;
    }
    DCHECK(Allocator::IsAllocationAllowed());
#ifdef WEAK_MEMBER_SET_DUMPSTATS
    stats_.inserted_++;
    if (size_ > stats_.max_size_)
      stats_.max_size_ = size_;
#endif

    if (UNLIKELY(size() == capacity())) {
      ExpandCapacity(size() + 1);
      DCHECK(raw_begin());

      ANNOTATE_CHANGE_SIZE(buf_.Buffer(), capacity(), size_, size_ + 1);
    }

    // Find the first element which is greater than the thing we're inserting.
    auto insert_point = std::upper_bound(raw_begin(), raw_end(), val);

    // Shift the first element up
    WTF::VectorMover<true, WeakMember<T>>::MoveOverlapping(
        insert_point, raw_end(), insert_point + 1);

    new (NotNull, insert_point) WeakMember<T>(val);
    size_++;
  }

  void Finalize() {
#ifdef WEAK_MEMBER_SET_TRACING
    LOG(INFO) << "Finalize";
#endif
  }

  void Dispose() {
#ifdef WEAK_MEMBER_SET_TRACING
    LOG(INFO) << "Dispose";
#endif
    buf_.Destruct();
    size_ = 0;
  }

  inline iterator begin() {
#ifdef WEAK_MEMBER_SET_DUMPSTATS
    stats_.iterated_++;
#endif
    // Need to disable GC here under extremely conservative assumptions.
    if (requires_compaction_) {
#ifdef WEAK_MEMBER_SET_DUMPSTATS
      stats_.iterated_with_compact_++;
#endif
      compact();
    } else {
#ifdef WEAK_MEMBER_SET_DUMPSTATS
      stats_.iterated_no_compact_++;
#endif
    }
    auto ret = WeakMemberSetIterator(buf_, 0, size_);
    // Can re-enable GC here under extremely conservative assumptions.
    return ret;
  }
  iterator end() { return WeakMemberSetIterator(buf_, size_, size_); }

  WeakMember<T>& at(size_t i) {
    ENTER_GC_FORBIDDEN();
#ifdef WEAK_MEMBER_SET_DUMPSTATS
    stats_.at_++;
#endif
    if (requires_compaction_) {
      compact();
#ifdef WEAK_MEMBER_SET_DUMPSTATS
      stats_.at_with_compact_++;
#endif
    } else {
#ifdef WEAK_MEMBER_SET_DUMPSTATS
#endif
    }
    WeakMember<T>& ret = buf_.Buffer()[i];
    LEAVE_GC_FORBIDDEN();
    return ret;
  }

  void clearWeakMembers(Visitor* visitor) {
    bool req = requires_compaction_;
    for (size_t i = 0; i < size_; i++) {
      if (buf_.Buffer()[i] == nullptr) {
        req = true;
        continue;
      }
      if (buf_.Buffer()[i].Get() == nullptr) {
        req = true;
        continue;
      }
      if (!ThreadHeap::IsHeapObjectAlive(*(buf_.Buffer() + i))) {
        buf_.Buffer()[i] = nullptr;
        req = true;
        continue;
      }
    }
    requires_compaction_ = req;
  }

  void compact() {
    if (!requires_compaction_) {
      return;
    }

#ifdef WEAK_MEMBER_SET_DUMPSTATS
    stats_.compactions_++;
#endif

    if (size_ == 0) {
      return;
    }

    auto cmp = WeakMemberLessThan<T>();

    // Sort everything in ascending order
    ENTER_GC_FORBIDDEN();
    std::sort(raw_begin(), raw_end(), cmp);

    // Wind the head of the list until we come to a live element
    auto start = raw_begin();
    auto cur = raw_begin();
    auto stop = raw_end();
    int collected_elements = 0;
    while (stop != cur) {
      if (*cur != nullptr)
        break;
      ++cur;
      ++collected_elements;
    }
    size_ -= collected_elements;

    // Move everything in the list forward, over the compacted elements
    WTF::VectorMover<true, WeakMember<T>>::MoveOverlapping(cur, stop, start);

    requires_compaction_ = false;

    if (release_capacity_) {
      ReserveCapacity(size_);
      release_capacity_ = false;
    }
    LEAVE_GC_FORBIDDEN();
  }

  DEFINE_INLINE_TRACE() {
#ifdef WEAK_MEMBER_SET_TRACING
    LOG(INFO) << "WeakMemberSet::Trace";
#endif
#ifdef WEAK_MEMBER_SET_DUMPSTATS
    stats_.traced_++;
    stats_.dump();
#endif
    visitor->template RegisterWeakMembers<
        WeakMemberSet<T, inlineCapacity>,
        &WeakMemberSet<T, inlineCapacity>::clearWeakMembers>(this);

    if (buf_.HasOutOfLineBuffer()) {
      if (Allocator::IsHeapObjectAlive(buf_.Buffer()))
        return;
      Allocator::MarkNoTracing(visitor, buf_.Buffer());
      Allocator::RegisterBackingStoreReference(visitor, buf_.BufferSlot());
    }
  }

  void swap(WeakMemberSet& with) {
    // Figure out the minimum number of elements we can directly swap.
    REALLY_ENTER_GC_FORBIDDEN();

    size_t max_size = capacity();
    if (with.capacity() > max_size)
      max_size = with.capacity();

    with.ReserveCapacity(max_size);
    ReserveCapacity(max_size);

    // Swap over the raw arrays.
    std::swap_ranges(raw_begin(), raw_begin() + max_size, with.raw_begin());

    // Swap over the size values.
    size_t size_tmp = size_;
    size_ = with.size_;
    with.size_ = size_tmp;

    // Swap over whether the list needs compaction.
    bool compact_tmp = requires_compaction_;
    requires_compaction_ = with.requires_compaction_;
    with.requires_compaction_ = compact_tmp;

#ifdef WEAK_MEMBER_SET_DUMPSTATS
    WeakMemberSetStatistics stats_tmp = stats_;
    stats_ = with.stats_;
    with.stats_ = stats_tmp;
#endif

    REALLY_LEAVE_GC_FORBIDDEN();
  }

 private:
  FRIEND_TEST_ALL_PREFIXES(WeakMemberSetTest,
                           PrecompactShouldLazilyRemoveElementsAtStart);

  WeakMember<T>* raw_begin() { return buf_.Buffer(); }
  WeakMember<T>* raw_end() { return buf_.Buffer() + size_; }
  const WeakMember<T>* const_raw_begin() const { return buf_.Buffer(); }
  const WeakMember<T>* const_raw_end() const { return buf_.Buffer() + size_; }

  WeakMember<T>* find(const T* finding) {
    if (finding == nullptr)
      return nullptr;
    if (size_ == 0)
      return nullptr;
    if (requires_compaction_)
      compact();

    auto found = std::lower_bound(raw_begin(), raw_end(), finding);

    if (found == raw_end())
      return nullptr;

    if (found->Get() == finding)
      return found;

    return nullptr;
  }

  bool requires_compaction_ = false;
  bool release_capacity_ = false;
  ImplType buf_;
  size_t size_;
#ifdef WEAK_MEMBER_SET_DUMPSTATS
  WeakMemberSetStatistics stats_;
#endif
};

}  // namespace blink

#endif  // WeakMemberSet_h
