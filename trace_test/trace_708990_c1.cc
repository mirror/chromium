// gn args must include "is_official_build = true"
// you can compile this by running
//  $ ninja -C out/Official trace_test -v

#include "base/trace_event/trace_event.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

struct Category {
  const char* name;
  uint8_t is_enabled;
};

constexpr uint32_t N = 100;
constexpr Category kKnownCategories[N] = {
    {"foo", false},
    {"bar", false},
    {"toplevel", false},
    {"cc", false},
    // The remaining slots are zero-initialized and available for the dynamic
    // categories.
};

Category g_categories[N] = {
    // Having to do these assignments is really awkward. But I don't know
    // enough constexpr magic to be able to just assign the array to the
    // constexpr one.
    kKnownCategories[0], kKnownCategories[1], kKnownCategories[2],
    kKnownCategories[3],
};

constexpr bool ConstexprStrEq(const char* a, const char* b, int n = 0) {
  return (!a[n] && !b[n])
             ? true
             : ((a[n] == b[n]) ? ConstexprStrEq(a, b, n + 1) : false);
}

constexpr uint8_t* GetEnabledPtrAtCompileTime(const char* cat, uint32_t i = 0) {
  // Apologies for the nested ternary operators, but any other more readable
  // option (if statements, variables) was a C++14 extension.
  return (kKnownCategories[i].name == nullptr)
             ? nullptr
             : (ConstexprStrEq(cat, kKnownCategories[i].name)
                    ? &g_categories[i].is_enabled
                    : (GetEnabledPtrAtCompileTime(cat, i + 1)));
}

// from: INTERNAL_TRACE_EVENT_GET_CATEGORY_INFO_CUSTOM_VARIABLES
#define TEST_INTERNAL_TRACE_EVENT_GET_CATEGORY_INFO_CUSTOM_VARIABLES(     \
    category_group, atomic, category_group_enabled)                       \
  if (category_group_enabled) {                                           \
    TRACE_EVENT_API_ATOMIC_STORE(                                         \
        atomic, reinterpret_cast<TRACE_EVENT_API_ATOMIC_WORD>(            \
                    category_group_enabled));                             \
  } else if (UNLIKELY(!reinterpret_cast<const unsigned char*>(            \
                 TRACE_EVENT_API_ATOMIC_LOAD(atomic)))) {                 \
    TRACE_EVENT_API_ATOMIC_STORE(                                         \
        atomic,                                                           \
        reinterpret_cast<TRACE_EVENT_API_ATOMIC_WORD>(                    \
            TRACE_EVENT_API_GET_CATEGORY_GROUP_ENABLED(category_group))); \
  }

// from: INTERNAL_TRACE_EVENT_GET_CATEGORY_INFO
#define TEST_INTERNAL_TRACE_EVENT_GET_CATEGORY_INFO(category_group)        \
  static TRACE_EVENT_API_ATOMIC_WORD INTERNAL_TRACE_EVENT_UID(atomic) = 0; \
  constexpr uint8_t* INTERNAL_TRACE_EVENT_UID(category_group_enabled) =    \
      GetEnabledPtrAtCompileTime(category_group);                          \
  TEST_INTERNAL_TRACE_EVENT_GET_CATEGORY_INFO_CUSTOM_VARIABLES(            \
      category_group, INTERNAL_TRACE_EVENT_UID(atomic),                    \
      INTERNAL_TRACE_EVENT_UID(category_group_enabled));

#define TEST_INTERNAL_TRACE_EVENT_CATEGORY_GROUP_ENABLED()                 \
  UNLIKELY(                                                                \
      *reinterpret_cast<const unsigned char*>(                             \
          TRACE_EVENT_API_ATOMIC_LOAD(INTERNAL_TRACE_EVENT_UID(atomic))) & \
      (base::trace_event::TraceCategory::ENABLED_FOR_RECORDING |           \
       base::trace_event::TraceCategory::ENABLED_FOR_ETW_EXPORT |          \
       base::trace_event::TraceCategory::ENABLED_FOR_FILTERING))

// from: INTERNAL_TRACE_EVENT_ADD_SCOPED
#define TEST_INTERNAL_TRACE_EVENT_ADD_SCOPED(category_group, name, ...)      \
  TEST_INTERNAL_TRACE_EVENT_GET_CATEGORY_INFO(category_group);               \
  trace_event_internal::ScopedTracer INTERNAL_TRACE_EVENT_UID(tracer);       \
  if (TEST_INTERNAL_TRACE_EVENT_CATEGORY_GROUP_ENABLED()) {                  \
    base::trace_event::TraceEventHandle h =                                  \
        trace_event_internal::AddTraceEvent(                                 \
            TRACE_EVENT_PHASE_COMPLETE,                                      \
            INTERNAL_TRACE_EVENT_UID(category_group_enabled), name,          \
            trace_event_internal::kGlobalScope, trace_event_internal::kNoId, \
            TRACE_EVENT_FLAG_NONE, trace_event_internal::kNoId,              \
            ##__VA_ARGS__);                                                  \
    INTERNAL_TRACE_EVENT_UID(tracer).Initialize(                             \
        INTERNAL_TRACE_EVENT_UID(category_group_enabled), name, h);          \
  }

// from: TRACE_EVENT0
#define TEST_TRACE_EVENT0(category_group, name) \
  TEST_INTERNAL_TRACE_EVENT_ADD_SCOPED(category_group, name)

extern "C" void trace_708990_c1() {
  TEST_TRACE_EVENT0("cc", "layers");
}

extern "C" void trace_708990_c1_unknown() {
  TEST_TRACE_EVENT0("unknown", "unknown");
}
