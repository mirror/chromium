// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_ARC_TEST_CONNECTION_HOLDER_UTIL_H_
#define COMPONENTS_ARC_TEST_CONNECTION_HOLDER_UTIL_H_

#include "base/callback_helpers.h"
#include "base/run_loop.h"
#include "components/arc/connection_holder.h"

namespace arc {

// Waits for the instance to be ready.
template <typename InstanceType, typename HostType>
void WaitForInstanceReady(ConnectionHolder<InstanceType, HostType>* holder) {
  if (holder->IsConnected())
    return;

  class ReadinessObserver
      : public ConnectionHolder<InstanceType, HostType>::Observer {
   public:
    explicit ReadinessObserver(base::OnceClosure closure)
        : closure_(std::move(closure)) {}
    ~ReadinessObserver() override = default;

   private:
    void OnConnectionReady() override {
      if (!closure_)
        return;
      base::ResetAndReturn(&closure_).Run();
    }

    base::OnceClosure closure_;
  };

  base::RunLoop run_loop;
  ReadinessObserver readiness_observer(run_loop.QuitClosure());
  holder->AddObserver(&readiness_observer);
  run_loop.Run();
  holder->RemoveObserver(&readiness_observer);
}

}  // namespace arc

#endif  // COMPONENTS_ARC_TEST_CONNECTION_HOLDER_UTIL_H_
