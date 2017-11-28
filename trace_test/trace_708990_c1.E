struct Category {
  const char *name;
  uint8_t is_enabled;
};

constexpr uint32_t N = 100;
constexpr Category kKnownCategories[N] = {
   {"foo", false},
   {"bar", false},
   {"toplevel", false},
   {"cc", false},

};

Category g_categories[N] = {



    kKnownCategories[0],
    kKnownCategories[1],
    kKnownCategories[2],
    kKnownCategories[3],
};

constexpr bool ConstexprStrcmp(const char* a, const char* b, int n = 0) {
  return (!a[n] && !b[n]) ? true : ((a[n] == b[n]) ? ConstexprStrcmp(a, b, n + 1) : false);
}

constexpr uint8_t *GetEnabledPtrAtCompileTime(const char *cat, uint32_t i = 0) {


  return (kKnownCategories[i].name == nullptr)
             ? nullptr
             : (ConstexprStrcmp(cat, kKnownCategories[i].name)
                    ? &g_categories[i].is_enabled
                    : (GetEnabledPtrAtCompileTime(cat, i + 1)));
}
# 99 "../../trace_test/trace_708990_c1.cc"
extern "C" void trace_708990_c1() {
  static base::subtle::AtomicWord trace_event_unique_atomic100 = 0;
  constexpr uint8_t* trace_event_unique_category_group_enabled100 = GetEnabledPtrAtCompileTime("cc");
  if (trace_event_unique_category_group_enabled100) {
    base::subtle::NoBarrier_Store(&(trace_event_unique_atomic100),
      (reinterpret_cast<base::subtle::AtomicWord>(
        trace_event_unique_category_group_enabled100)));
  } else if (__builtin_expect(!!(!reinterpret_cast<const unsigned char*>(
    base::subtle::NoBarrier_Load(&(trace_event_unique_atomic100)))), 0)) {
    base::subtle::NoBarrier_Store(&(trace_event_unique_atomic100),
      (reinterpret_cast<base::subtle::AtomicWord>(
        base::trace_event::TraceLog::GetCategoryGroupEnabled("cc"))));
  };;
  trace_event_internal::ScopedTracer trace_event_unique_tracer100;
  if (__builtin_expect(!!(*reinterpret_cast<const unsigned char*>(
    base::subtle::NoBarrier_Load(&(trace_event_unique_atomic100)))
    & (base::trace_event::TraceCategory::ENABLED_FOR_RECORDING
      | base::trace_event::TraceCategory::ENABLED_FOR_ETW_EXPORT
      | base::trace_event::TraceCategory::ENABLED_FOR_FILTERING)), 0)) {
    base::trace_event::TraceEventHandle h = trace_event_internal::AddTraceEvent(
          ('X'),
          trace_event_unique_category_group_enabled100,
          "layers",
          trace_event_internal::kGlobalScope,
          trace_event_internal::kNoId,
          (static_cast<unsigned int>(0)),
          trace_event_internal::kNoId);
          trace_event_unique_tracer100.Initialize(
              trace_event_unique_category_group_enabled100, "layers", h);
  };
}

extern "C" void trace_708990_c1_unknown() {
  static base::subtle::AtomicWord trace_event_unique_atomic104 = 0; constexpr uint8_t* trace_event_unique_category_group_enabled104 = GetEnabledPtrAtCompileTime("unknown"); if (trace_event_unique_category_group_enabled104) { base::subtle::NoBarrier_Store(&(trace_event_unique_atomic104), (reinterpret_cast<base::subtle::AtomicWord>( trace_event_unique_category_group_enabled104))); } else if (__builtin_expect(!!(!reinterpret_cast<const unsigned char*>( base::subtle::NoBarrier_Load(&(trace_event_unique_atomic104)))), 0)) { base::subtle::NoBarrier_Store(&(trace_event_unique_atomic104), (reinterpret_cast<base::subtle::AtomicWord>( base::trace_event::TraceLog::GetCategoryGroupEnabled("unknown")))); };; trace_event_internal::ScopedTracer trace_event_unique_tracer104; if (__builtin_expect(!!(*reinterpret_cast<const unsigned char*>( base::subtle::NoBarrier_Load(&(trace_event_unique_atomic104))) & (base::trace_event::TraceCategory::ENABLED_FOR_RECORDING | base::trace_event::TraceCategory::ENABLED_FOR_ETW_EXPORT | base::trace_event::TraceCategory::ENABLED_FOR_FILTERING)), 0)) { base::trace_event::TraceEventHandle h = trace_event_internal::AddTraceEvent( ('X'), trace_event_unique_category_group_enabled104, "unknown", trace_event_internal::kGlobalScope, trace_event_internal::kNoId, (static_cast<unsigned int>(0)), trace_event_internal::kNoId); trace_event_unique_tracer104.Initialize( trace_event_unique_category_group_enabled104, "unknown", h); };
}
