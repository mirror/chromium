#include "cronet_thread_util.h"

#include "base/bind.h"
#include "base/location.h"
#include "base/sequenced_task_runner.h"
#include "base/synchronization/waitable_event.h"

namespace {
void TaskWrapperWithWaitableEventSignal(std::function<void()> task,
                                        base::WaitableEvent* lock) {
  task();
  lock->Signal();
}
}  // namespace

void cronet::PostTaskAndWaitForCompletion(
    base::SequencedTaskRunner* task_runner,
    const base::Location& from_here,
    std::function<void()> task) {
  base::WaitableEvent lock(base::WaitableEvent::ResetPolicy::AUTOMATIC,
                           base::WaitableEvent::InitialState::NOT_SIGNALED);
  task_runner->PostTask(
      from_here,
      base::BindOnce(TaskWrapperWithWaitableEventSignal, task, &lock));
  lock.Wait();
}
