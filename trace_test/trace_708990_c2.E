# 2 "../../trace_test/trace_708990_c2.cc" 2



static constexpr bool str_eq(const char* a, const char* b) {
  return (*a == *b) && (!*a || str_eq(a + 1, b + 1));
}

static_assert(str_eq("foo", "foo"), "strings should be equal");
static_assert(!str_eq("foo", "Foo"), "strings should not be equal");
static_assert(!str_eq("foo", "foo1"), "strings should not be equal");
static_assert(!str_eq("foo2", "foo"), "strings should not be equal");
# 22 "../../trace_test/trace_708990_c2.cc"
enum class Category {
  Unknown = -1,

  foo, bar, zoo, cc,

      TotalCountMustBeLast
};

static constexpr int kCategoryCount =
    static_cast<int>(Category::TotalCountMustBeLast);


static constexpr const char* const kCategoryNames[kCategoryCount] = {


    "foo", "bar", "zoo", "cc",


};


static uint8_t kCategoryEnabled[kCategoryCount] = {

    false, false, false, false,

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


constexpr bool IsKnownCategory(const char* category) {
  return FindCategoryByName(category) != nullptr;
}





template <bool IS_KNOWN_CATEGORY>
struct CategoryEnabledPtrInternal;




template <>
struct CategoryEnabledPtrInternal<true> {
  constexpr static uint8_t* value(const char* category) {
    return FindCategoryByName(category);
  }
};




template <>
struct CategoryEnabledPtrInternal<false> {
  base::subtle::AtomicWord sAtomic = 0;
  const uint8_t* value(const char* category) {
    auto* enabled_ptr = reinterpret_cast<const unsigned char*>(
        base::subtle::NoBarrier_Load(&(sAtomic)));
    if (__builtin_expect(!!(enabled_ptr == nullptr), 0)) {
      enabled_ptr = base::trace_event::TraceLog::GetCategoryGroupEnabled(category);
      base::subtle::NoBarrier_Store(&(sAtomic), (reinterpret_cast<base::subtle::AtomicWord>(enabled_ptr)));

    }
    return enabled_ptr;
  }
};
# 137 "../../trace_test/trace_708990_c2.cc"
extern "C" void trace_708990_c2() {
  CategoryEnabledPtrInternal<IsKnownCategory("cc")> trace_event_unique_enabled_ptr138;;
  trace_event_internal::ScopedTracer trace_event_unique_tracer138;
  if (__builtin_expect(!!(*trace_event_unique_enabled_ptr138.value("cc")
      & (base::trace_event::TraceCategory::ENABLED_FOR_RECORDING
        | base::trace_event::TraceCategory::ENABLED_FOR_ETW_EXPORT
        | base::trace_event::TraceCategory::ENABLED_FOR_FILTERING)), 0)) {
    base::trace_event::TraceEventHandle h = trace_event_internal::AddTraceEvent(
                                      ('X'),
                                      trace_event_unique_enabled_ptr138.value("cc"),
                                      "layers",
                                      trace_event_internal::kGlobalScope,
                                      trace_event_internal::kNoId,
                                      (static_cast<unsigned int>(0)),
                                      trace_event_internal::kNoId);
    trace_event_unique_tracer138.Initialize(
                        trace_event_unique_enabled_ptr138.value("cc"), "layers", h);
  };
}

extern "C" void trace_708990_c2_unknown() {
  CategoryEnabledPtrInternal<IsKnownCategory("unknown")> trace_event_unique_enabled_ptr142;;
  trace_event_internal::ScopedTracer trace_event_unique_tracer142;
  if (__builtin_expect(!!(*trace_event_unique_enabled_ptr142.value("unknown")
      & (base::trace_event::TraceCategory::ENABLED_FOR_RECORDING
        | base::trace_event::TraceCategory::ENABLED_FOR_ETW_EXPORT
        | base::trace_event::TraceCategory::ENABLED_FOR_FILTERING)), 0)) {
    base::trace_event::TraceEventHandle h = trace_event_internal::AddTraceEvent(
                                  ('X'),
                                  trace_event_unique_enabled_ptr142.value("unknown"),
                                  "unknown",
                                  trace_event_internal::kGlobalScope,
                                  trace_event_internal::kNoId,
                                  (static_cast<unsigned int>(0)),
                                  trace_event_internal::kNoId);
    trace_event_unique_tracer142.Initialize(
                    trace_event_unique_enabled_ptr142.value("unknown"), "unknown", h);
  };
}
