// gn args must include "is_official_build = true"
// you can compile this by running
//  $ ninja -C out/Official trace_test -v

#include "base/trace_event/trace_event.h"

extern "C" void trace_initial() {
  TRACE_EVENT0("cc", "layers");
}
