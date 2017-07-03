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

template <typename T>
class WeakMemberSetConstIterator {};

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
    size_t max_size_ = 0;
    size_t max_capacity_ = 0;
  };

  class WeakMemberSetIterator {
    STACK_ALLOCATED();
    DISALLOW_ASSIGN(WeakMemberSetIterator);

   public:
    WeakMemberSetIterator(const ImplType& buf,
                          const size_t cur,
                          const size_t sz)
        : buf_(buf), cur_(cur), size_(sz) {}

    ~WeakMemberSetIterator() {}

    inline bool operator==(const WeakMemberSetIterator& rhs) const {
      return (buf_.Buffer() == rhs.buf_.Buffer()) && (cur_ == rhs.cur_);
    }

    inline bool operator!=(const WeakMemberSetIterator& rhs) const {
      return (buf_.Buffer() != rhs.buf_.Buffer()) || (cur_ != rhs.cur_);
    }

    WeakMemberSetIterator& operator++() {
      DCHECK_NE(cur_, size_);
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
      return size_;
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

  bool erase(const T* what, std::true_type const&) {
    // This is used when T supports true equality.
    if (what == nullptr)
      return false;
    for (auto it = raw_begin(); it != raw_end(); ++it) {
      if ((*it).Get() == what) {
        *it = nullptr;
        requires_compaction_ = true;
        release_capacity_ = true;
        return true;
      }
    }
    for (auto it = raw_begin(); it != raw_end(); ++it) {
      if (*((*it).Get()) == *what) {
        *it = nullptr;
        requires_compaction_ = true;
        release_capacity_ = true;
        return true;
      }
    }
    return false;
  }

  bool erase(const T* what, std::false_type const&) {
    if (what == nullptr)
      return false;
    for (auto it = raw_begin(); it != raw_end(); ++it) {
      if ((*it).Get() == what) {
        *it = nullptr;
        requires_compaction_ = true;
        release_capacity_ = true;
        return true;
      }
    }
    return false;
  }

  bool erase(const T* what) {
#ifdef WEAK_MEMBER_SET_DUMPSTATS
    stats_.erased_++;
#endif
    return erase(what, HasEqualityOverload<T, T>());
  }

  bool Contains(const T* what, std::true_type const&) const {
    // Contains implementation when something overloads ==
    if (what == nullptr)
      return false;
    for (auto it = const_raw_begin(); it != const_raw_end(); ++it) {
      if (*it == nullptr) {
        continue;
      }
      if ((*it).Get() == what || *((*it).Get()) == *what) {
        return true;
      }
    }
    return false;
  }

  bool Contains(const T* what, std::false_type const&) const {
    // Contains implementation when something doesn't overload ==
    // so just go on the pointer value.
    if (what == nullptr)
      return false;
    for (auto it = const_raw_begin(); it != const_raw_end(); ++it) {
      if (*it == nullptr) {
        continue;
      }
      if ((*it).Get() == what) {
        return true;
      }
    }
    return false;
  }

#ifdef WEAK_MEMBER_SET_DUMPSTATS
  bool Contains(const T* what) {
    stats_.contains_++;
    return Contains(what, HasEqualityOverload<T, T>());
  }
#else
  bool Contains(const T* what) const {
    return Contains(what, HasEqualityOverload<T, T>());
  }
#endif

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
      if (((size_ + insertion_head_) << 2) >=
          ((old_capacity << 1) + old_capacity))
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

    if (requires_compaction_) {
      for (auto it = raw_begin() + insertion_head_; it != raw_end(); ++it) {
        if (*it == nullptr || (*it).Get() == nullptr) {
          *it = val;
          return;
        }
      }
    }

    if (LIKELY(size() != capacity())) {
      WeakMember<T>* cur = nullptr;
#ifdef WEAK_MEMBER_SET_TRACING
      LOG(INFO) << this << " insert::insertion_point is " << insertion_head_;
#endif
      if (insertion_head_ > 0) {
#ifdef WEAK_MEMBER_SET_TRACING
        LOG(INFO) << this << " insert:: prepending at " << insertion_head_ - 1;
#endif
        insertion_head_--;
        cur = raw_begin();
      } else {
#ifdef WEAK_MEMBER_SET_TRACING
        LOG(INFO) << this << " insert:: appending";
#endif
        cur = raw_end();
      }
      ANNOTATE_CHANGE_SIZE(buf_.Buffer(), capacity(), size_, size_ + 1);
      new (NotNull, cur) WeakMember<T>(val);
      ++size_;
      return;
    }

    AppendSlowCase(val);
  }

  void AppendSlowCase(T* val) {
    DCHECK_EQ(size(), capacity());
#ifdef WEAK_MEMBER_SET_TRACING
    LOG(INFO) << this << "insert:: appending (slow case)";
#endif

    ExpandCapacity(size() + 1);
    DCHECK(raw_begin());

    ANNOTATE_CHANGE_SIZE(buf_.Buffer(), capacity(), size_, size_ + 1);
    new (NotNull, raw_end()) WeakMember<T>(val);
    ++size_;
  }

  void Finalize() { LOG(INFO) << "Finalize"; }

  void Dispose() {
    LOG(INFO) << "Dispose";
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
    auto ret =
        WeakMemberSetIterator(buf_, insertion_head_, size_ + insertion_head_);
    // Can re-enable GC here under extremely conservative assumptions.
    return ret;
  }
  iterator end() {
    return WeakMemberSetIterator(buf_, insertion_head_ + size_,
                                 size_ + insertion_head_);
  }

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
      stats_.at_no_compact_++;
#endif
    }
    WeakMember<T>& ret = buf_.Buffer()[i];
    LEAVE_GC_FORBIDDEN();
    return ret;
  }

  void clearWeakMembers(Visitor* visitor) {
    bool req = requires_compaction_;
    for (size_t i = insertion_head_; i < size_ + insertion_head_; i++) {
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

  // TrimStart winds the insertion_head_ forwards over collected elements.
  // Inserted items can then be prepended over collected items.
  inline void TrimStart() {
    ENTER_GC_FORBIDDEN();
#ifdef WEAK_MEMBER_SET_TRACING
    size_t old_size = size_;
    size_t old_insert = insertion_head_;
#endif
    auto cur = raw_begin();
    auto stop = raw_end();
    while (stop != cur) {
      // Seek until the insertion head starts to become alive.
      if (*cur != nullptr)
        break;
      insertion_head_++;
      ++cur;
      size_--;
    }
#ifdef WEAK_MEMBER_SET_TRACING
    LOG(INFO) << this << " precompaction pass took size from " << old_size
              << " to " << size_;
    LOG(INFO) << this << " precompaction moved insertion_head_ from "
              << old_insert << " to " << insertion_head_;
#endif
    LEAVE_GC_FORBIDDEN();
  }

  inline WeakMember<T>* CompactionPass(WeakMember<T>* base,
                                       WeakMember<T>* stop) {
    ENTER_GC_FORBIDDEN();
    // Seek the insertion head until it's no longer alive.
    size_t cur = 0;
    while (true) {
      if (base + cur >= stop)
        break;
      if (*(base + cur) == nullptr)
        break;
      ++cur;
    }
    auto insert_head = cur;

    // Seek cur until it starts to become alive.
    while (true) {
      if (base + cur >= stop)
        break;
      if (*(base + cur) != nullptr)
        break;
      ++cur;
    }

    auto insert_tail = cur - 1;
    auto move_head = cur;

    // Seek cur until it stops being alive to find the maximum move.
    while (true) {
      if (base + cur >= stop)
        break;
      if (*(base + cur) == nullptr)
        break;
      ++cur;
    }

    auto move_tail = cur;

    if (base + move_tail >= stop) {
      requires_compaction_ = false;
    }

    // Indicates that we've completed compaction.
    if (insert_head == cur && base + cur >= stop) {
      LEAVE_GC_FORBIDDEN();
      return nullptr;
    }

    // If the size of [insert_head, insert_tail] is greater than
    // the size of [move_start, move_tail], then it's an overlapping move.
    if (move_tail - move_head > insert_tail - insert_head + 1) {
#ifdef WEAK_MEMBER_SET_TRACING
      LOG(INFO) << this << " move-overlapping " << move_head << " " << move_tail
                << " to " << insert_head;
#endif

      WTF::VectorMover<true, WeakMember<T>>::MoveOverlapping(
          base + move_head, base + move_tail, base + insert_head);
      // Compute the region to clear from.
      auto shift = move_head - insert_head;
      auto clear_from = move_tail - shift;
      size_ -= clear_from;
      size_ = base + clear_from - raw_begin();
      WTF::VectorInitializer<true, WeakMember<T>>::Initialize(base + clear_from,
                                                              base + move_tail);
      LEAVE_GC_FORBIDDEN();
      return base + clear_from;
    } else {
#ifdef WEAK_MEMBER_SET_TRACING
      LOG(INFO) << this << " move " << move_head << " " << move_tail << " to "
                << insert_head << " " << insert_tail << " size_ = " << size_;
#endif
      // We're moving the region [cur, tail) into a smaller slot.
      WTF::VectorMover<true, WeakMember<T>>::Move(
          base + move_head, base + move_tail, base + insert_head);
      auto move_size = move_tail - move_head;
#ifdef WEAK_MEMBER_SET_TRACING
      auto region_size = insert_tail + 1 - insert_head;
      LOG(INFO) << this << " size_ = " << size_
                << " region_size = " << region_size
                << " move_size = " << move_size;
      LOG(INFO) << this << " base =" << base + insert_head + move_size
                << " raw_begin = " << raw_begin()
                << " size = " << sizeof(WeakMember<T>);
      LOG(INFO) << this << " base + insert_head + move_size - raw_begin() = "
                << (base + insert_head + move_size) - raw_begin();
#endif
      size_ = base + insert_head + move_size - raw_begin();
      // Blank out the elements that we don't need.
      WTF::VectorInitializer<true, WeakMember<T>>::Initialize(base + move_head,
                                                              base + move_tail);
      LEAVE_GC_FORBIDDEN();
      return base + insert_head + move_size;
    }
  }

  void compact() {
#ifdef WEAK_MEMBER_SET_DUMPSTATS
    stats_.compactions_++;
#endif

    this->TrimStart();
    auto cur = raw_begin();
    auto end = raw_end();
    while (requires_compaction_) {
#ifdef WEAK_MEMBER_SET_TRACING
      size_t old_size = size_;
#endif
      cur = this->CompactionPass(raw_begin(), end);
#ifdef WEAK_MEMBER_SET_TRACING
      LOG(INFO) << "compact pass to size_ from " << old_size << " to " << size_;
#endif
    }
    if (release_capacity_) {
      ReserveCapacity(size_ + insertion_head_);
      release_capacity_ = false;
    }
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
    std::swap_ranges(absolute_raw_begin(), absolute_raw_begin() + max_size,
                     with.absolute_raw_begin());

    // Swap over the size values.
    size_t size_tmp = size_;
    size_ = with.size_;
    with.size_ = size_tmp;

    // Swap over whether the list needs compaction.
    bool compact_tmp = requires_compaction_;
    requires_compaction_ = with.requires_compaction_;
    with.requires_compaction_ = compact_tmp;

    // Swap over the insertion offset.
    size_t insert_tmp = insertion_head_;
    insertion_head_ = with.insertion_head_;
    with.insertion_head_ = insert_tmp;

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
  FRIEND_TEST_ALL_PREFIXES(WeakMemberSetTest, CompactionPassSmallerOriginSlot);
  FRIEND_TEST_ALL_PREFIXES(WeakMemberSetTest, CompactionPassEqualOriginSlot);
  FRIEND_TEST_ALL_PREFIXES(WeakMemberSetTest, CompactionPassLargerOriginSlot);
  FRIEND_TEST_ALL_PREFIXES(WeakMemberSetTest, TestCompactionPassAfterErase);

  WeakMember<T>* absolute_raw_begin() { return buf_.Buffer(); }

  WeakMember<T>* raw_begin() { return buf_.Buffer() + insertion_head_; }
  WeakMember<T>* raw_end() { return buf_.Buffer() + insertion_head_ + size_; }
  const WeakMember<T>* const_raw_begin() const {
    return buf_.Buffer() + insertion_head_;
  }
  const WeakMember<T>* const_raw_end() const {
    return buf_.Buffer() + insertion_head_ + size_;
  }

  // In lazy compaction cases, we can prepend elements.
  size_t insertion_head_ = 0;

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
