// Special trace event macro to trace task execution with the location where it
// was posted from.
#define TRACE_TASK_EXECUTION(run_function, task) \
  INTERNAL_TRACE_TASK_EXECUTION(run_function, task)
