#ifndef PRODUCTS_CRONET_THREAD_UTIL_H
#define PRODUCTS_CRONET_THREAD_UTIL_H

#include <functional>

#include "base/callback.h"

namespace base {
class Location;
class SequencedTaskRunner;
}  // namespace base

namespace cronet {
typedef void (*TaskFunc)();
void PostTaskAndWaitForCompletion(base::SequencedTaskRunner* task_runner,
                                  const base::Location& from_here,
                                  std::function<void()> task);
}  // namespace cronet

class cronet_thread_util {};

#endif  // PRODUCTS_CRONET_THREAD_UTIL_H
