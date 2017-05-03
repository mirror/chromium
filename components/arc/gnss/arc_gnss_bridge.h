// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_ARC_GNSS_ARC_GNSS_BRIDGE_H_
#define COMPONENTS_ARC_GNSS_ARC_GNSS_BRIDGE_H_

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/threading/thread_checker.h"
#include "base/timer/timer.h"
#include "components/arc/arc_service.h"
#include "components/arc/common/gnss.mojom.h"
#include "components/arc/instance_holder.h"
#include "mojo/public/cpp/bindings/binding.h"

namespace arc {

class ArcBridgeService;

class ArcGnssBridge : public ArcService,
                      public InstanceHolder<mojom::GnssInstance>::Observer,
                      public mojom::GnssHost {
 public:
  explicit ArcGnssBridge(ArcBridgeService* bridge_service);
  ~ArcGnssBridge() override;

  // InstanceHolder<mojom::GnssInstance>::Observer overrides:
  void OnInstanceReady() override;
  void OnInstanceClosed() override;

  // mojom::GnssHost overrides:
  void Start() override;
  void Stop() override;

 private:
  void OnPeriodicTask();

  base::ThreadChecker thread_checker_;
  mojo::Binding<mojom::GnssHost> binding_;
  base::RepeatingTimer timer_;

  double latitude_ = 0;
  double longitude_ = 0;
  double altitude_ = 0;
  double speed_ = 0;
  double bearing_ = 0;

  // WeakPtrFactory to use for callbacks.
  base::WeakPtrFactory<ArcGnssBridge> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(ArcGnssBridge);
};

}  // namespace arc

#endif  // COMPONENTS_ARC_GNSS_ARC_GNSS_BRIDGE_H_
