#include "base/trace_event/trace_event.h"

#include <stdint.h>

static constexpr bool str_eq(const char* a, const char* b) {
    return (*a == *b) && (!*a || str_eq(a + 1, b + 1));
}

static_assert(str_eq("foo", "foo"), "strings should be equal");
static_assert(!str_eq("foo", "Foo"), "strings should not be equal");
static_assert(!str_eq("foo", "foo1"), "strings should not be equal");
static_assert(!str_eq("foo2", "foo"), "strings should not be equal");

#define LIST_KNOWN_CATEGORIES(X) \
    X(foo, false) \
    X(bar, false) \
    X(zoo, false) \
    X(cc, false) \

// Declare the list of known categories as an enum class.
// Note that Category::Unknown is -1 on purpose.
enum class Category {
    Unknown = -1,
    #define DECLARE_CATEGORY(name, enabled) name,
    LIST_KNOWN_CATEGORIES(DECLARE_CATEGORY)
    #undef DECLARE_CATEGORY
    TotalCountMustBeLast   // Don't remove.
};

static constexpr int kCategoryCount =
        static_cast<int>(Category::TotalCountMustBeLast);

// An array of category names.
static constexpr const char* const kCategoryNames[kCategoryCount] = {
#define STRINGIFY_(x) #x
#define STRINGIFY(x, y) STRINGIFY_(x),
    LIST_KNOWN_CATEGORIES(STRINGIFY)
#undef STRINGIFY_
#undef STRINGIFY
};

// An array to check category enabled.
uint8_t kCategoryEnabled[kCategoryCount] = {
#define CATEGORY_ENABLED(name, enabled) enabled,
    LIST_KNOWN_CATEGORIES(CATEGORY_ENABLED)
#undef CATEGORY_ENABLED
};

static constexpr uint8_t* FindCategoryByIndex(const char* name, int index) {
    return (index < kCategoryCount)
            ? (str_eq(kCategoryNames[index], name)
                    ? &kCategoryEnabled[index]
                    : FindCategoryByIndex(name, index + 1))
            : NULL;
}

static constexpr uint8_t* FindCategoryByName(const char* name) {
    return FindCategoryByIndex(name, 0);
}

static_assert(FindCategoryByName("foo"), "foo not found");
static_assert(FindCategoryByName("bar"), "bar not found");
static_assert(FindCategoryByName("zoo"), "zoo not found");
static_assert(!FindCategoryByName("ah ah"), "ah ah found");

// from: INTERNAL_TRACE_EVENT_GET_CATEGORY_INFO_CUSTOM_VARIABLES
#define TEST_INTERNAL_TRACE_EVENT_GET_CATEGORY_INFO_CUSTOM_VARIABLES( \
    category_group, atomic, category_group_enabled) \
    if (category_group_enabled) { \
        TRACE_EVENT_API_ATOMIC_STORE(atomic, \
                reinterpret_cast<TRACE_EVENT_API_ATOMIC_WORD>( \
                    category_group_enabled)); \
    } else if (UNLIKELY(!reinterpret_cast<const unsigned char*>( \
                    TRACE_EVENT_API_ATOMIC_LOAD(atomic)))) { \
        TRACE_EVENT_API_ATOMIC_STORE(atomic, \
                reinterpret_cast<TRACE_EVENT_API_ATOMIC_WORD>( \
                    TRACE_EVENT_API_GET_CATEGORY_GROUP_ENABLED(category_group))); \
    }

// from: INTERNAL_TRACE_EVENT_GET_CATEGORY_INFO
#define TEST_INTERNAL_TRACE_EVENT_GET_CATEGORY_INFO(category_group) \
    static TRACE_EVENT_API_ATOMIC_WORD INTERNAL_TRACE_EVENT_UID(atomic) = 0; \
    constexpr uint8_t* INTERNAL_TRACE_EVENT_UID(category_group_enabled) = \
 			FindCategoryByName(category_group); \
    TEST_INTERNAL_TRACE_EVENT_GET_CATEGORY_INFO_CUSTOM_VARIABLES(category_group, \
        INTERNAL_TRACE_EVENT_UID(atomic), \
        INTERNAL_TRACE_EVENT_UID(category_group_enabled));

// from: INTERNAL_TRACE_EVENT_CATEGORY_GROUP_ENABLED
#define TEST_INTERNAL_TRACE_EVENT_CATEGORY_GROUP_ENABLED()                  \
  UNLIKELY(*reinterpret_cast<const unsigned char*>( \
				TRACE_EVENT_API_ATOMIC_LOAD(INTERNAL_TRACE_EVENT_UID(atomic))) &         \
           (base::trace_event::TraceCategory::ENABLED_FOR_RECORDING |  \
            base::trace_event::TraceCategory::ENABLED_FOR_ETW_EXPORT | \
            base::trace_event::TraceCategory::ENABLED_FOR_FILTERING))

// from: INTERNAL_TRACE_EVENT_ADD_SCOPED
#define TEST_INTERNAL_TRACE_EVENT_ADD_SCOPED(category_group, name, ...)           \
  TEST_INTERNAL_TRACE_EVENT_GET_CATEGORY_INFO(category_group);                    \
  trace_event_internal::ScopedTracer INTERNAL_TRACE_EVENT_UID(tracer);       \
  if (TEST_INTERNAL_TRACE_EVENT_CATEGORY_GROUP_ENABLED()) {                       \
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
#define TEST_TRACE_EVENT0(category_group, name)    \
  TEST_INTERNAL_TRACE_EVENT_ADD_SCOPED(category_group, name)

extern "C" void trace_708990_c2() {
  TEST_TRACE_EVENT0("cc", "layers");
}

extern "C" void trace_708990_c2_unknown() {
  TEST_TRACE_EVENT0("unknown", "unknown");
}
