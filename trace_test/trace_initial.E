# 6 "../../trace_test/trace_initial.cc" 2

extern "C" void trace_initial() {
  static base::subtle::AtomicWord trace_event_unique_atomic8 = 0;
  const unsigned char* trace_event_unique_category_group_enabled8;
  trace_event_unique_category_group_enabled8
    = reinterpret_cast<const unsigned char*>(
        base::subtle::NoBarrier_Load(&(trace_event_unique_atomic8)));

  if (__builtin_expect(!!(!trace_event_unique_category_group_enabled8), 0)) {
    trace_event_unique_category_group_enabled8 =
      base::trace_event::TraceLog::GetCategoryGroupEnabled("cc");
    base::subtle::NoBarrier_Store(&(trace_event_unique_atomic8),
      (reinterpret_cast<base::subtle::AtomicWord>(
          trace_event_unique_category_group_enabled8)));
  };;

  trace_event_internal::ScopedTracer trace_event_unique_tracer8;
  if (__builtin_expect(!!(*trace_event_unique_category_group_enabled8
      & (base::trace_event::TraceCategory::ENABLED_FOR_RECORDING
          | base::trace_event::TraceCategory::ENABLED_FOR_ETW_EXPORT
          | base::trace_event::TraceCategory::ENABLED_FOR_FILTERING)), 0)) {

    base::trace_event::TraceEventHandle h
      = trace_event_internal::AddTraceEvent(
                                ('X'),
                                trace_event_unique_category_group_enabled8,
                                "layers",
                                trace_event_internal::kGlobalScope,
                                trace_event_internal::kNoId,
                                (static_cast<unsigned int>(0)),
                                trace_event_internal::kNoId);

    trace_event_unique_tracer8.Initialize(
      trace_event_unique_category_group_enabled8, "layers", h);
  };
}
