// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CallbackStack_h
#define CallbackStack_h

#include "platform/heap/BlinkGC.h"
#include "platform/heap/HeapPage.h"
#include "platform/wtf/Allocator.h"
#include "platform/wtf/Assertions.h"
#include "platform/wtf/Threading.h"
#include "platform/wtf/LinkedHashSet.h"
#include "platform/wtf/ThreadingPrimitives.h"

namespace blink {

// The CallbackStack contains all the visitor callbacks used to trace and mark
// objects. A specific CallbackStack instance contains at most bufferSize
// elements.
// If more space is needed a new CallbackStack instance is created and chained
// together with the former instance. I.e. a logical CallbackStack can be made
// of multiple chained CallbackStack object instances.
class PLATFORM_EXPORT CallbackStack final {
  USING_FAST_MALLOC(CallbackStack);

 public:
  class Item {
    DISALLOW_NEW();

   public:
    Item() {}
    Item(void* object, VisitorCallback callback)
        : object_(object), callback_(callback) {
          // object_ maybe a Persistent so not appropriate to check header
        }
    void* Object() { return object_; }
    VisitorCallback Callback() { return callback_; }
    void Call(Visitor* visitor) {
      CHECK(object_);
      // object_ maybe a Persistent so not appropriate to check header
      callback_(visitor, object_);
    }

   private:
    void* object_;
    VisitorCallback callback_;
  };

  static std::unique_ptr<CallbackStack> Create();
  ~CallbackStack();

  void Commit();
  void Decommit();

  Item* AllocateEntry();
  Item* Pop();

  bool IsEmpty() const;

  void InvokeEphemeronCallbacks(Visitor*);

#if DCHECK_IS_ON()
  bool HasCallbackForObject(const void*);
#endif
  bool HasJustOneBlock() const;

  static const size_t kMinimalBlockSize;
  static const size_t kDefaultBlockSize = (1 << 13);

 private:
  class Block {
    USING_FAST_MALLOC(Block);

   public:
    explicit Block(Block* next);
    ~Block();

#if DCHECK_IS_ON()
    void Clear();
#endif
    Block* Next() const { return next_; }
    void SetNext(Block* next) { next_ = next; }

    bool IsEmptyBlock() const { return current_ == &(buffer_[0]); }

    size_t BlockSize() const { return block_size_; }

    Item* AllocateEntry() {
      if (LIKELY(current_ < limit_))
        return current_++;
      return nullptr;
    }

    Item* Pop() {
      if (UNLIKELY(IsEmptyBlock()))
        return nullptr;
      return --current_;
    }

    void InvokeEphemeronCallbacks(Visitor*);

#if DCHECK_IS_ON()
    bool HasCallbackForObject(const void*);
#endif

   private:
    size_t block_size_;

    Item* buffer_;
    Item* limit_;
    Item* current_;
    Block* next_;
  };

  CallbackStack();
  Item* PopSlow();
  Item* AllocateEntrySlow();
  void InvokeOldestCallbacks(Block*, Block*, Visitor*);

  Block* first_;
  Block* last_;
  std::set<std::pair<void*, VisitorCallback>> set_;
};

class CallbackStackMemoryPool final {
  USING_FAST_MALLOC(CallbackStackMemoryPool);

 public:
  // 2048 * 8 * sizeof(Item) = 256 KB (64bit) is pre-allocated for the
  // underlying buffer of CallbackStacks.
  static const size_t kBlockSize = 2048;
  static const size_t kPooledBlockCount = 8;
  static const size_t kBlockBytes = kBlockSize * sizeof(CallbackStack::Item);

  static CallbackStackMemoryPool& Instance();

  void Initialize();
  CallbackStack::Item* Allocate();
  void Free(CallbackStack::Item*);

 private:
  Mutex mutex_;
  int free_list_first_;
  int free_list_next_[kPooledBlockCount];
  CallbackStack::Item* pooled_memory_;
};

ALWAYS_INLINE CallbackStack::Item* CallbackStack::AllocateEntry() {
  DCHECK(first_);
  Item* item = first_->AllocateEntry();
  if (LIKELY(!!item))
    return item;

  return AllocateEntrySlow();
}

ALWAYS_INLINE CallbackStack::Item* CallbackStack::Pop() {
  Item* item = first_->Pop();
  if (!item)
    item = PopSlow();
  if (item) {
    auto it = set_.find(std::pair<void*, VisitorCallback>(item->Object(), item->Callback()));
    set_.erase(it);
  }
  return item;
}

class NewCallbackStack {
public:
  NewCallbackStack() {
    CHECK(callbacks_.IsEmpty());
  }
  void Commit() {
    if (!callbacks_.IsEmpty()) {
      LOG(ERROR) << "NewCallbackStack::Commit error not empty " << static_cast<void*>(this);
    }
    CHECK(callbacks_.IsEmpty());
  }
  void Decommit() {
    callbacks_.clear();
    CHECK(callbacks_.IsEmpty());
  }
  // True if an new entry was added.
  void Register(void* object, VisitorCallback callback) {
    LOG(ERROR) << "NewCallbackStack::Register " << static_cast<void*>(this);
    callbacks_.insert(std::pair<void*, VisitorCallback>(object, callback));
  }
  void UnregisterForObject(void* object) {
    //LOG(ERROR) << "NewCallbackStack::UnregisterForObject " << static_cast<void*>(this) << " object " << object << " callbacks_.size() " << callbacks_.size();
    LinkedHashSet<std::pair<void*, VisitorCallback>> x;
    callbacks_.Swap(x);
    for (auto it : x) {
      if (it.first == object)
        continue;
      callbacks_.insert(it);
    }
    //LOG(ERROR) << "NewCallbackStack::UnregisterForObject END " << static_cast<void*>(this) << " object " << object << " callbacks_.size() " << callbacks_.size();
  }
  bool IsEmpty() {
    return callbacks_.IsEmpty();
  }
#if DCHECK_IS_ON()
  bool HasCallbackForObject(const void* object) {
    for (auto it : callbacks_) {
      if (it.first == object)
        return true;
    }
    return false;
  }
#endif
  std::pair<void*, VisitorCallback> Pop() {
    if (callbacks_.IsEmpty())
      return std::pair<void*, VisitorCallback>(nullptr, nullptr);
    std::pair<void*, VisitorCallback> p = callbacks_.back();
    callbacks_.pop_back();
    return p;
  }
  bool PopAndInvoke(Visitor* visitor) {
    std::pair<void*, VisitorCallback> p = Shift();//Pop();
    if (!p.first)
      return false;
    p.second(visitor, p.first);
    return true;
  }
  std::pair<void*, VisitorCallback> Shift() {
    if (callbacks_.IsEmpty())
      return std::pair<void*, VisitorCallback>(nullptr, nullptr);
    std::pair<void*, VisitorCallback> p = callbacks_.front();
    callbacks_.RemoveFirst();
    return p;
  }
  void InvokeEphemeronCallbacks(Visitor* visitor) {
    LinkedHashSet<std::pair<void*, VisitorCallback>> x(callbacks_);
    while (!x.IsEmpty()) {
      std::pair<void*, VisitorCallback> p = x.front();
      x.RemoveFirst();
      if (!p.first) {
        // FIXME: must call new callbacks
        break;
      }
      p.second(visitor, p.first);
    }
  }
private:
  LinkedHashSet<std::pair<void*, VisitorCallback>> callbacks_;
};

}  // namespace blink

#endif  // CallbackStack_h
