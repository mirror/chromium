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
#include "base/optional.h"
#include "base/run_loop.h"
#include "components/arc/common/timer.mojom.h"

namespace arc {

class FakeTimerInstance : public mojom::TimerInstance {
 public:
  FakeTimerInstance();
  ~FakeTimerInstance() override;

  // mojom::TimerInstance overrides:
  void Init(mojom::TimerHostPtr host_ptr, InitCallback callback) override;

  // Calls mojom::TimerHost::CreateTimers and waits until timer creation
  // completes.
  void CallCreateTimers(const std::vector<clockid_t>& clocks);

  // Calls mojom::TimerHost::StartTimer. Returns true iff the timer was started
  // successfully.
  bool CallStartTimer(clockid_t clock_id,
                      base::TimeTicks absolute_expiration_time);

  // Returns true if the timer for |clock_id| expired. Returns false if the
  // timer isn't present or the data read on a wake up is inconsistent. Blocks
  // if expiration is not indicated by the host.
  bool WaitForExpiration(clockid_t clock_id);

  // Returns true if a |mojom::TimerPtr| corresponding to |clock| is stored.
  bool IsTimerPresent(clockid_t clock_id) const;

 private:
  struct ArcTimerInfo;

  // Adds timers stored in |result| to |arc_timers_|.
  void CreateTimersCallback(
      base::Optional<std::vector<mojom::CreateTimerResponsePtr>> result);

  std::map<clockid_t, ArcTimerInfo> arc_timers_;

  mojom::TimerHostPtr host_ptr_;

  DISALLOW_COPY_AND_ASSIGN(FakeTimerInstance);
};

}  // namespace arc

#endif  // COMPONENTS_ARC_TEST_FAKE_TIMER_INSTANCE_H_
