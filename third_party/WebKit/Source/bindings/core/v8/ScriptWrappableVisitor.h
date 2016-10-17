// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ScriptWrappableVisitor_h
#define ScriptWrappableVisitor_h

#include "bindings/core/v8/ScopedPersistent.h"
#include "bindings/core/v8/ScriptWrappable.h"
#include "core/CoreExport.h"
#include "platform/RuntimeEnabledFeatures.h"
#include "platform/heap/WrapperVisitor.h"
#include "wtf/Deque.h"
#include "wtf/Vector.h"
#include <v8.h>

namespace blink {

class HeapObjectHeader;
template <typename T>
class Member;

class WrapperMarkingData {
 public:
  friend class ScriptWrappableVisitor;

  WrapperMarkingData(void (*traceWrappersCallback)(const WrapperVisitor*,
                                                   const void*),
                     HeapObjectHeader* (*heapObjectHeaderCallback)(const void*),
                     const void* object)
      : m_traceWrappersCallback(traceWrappersCallback),
        m_heapObjectHeaderCallback(heapObjectHeaderCallback),
        m_rawObjectPointer(object) {
    DCHECK(m_traceWrappersCallback);
    DCHECK(m_heapObjectHeaderCallback);
    DCHECK(m_rawObjectPointer);
  }

  inline void traceWrappers(WrapperVisitor* visitor) {
    if (m_rawObjectPointer) {
      m_traceWrappersCallback(visitor, m_rawObjectPointer);
    }
  }

  /**
     * Returns true when object was marked. Ignores (returns true) invalidated
     * objects.
     */
  inline bool isWrapperHeaderMarked() {
    return !m_rawObjectPointer || heapObjectHeader()->isWrapperHeaderMarked();
  }

  /**
     * Returns raw object pointer. Beware it doesn't necessarily point to the
     * beginning of the object.
     */
  const void* rawObjectPointer() { return m_rawObjectPointer; }

 private:
  inline bool shouldBeInvalidated() {
    return m_rawObjectPointer && !heapObjectHeader()->isMarked();
  }

  inline void invalidate() { m_rawObjectPointer = nullptr; }

  inline const HeapObjectHeader* heapObjectHeader() {
    DCHECK(m_rawObjectPointer);
    return m_heapObjectHeaderCallback(m_rawObjectPointer);
  }

  void (*m_traceWrappersCallback)(const WrapperVisitor*, const void*);
  HeapObjectHeader* (*m_heapObjectHeaderCallback)(const void*);
  const void* m_rawObjectPointer;
};

/**
 * ScriptWrappableVisitor is able to trace through the objects to get all
 * wrappers. It is used during V8 garbage collection.  When this visitor is
 * set to the v8::Isolate as its embedder heap tracer, V8 will call it during
 * its garbage collection. At the beginning, it will call TracePrologue, then
 * repeatedly it will call AdvanceTracing, and at the end it will call
 * TraceEpilogue. Everytime V8 finds new wrappers, it will let the tracer know
 * using RegisterV8References.
 */
class CORE_EXPORT ScriptWrappableVisitor : public WrapperVisitor,
                                           public v8::EmbedderHeapTracer {
 public:
  ScriptWrappableVisitor(v8::Isolate* isolate) : m_isolate(isolate){};
  ~ScriptWrappableVisitor() override;
  /**
     * Replace all dead objects in the marking deque with nullptr after oilpan
     * gc.
     */
  static void invalidateDeadObjectsInMarkingDeque(v8::Isolate*);

  /**
     * Immediately clean up all wrappers.
     */
  static void performCleanup(v8::Isolate*);

  void TracePrologue(v8::EmbedderReachableReferenceReporter*) override;

  static WrapperVisitor* currentVisitor(v8::Isolate*);

  template <typename T>
  static void writeBarrier(const void* object, const Member<T> value) {
    writeBarrier(object, value.get());
  }

  template <typename T>
  static void writeBarrier(const void* object, const T* other) {
    if (!RuntimeEnabledFeatures::traceWrappablesEnabled()) {
      return;
    }
    if (!object || !other) {
      return;
    }
    if (!HeapObjectHeader::fromPayload(object)->isWrapperHeaderMarked()) {
      return;
    }
    HeapObjectHeader* otherObjectHeader =
        TraceTrait<T>::heapObjectHeader(other);
    if (!otherObjectHeader->isWrapperHeaderMarked()) {
      currentVisitor(ThreadState::current()->isolate())
          ->traceWrappers(otherObjectHeader->payload());
    }
  }

  void RegisterV8References(const std::vector<std::pair<void*, void*>>&
                                internalFieldsOfPotentialWrappers) override;
  void RegisterV8Reference(const std::pair<void*, void*>& internalFields);
  bool AdvanceTracing(double deadlineInMs,
                      v8::EmbedderHeapTracer::AdvanceTracingActions) override;
  void TraceEpilogue() override;
  void AbortTracing() override;
  void EnterFinalPause() override;
  size_t NumberOfWrappersToTrace() override;

  void dispatchTraceWrappers(const ScriptWrappable*) const override;
#define DECLARE_DISPATCH_TRACE_WRAPPERS(className) \
  void dispatchTraceWrappers(const className*) const override;

  WRAPPER_VISITOR_SPECIAL_CLASSES(DECLARE_DISPATCH_TRACE_WRAPPERS);

#undef DECLARE_DISPATCH_TRACE_WRAPPERS
  void dispatchTraceWrappers(const void*) const override {}

  void traceWrappers(const ScopedPersistent<v8::Value>*) const override;
  void traceWrappers(const ScopedPersistent<v8::Object>*) const override;
  void markWrapper(const v8::PersistentBase<v8::Value>* handle) const;
  void markWrapper(const v8::PersistentBase<v8::Object>* handle) const override;

  void invalidateDeadObjectsInMarkingDeque();

  void pushToMarkingDeque(
      void (*traceWrappersCallback)(const WrapperVisitor*, const void*),
      HeapObjectHeader* (*heapObjectHeaderCallback)(const void*),
      const void* object) const override {
    m_markingDeque.append(WrapperMarkingData(traceWrappersCallback,
                                             heapObjectHeaderCallback, object));
#if DCHECK_IS_ON()
    if (!m_advancingTracing) {
      m_verifierDeque.append(WrapperMarkingData(
          traceWrappersCallback, heapObjectHeaderCallback, object));
    }
#endif
  }

  bool markWrapperHeader(HeapObjectHeader*) const;
  /**
     * Mark wrappers in all worlds for the given script wrappable as alive in
     * V8.
     */
  void markWrappersInAllWorlds(const ScriptWrappable*) const override;
  void markWrappersInAllWorlds(const void*) const override {}

  WTF::Deque<WrapperMarkingData>* getMarkingDeque() { return &m_markingDeque; }
  WTF::Deque<WrapperMarkingData>* getVerifierDeque() {
    return &m_verifierDeque;
  }
  WTF::Vector<HeapObjectHeader*>* getHeadersToUnmark() {
    return &m_headersToUnmark;
  }

 private:
  /**
     * Is wrapper tracing currently in progress? True if TracePrologue has been
     * called, and TraceEpilogue has not yet been called.
     */
  bool m_tracingInProgress = false;
  /**
     * Is AdvanceTracing currently running? If not, we know that all calls of
     * pushToMarkingDeque are from V8 or new wrapper associations. And this
     * information is used by the verifier feature.
     */
  bool m_advancingTracing = false;

  /**
     * Indicates whether an idle task for a lazy cleanup has already been
     * scheduled.  The flag is used to avoid scheduling multiple idle tasks for
     * cleaning up.
     */
  bool m_idleCleanupTaskScheduled = false;

  /**
     * Indicates whether cleanup should currently happen.
     * The flag is used to avoid cleaning up in the next GC cycle.
     */
  bool m_shouldCleanup = false;

  /**
     * Immediately cleans up all wrappers.
     */
  void performCleanup();

  /**
     * Schedule an idle task to perform a lazy (incremental) clean up of
     * wrappers.
     */
  void scheduleIdleLazyCleanup();
  void performLazyCleanup(double deadlineSeconds);

  /**
     * Collection of objects we need to trace from. We assume it is safe to hold
     * on to the raw pointers because:
     *     * oilpan object cannot move
     *     * oilpan gc will call invalidateDeadObjectsInMarkingDeque to delete
     *       all obsolete objects
     */
  mutable WTF::Deque<WrapperMarkingData> m_markingDeque;
  /**
     * Collection of objects we started tracing from. We assume it is safe to
     * hold on to the raw pointers because:
     *     * oilpan object cannot move
     *     * oilpan gc will call invalidateDeadObjectsInMarkingDeque to delete
     *       all obsolete objects
     *
     * These objects are used when TraceWrappablesVerifier feature is enabled to
     * verify that all objects reachable in the atomic pause were marked
     * incrementally. If not, there is one or multiple write barriers missing.
     */
  mutable WTF::Deque<WrapperMarkingData> m_verifierDeque;
  /**
     * Collection of headers we need to unmark after the tracing finished. We
     * assume it is safe to hold on to the headers because:
     *     * oilpan objects cannot move
     *     * objects this headers belong to are invalidated by the oilpan
     *       gc in invalidateDeadObjectsInMarkingDeque.
     */
  mutable WTF::Vector<HeapObjectHeader*> m_headersToUnmark;
  v8::Isolate* m_isolate;

  /**
     * A reporter instance set in TracePrologue and cleared in TraceEpilogue,
     * which is used to report all reachable references back to v8.
     */
  v8::EmbedderReachableReferenceReporter* m_reporter = nullptr;
};

/**
 * TraceWrapperMember is used for Member fields that should participate in
 * wrapper tracing, i.e., strongly hold a ScriptWrappable alive. All
 * TraceWrapperMember fields must be traced in the class' traceWrappers method.
 */
template <class T>
class TraceWrapperMember : public Member<T> {
  DISALLOW_NEW_EXCEPT_PLACEMENT_NEW();

 public:
  TraceWrapperMember(void* parent, T* raw) : Member<T>(raw), m_parent(parent) {
#if DCHECK_IS_ON()
    DCHECK(!m_parent || HeapObjectHeader::fromPayload(m_parent)->checkHeader());
#endif
    ScriptWrappableVisitor::writeBarrier(m_parent, raw);
  }
  TraceWrapperMember(WTF::HashTableDeletedValueType x)
      : Member<T>(x), m_parent(nullptr) {}

  /**
   * Copying a TraceWrapperMember means that its backpointer will also be
   * copied.
   */
  TraceWrapperMember(const TraceWrapperMember& other) { *this = other; }

  template <typename U>
  TraceWrapperMember& operator=(const TraceWrapperMember<U>& other) {
    DCHECK(other.m_parent);
    m_parent = other.m_parent;
    Member<T>::operator=(other);
    ScriptWrappableVisitor::writeBarrier(m_parent, other);
    return *this;
  }

  template <typename U>
  TraceWrapperMember& operator=(const Member<U>& other) {
    DCHECK(!traceWrapperMemberIsNotInitialized());
    Member<T>::operator=(other);
    ScriptWrappableVisitor::writeBarrier(m_parent, other);
    return *this;
  }

  template <typename U>
  TraceWrapperMember& operator=(U* other) {
    DCHECK(!traceWrapperMemberIsNotInitialized());
    Member<T>::operator=(other);
    ScriptWrappableVisitor::writeBarrier(m_parent, other);
    return *this;
  }

  TraceWrapperMember& operator=(std::nullptr_t) {
    // No need for a write barrier when assigning nullptr.
    Member<T>::operator=(nullptr);
    return *this;
  }

  void* parent() { return m_parent; }

 private:
  bool traceWrapperMemberIsNotInitialized() { return !m_parent; }

  /**
   * The parent object holding strongly onto the actual Member.
   */
  void* m_parent;
};

/**
 * Swaps two HeapVectors specialized for TraceWrapperMember. The custom swap
 * function is required as TraceWrapperMember contains ownership information
 * which is not copyable but has to be explicitly specified.
 */
template <typename T>
void swap(HeapVector<TraceWrapperMember<T>>& a,
          HeapVector<TraceWrapperMember<T>>& b,
          void* parentForA,
          void* parentForB) {
  HeapVector<TraceWrapperMember<T>> temp;
  temp.reserveCapacity(a.size());
  for (auto item : a) {
    temp.append(TraceWrapperMember<T>(parentForB, item.get()));
  }
  a.clear();
  a.reserveCapacity(b.size());
  for (auto item : b) {
    a.append(TraceWrapperMember<T>(parentForA, item.get()));
  }
  b.clear();
  b.reserveCapacity(temp.size());
  for (auto item : temp) {
    b.append(TraceWrapperMember<T>(parentForB, item.get()));
  }
}

/**
 * Swaps two HeapVectors, one containing TraceWrapperMember and one with
 * regular Members. The custom swap function is required as
 * TraceWrapperMember contains ownership information which is not copyable
 * but has to be explicitly specified.
 */
template <typename T>
void swap(HeapVector<TraceWrapperMember<T>>& a,
          HeapVector<Member<T>>& b,
          void* parentForA) {
  HeapVector<TraceWrapperMember<T>> temp;
  temp.reserveCapacity(a.size());
  for (auto item : a) {
    temp.append(TraceWrapperMember<T>(nullptr, item.get()));
  }
  a.clear();
  a.reserveCapacity(b.size());
  for (auto item : b) {
    a.append(TraceWrapperMember<T>(parentForA, item.get()));
  }
  b.clear();
  b.reserveCapacity(temp.size());
  for (auto item : temp) {
    b.append(item.get());
  }
}
}
#endif
