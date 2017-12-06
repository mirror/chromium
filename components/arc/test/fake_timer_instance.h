// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_ARC_TEST_FAKE_TIMER_INSTANCE_H_
#define COMPONENTS_ARC_TEST_FAKE_TIMER_INSTANCE_H_

#include <map>
#include <vector>

#include "base/files/scoped_file.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/run_loop.h"
#include "components/arc/common/timer.mojom.h"

namespace arc {

class FakeTimer : public mojom::Timer {
  // mojom::Timer overrides.
  void Start(int64_t nanoseconds_from_now, StartCallback callback) override;

  DISALLOW_COPY_AND_ASSIGN(FakeTimer);
};
class FakeTimerInstance : public mojom::TimerInstance {
 public:
  FakeTimerInstance();
  ~FakeTimerInstance() override;

  // mojom::PowerInstance overrides:
  void Init(mojom::TimerHostPtr host_ptr) override;

  // Calls mojom::PowerHost::CreateTimers.
  void CallCreateTimers(const std::vector<clockid_t>& clocks);

  // Calls mojom::PowerHost::StartTimer. Returns true if the timer expired
  // at |nanoseconds_from_now| within some admissible error. Returns false
  // otherwise.
  bool CallStartTimerAndWaitForExpiration(clockid_t clock_id,
                                          int64_t nanoseconds_from_now);

  // Returns true if a |mojom::TimerPtr| corresponding to |clock| is stored.
  bool IsTimerPresent(const clockid_t clock_id);

 private:
  struct ArcTimerInfo {
    ArcTimerInfo();
    ArcTimerInfo(ArcTimerInfo&&);
    ~ArcTimerInfo();
    mojom::TimerPtr timer;
    base::ScopedFD read_fd;
  };

  // Adds timers stored in |result| to |arc_timers_|.
  void CreateTimersCallback(
      const std::vector<clockid_t>& arc_clocks,
      base::RunLoop* loop,
      base::Optional<std::vector<mojom::TimerPtr>>* result);

  std::map<clockid_t, ArcTimerInfo> arc_timers_;

  mojom::TimerHostPtr host_ptr_;
  base::WeakPtrFactory<FakeTimerInstance> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(FakeTimerInstance);
};

}  // namespace arc

#endif  // COMPONENTS_ARC_TEST_FAKE_TIMER_INSTANCE_H_
