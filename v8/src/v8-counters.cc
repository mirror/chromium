// Copyright 2007-2008 Google Inc. All Rights Reserved.
// <<license>>

#include "v8.h"

#include "v8-counters.h"

namespace v8 { namespace internal {

#define SR(name, caption) StatsRate Counters::name(L###caption, k_##name);
  STATS_RATE_LIST(SR)
#undef SR

#define SC(name, caption) StatsCounter Counters::name(L###caption, k_##name);
  STATS_COUNTER_LIST_1(SC)
  STATS_COUNTER_LIST_2(SC)
#undef SC

StatsCounter Counters::state_counters[state_tag_count] = {
#define COUNTER_NAME(name) StatsCounter(L"V8.State" L###name, k_##name),
  STATE_TAG_LIST(COUNTER_NAME)
#undef COUNTER_NAME
};

} }  // namespace v8::internal
