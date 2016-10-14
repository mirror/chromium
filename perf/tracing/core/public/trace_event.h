// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PERF_TRACING_CORE_PUBLIC_TRACE_EVENT_H_
#define PERF_TRACING_CORE_PUBLIC_TRACE_EVENT_H_

#include "perf/tracing/core/macros.h"
#include "perf/tracing/core/public/trace_event_handle.h"
#include "perf/tracing/platform_light.h"

// This will mark the trace event as disabled by default. The user will need
// to explicitly enable the event.
#define TRACE_DISABLED_BY_DEFAULT(name) "disabled-by-default-" name

#define TRACE_INTERNAL_SHOULD_ADD_EVENT(category_enabled)                    \
  (TRACE_UNLIKELY(*(category_enabled) & (::tracing::ENABLED_FOR_RECORDING |  \
                                         ::tracing::ENABLED_FOR_ETW_EXPORT | \
                                         ::tracing::ENABLED_FOR_FILTERING)))

// Give a unique name to variabels to prevent name collisions.
#define TRACE_UNIQUE_NAME3(a, b) trace_event_unique_##a##b
#define TRACE_UNIQUE_NAME2(a, b) TRACE_UNIQUE_NAME3(a, b)
#define TRACE_UNIQUE_NAME(name_prefix) \
  TRACE_UNIQUE_NAME2(name_prefix, __LINE__)

// Records a pair of begin and end events called "name" for the current
// scope, with 0, 1 or 2 associated arguments. If the category is not
// enabled, then this does nothing.
// - category and name strings must have application lifetime (statics or
//   literals). They may not include " chars.
#define TRACE_EVENT0 TRACE_EVENT
#define TRACE_EVENT1 TRACE_EVENT
#define TRACE_EVENT2 TRACE_EVENT
#define TRACE_EVENT(category, name, ...)                                       \
  static ::tracing::platform::AtomicWord TRACE_UNIQUE_NAME(cached_ptr) = 0;    \
  const unsigned char* TRACE_UNIQUE_NAME(category_group_enabled) =             \
      ::tracing::internal::GetCategoryGroupEnabledCached(category, &cached_ptr);     \
  ::tracing::internal::ScopedTracer TRACE_UNIQUE_NAME(tracer);                 \
  if (TRACE_INTERNAL_SHOULD_ADD_EVENT(                                         \
          TRACE_UNIQUE_NAME(category_group_enabled))) {                        \
    ::tracing::TraceEventHandle h = ::tracing::internal::AddTraceEvent(        \
        TRACE_EVENT_PHASE_COMPLETE, TRACE_UNIQUE_NAME(category_group_enabled), \
        name, trace_event_internal::kGlobalScope, trace_event_internal::kNoId, \
        TRACE_EVENT_FLAG_NONE, trace_event_internal::kNoId, ##__VA_ARGS__);    \
    TRACE_UNIQUE_NAME(tracer).Initialize(                                      \
        TRACE_UNIQUE_NAME(category_group_enabled), name, h);                   \
  }

#define TRACE_EVENT_BEGIN0 TRACE_EVENT_BEGIN
#define TRACE_EVENT_BEGIN1 TRACE_EVENT_BEGIN
#define TRACE_EVENT_BEGIN2 TRACE_EVENT_BEGIN
#define TRACE_EVENT_BEGIN(category, name, ...)                               \
  do {                                                                       \
    static ::tracing::platform::AtomicWord cached_ptr = 0;                   \
    const unsigned char* TRACE_UNIQUE_NAME(category_group_enabled) =         \
        ::tracing::internal::GetCategoryGroupEnabledCached(category, &cached_ptr); \
    if (TRACE_INTERNAL_SHOULD_ADD_EVENT(                                     \
            TRACE_UNIQUE_NAME(category_group_enabled))) {                    \
      ::tracing::internal::AddTraceEvent(                                    \
          phase, TRACE_UNIQUE_NAME(category_group_enabled), name,            \
          trace_event_internal::kGlobalScope, trace_event_internal::kNoId,   \
          flags, trace_event_internal::kNoId, ##__VA_ARGS__);                \
    }                                                                        \
  } while (0)

// TODO repeat the same pattern for the other TRACE_ macros.

// Notes regarding the following definitions:
// New values can be added and propagated to third party libraries, but existing
// definitions must never be changed, because third party libraries may use old
// definitions.

// Phase indicates the nature of an event entry. E.g. part of a begin/end pair.
#define TRACE_EVENT_PHASE_BEGIN ('B')
#define TRACE_EVENT_PHASE_END ('E')
#define TRACE_EVENT_PHASE_COMPLETE ('X')
#define TRACE_EVENT_PHASE_INSTANT ('I')
#define TRACE_EVENT_PHASE_ASYNC_BEGIN ('S')
#define TRACE_EVENT_PHASE_ASYNC_STEP_INTO ('T')
#define TRACE_EVENT_PHASE_ASYNC_STEP_PAST ('p')
#define TRACE_EVENT_PHASE_ASYNC_END ('F')
#define TRACE_EVENT_PHASE_NESTABLE_ASYNC_BEGIN ('b')
#define TRACE_EVENT_PHASE_NESTABLE_ASYNC_END ('e')
#define TRACE_EVENT_PHASE_NESTABLE_ASYNC_INSTANT ('n')
#define TRACE_EVENT_PHASE_FLOW_BEGIN ('s')
#define TRACE_EVENT_PHASE_FLOW_STEP ('t')
#define TRACE_EVENT_PHASE_FLOW_END ('f')
#define TRACE_EVENT_PHASE_METADATA ('M')
#define TRACE_EVENT_PHASE_COUNTER ('C')
#define TRACE_EVENT_PHASE_SAMPLE ('P')
#define TRACE_EVENT_PHASE_CREATE_OBJECT ('N')
#define TRACE_EVENT_PHASE_SNAPSHOT_OBJECT ('O')
#define TRACE_EVENT_PHASE_DELETE_OBJECT ('D')
#define TRACE_EVENT_PHASE_MEMORY_DUMP ('v')
#define TRACE_EVENT_PHASE_MARK ('R')
#define TRACE_EVENT_PHASE_CLOCK_SYNC ('c')
#define TRACE_EVENT_PHASE_ENTER_CONTEXT ('(')
#define TRACE_EVENT_PHASE_LEAVE_CONTEXT (')')
#define TRACE_EVENT_PHASE_BIND_IDS ('=')

// Flags for changing the behavior of TRACE_EVENT_API_ADD_TRACE_EVENT.
#define TRACE_EVENT_FLAG_NONE (static_cast<unsigned int>(0))
#define TRACE_EVENT_FLAG_COPY (static_cast<unsigned int>(1 << 0))
#define TRACE_EVENT_FLAG_HAS_ID (static_cast<unsigned int>(1 << 1))
// TODO(crbug.com/639003): Free this bit after ID mangling is deprecated.
#define TRACE_EVENT_FLAG_MANGLE_ID (static_cast<unsigned int>(1 << 2))
#define TRACE_EVENT_FLAG_SCOPE_OFFSET (static_cast<unsigned int>(1 << 3))
#define TRACE_EVENT_FLAG_SCOPE_EXTRA (static_cast<unsigned int>(1 << 4))
#define TRACE_EVENT_FLAG_EXPLICIT_TIMESTAMP (static_cast<unsigned int>(1 << 5))
#define TRACE_EVENT_FLAG_ASYNC_TTS (static_cast<unsigned int>(1 << 6))
#define TRACE_EVENT_FLAG_BIND_TO_ENCLOSING (static_cast<unsigned int>(1 << 7))
#define TRACE_EVENT_FLAG_FLOW_IN (static_cast<unsigned int>(1 << 8))
#define TRACE_EVENT_FLAG_FLOW_OUT (static_cast<unsigned int>(1 << 9))
#define TRACE_EVENT_FLAG_HAS_CONTEXT_ID (static_cast<unsigned int>(1 << 10))
#define TRACE_EVENT_FLAG_HAS_PROCESS_ID (static_cast<unsigned int>(1 << 11))
#define TRACE_EVENT_FLAG_HAS_LOCAL_ID (static_cast<unsigned int>(1 << 12))
#define TRACE_EVENT_FLAG_HAS_GLOBAL_ID (static_cast<unsigned int>(1 << 13))

#define TRACE_EVENT_FLAG_SCOPE_MASK                          \
  (static_cast<unsigned int>(TRACE_EVENT_FLAG_SCOPE_OFFSET | \
                             TRACE_EVENT_FLAG_SCOPE_EXTRA))

// Type values for identifying types in the TraceValue union.
#define TRACE_VALUE_TYPE_BOOL (static_cast<unsigned char>(1))
#define TRACE_VALUE_TYPE_UINT (static_cast<unsigned char>(2))
#define TRACE_VALUE_TYPE_INT (static_cast<unsigned char>(3))
#define TRACE_VALUE_TYPE_DOUBLE (static_cast<unsigned char>(4))
#define TRACE_VALUE_TYPE_POINTER (static_cast<unsigned char>(5))
#define TRACE_VALUE_TYPE_STRING (static_cast<unsigned char>(6))
#define TRACE_VALUE_TYPE_COPY_STRING (static_cast<unsigned char>(7))
#define TRACE_VALUE_TYPE_CONVERTABLE (static_cast<unsigned char>(8))

// Enum reflecting the scope of an INSTANT event. Must fit within
// TRACE_EVENT_FLAG_SCOPE_MASK.
#define TRACE_EVENT_SCOPE_GLOBAL (static_cast<unsigned char>(0 << 3))
#define TRACE_EVENT_SCOPE_PROCESS (static_cast<unsigned char>(1 << 3))
#define TRACE_EVENT_SCOPE_THREAD (static_cast<unsigned char>(2 << 3))

#define TRACE_EVENT_SCOPE_NAME_GLOBAL ('g')
#define TRACE_EVENT_SCOPE_NAME_PROCESS ('p')
#define TRACE_EVENT_SCOPE_NAME_THREAD ('t')


// Internal helper functions used by the macros.

namespace tracing {

class ConvertableToTraceFormat;

struct TraceEventHandle {
  uint32_t chunk_seq;

  // These numbers of bits must be kept consistent with
  // TraceBufferChunk::kMaxTrunkIndex and
  // TraceBufferChunk::kTraceBufferChunkSize (in trace_buffer.h).
  unsigned chunk_index : 26;
  unsigned event_index : 6;
};

namespace internal {

inline const unsigned char* GetCategoryGroupEnabledCached(
    const char* category_group,
    volatile const ::tracing::platform::AtomicWord* cached_ptr) {
  const unsigned char* category_group_enabled =
      reinterpret_cast<const unsigned char*>(
          ::tracing::platform::NoBarrier_Load(cached_ptr));
  if (TRACE_UNLIKELY(!category_group_enabled)) {
    category_group_enabled = ::tracing::platform::GetCategoryGroupEnabled(category_group);
    ::tracing::platform::NoBarrier_Store(
        category_group_enabled,
        reinterpret_cast<::tracing::platform::AtomicWord>(
            category_group_enabled));
  }
  return category_group_enabled;
}

// Provide an overload for all the possible combinations of vararg macros.
// Ultimately all these AddTraceEvent* call into tracing::platform::AddEvent().

inline TraceEventHandle AddTraceEvent(
    char phase,
    const unsigned char* category_group_enabled,
    const char* name,
    const char* scope,
    uint64_t id,
    uint32_t flags,
    uint64_t bind_id) {
  ::tracing::platform::AddEvent(
      phase, category_group_enabled, name, scope, id, bind_id,
      ::tracing::platform::GetCurrentThreadId(), ::tracing::platform::Now(),
      0 /* num_args */, nullptr /* arg_names */, nullptr /* arg_types */,
      nullptr /* arg_values */, nullptr /* convertable_values */, flags);
}

template <class ARG1_CONVERTABLE_TYPE>
inline TraceEventHandle AddTraceEventWithThreadIdAndTimestamp(
    char phase,
    const unsigned char* category_group_enabled,
    const char* name,
    const char* scope,
    uint64_t id,
    int thread_id,
    uint64_t timestamp,
    uint32_t flags,
    uint64_t bind_id,
    const char* arg1_name,
    std::unique_ptr<ARG1_CONVERTABLE_TYPE> arg1_val) {
  const char* arg_names[1] = {arg1_name};
  unsigned char arg_types[1] = {TRACE_VALUE_TYPE_CONVERTABLE};
  std::unique_ptr<ConvertableToTraceFormat> convertable_values[1] = {
      std::move(arg_value)};
  ::tracing::platform::AddTraceEventWithThreadIdAndTimestamp(
      phase, category_group_enabled, name, scope, id, bind_id, thread_id,
      timestamp, 1, arg_names, arg_types, convertable_values, flags);
}

// TODO add all other overloads

}  // namespace internal
}  // namespace tracing

#endif  // PERF_TRACING_CORE_PUBLIC_TRACE_EVENT_H_
