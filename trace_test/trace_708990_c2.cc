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
  X(foo, false)                  \
  X(bar, false)                  \
  X(zoo, false)                  \
  X(cc, false)

// Declare the list of known categories as an enum class.
// Note that Category::Unknown is -1 on purpose.
enum class Category {
  Unknown = -1,
#define DECLARE_CATEGORY(name, enabled) name,
  LIST_KNOWN_CATEGORIES(DECLARE_CATEGORY)
#undef DECLARE_CATEGORY
      TotalCountMustBeLast  // Don't remove.
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
static uint8_t kCategoryEnabled[kCategoryCount] = {
#define CATEGORY_ENABLED(name, enabled) enabled,
    LIST_KNOWN_CATEGORIES(CATEGORY_ENABLED)
#undef CATEGORY_ENABLED
};

static constexpr uint8_t* FindCategoryByIndex(const char* name, int index) {
  return (index < kCategoryCount) ? (str_eq(kCategoryNames[index], name)
                                         ? &kCategoryEnabled[index]
                                         : FindCategoryByIndex(name, index + 1))
                                  : nullptr;
}

static constexpr uint8_t* FindCategoryByName(const char* name) {
  return FindCategoryByIndex(name, 0);
}

static_assert(FindCategoryByName("foo"), "foo not found");
static_assert(FindCategoryByName("bar"), "bar not found");
static_assert(FindCategoryByName("zoo"), "zoo not found");
static_assert(!FindCategoryByName("ah ah"), "ah ah found");

// Returns true at compile time if |category| is part of kCategoryNames.
constexpr bool IsKnownCategory(const char* category) {
  return FindCategoryByName(category) != nullptr;
}

// A special internal template. Used internally by CategoryEnabledPtr below.
// Specializations should provide a static value(const char*) method that
// returns the address of the "enabled bit set" for a given |category|
// parameter.
template <bool IS_KNOWN_CATEGORY>
struct CategoryEnabledPtrInternal;

// The template specialization for known categories. This directly returns
// the address of the corresponding |enabled| entry in the matching
// |g_categories| array.
template <>
struct CategoryEnabledPtrInternal<true> {
  constexpr static uint8_t* value(const char* category) {
    return FindCategoryByName(category);
  }
};

// The template specialization for unknown categories. This uses an atomic
// variable to store a pointer to the |enabled| entry, lazily initialized by
// calling the TRACE_EVENT_API_GET_CATEGORY_GROUP() function at runtime.
template <>
struct CategoryEnabledPtrInternal<false> {
  TRACE_EVENT_API_ATOMIC_WORD sAtomic = 0;
  const uint8_t* value(const char* category) {
    auto* enabled_ptr = reinterpret_cast<const unsigned char*>(
        TRACE_EVENT_API_ATOMIC_LOAD(sAtomic));
    if (UNLIKELY(enabled_ptr == nullptr)) {
      enabled_ptr = TRACE_EVENT_API_GET_CATEGORY_GROUP_ENABLED(category);
      TRACE_EVENT_API_ATOMIC_STORE(
          sAtomic, reinterpret_cast<TRACE_EVENT_API_ATOMIC_WORD>(enabled_ptr));
    }
    return enabled_ptr;
  }
};

// Defines an |enabled_ptr| local variable that can access the enabled bit set
// for |category_group| by calling |enabled_ptr.value(category_group)|.
#define TEST_INTERNAL_TRACE_EVENT_GET_CATEGORY_INFO(category_group) \
  CategoryEnabledPtrInternal<IsKnownCategory(category_group)>       \
      INTERNAL_TRACE_EVENT_UID(enabled_ptr);

#define TEST_INTERNAL_TRACE_EVENT_CATEGORY_GROUP_ENABLED(category_group)  \
  UNLIKELY(*INTERNAL_TRACE_EVENT_UID(enabled_ptr).value(category_group) & \
           (base::trace_event::TraceCategory::ENABLED_FOR_RECORDING |     \
            base::trace_event::TraceCategory::ENABLED_FOR_ETW_EXPORT |    \
            base::trace_event::TraceCategory::ENABLED_FOR_FILTERING))

// from: INTERNAL_TRACE_EVENT_ADD_SCOPED
#define TEST_INTERNAL_TRACE_EVENT_ADD_SCOPED(category_group, name, ...)        \
  TEST_INTERNAL_TRACE_EVENT_GET_CATEGORY_INFO(category_group);                 \
  trace_event_internal::ScopedTracer INTERNAL_TRACE_EVENT_UID(tracer);         \
  if (TEST_INTERNAL_TRACE_EVENT_CATEGORY_GROUP_ENABLED(category_group)) {      \
    base::trace_event::TraceEventHandle h =                                    \
        trace_event_internal::AddTraceEvent(                                   \
            TRACE_EVENT_PHASE_COMPLETE,                                        \
            INTERNAL_TRACE_EVENT_UID(enabled_ptr).value(category_group), name, \
            trace_event_internal::kGlobalScope, trace_event_internal::kNoId,   \
            TRACE_EVENT_FLAG_NONE, trace_event_internal::kNoId,                \
            ##__VA_ARGS__);                                                    \
    INTERNAL_TRACE_EVENT_UID(tracer).Initialize(                               \
        INTERNAL_TRACE_EVENT_UID(enabled_ptr).value(category_group), name, h); \
  }

// from: TRACE_EVENT0
#define TEST_TRACE_EVENT0(category_group, name) \
  TEST_INTERNAL_TRACE_EVENT_ADD_SCOPED(category_group, name)

extern "C" void trace_708990_c2() {
  TEST_TRACE_EVENT0("cc", "layers");
}

extern "C" void trace_708990_c2_unknown() {
  TEST_TRACE_EVENT0("unknown", "unknown");
}
