// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/arc/gnss/arc_gnss_bridge.h"

#include "base/bind.h"
#include "base/logging.h"
#include "base/time/time.h"
#include "components/arc/arc_bridge_service.h"

namespace {

}  // namespace

namespace arc {

ArcGnssBridge::ArcGnssBridge(ArcBridgeService* bridge_service)
    : ArcService(bridge_service), binding_(this), weak_factory_(this) {
  LOG(INFO) << "@@ " << __func__;

  arc_bridge_service()->gnss()->AddObserver(this);
}

ArcGnssBridge::~ArcGnssBridge() {
  LOG(INFO) << "@@ " << __func__;

  DCHECK(thread_checker_.CalledOnValidThread());

  arc_bridge_service()->gnss()->RemoveObserver(this);
}

void ArcGnssBridge::OnInstanceReady() {
  LOG(INFO) << "@@ " << __func__;

  mojom::GnssInstance* gnss_instance =
      ARC_GET_INSTANCE_FOR_METHOD(arc_bridge_service()->gnss(), Init);
  DCHECK(gnss_instance);

  gnss_instance->Init(binding_.CreateInterfacePtrAndBind());

  timer_.Start(FROM_HERE, base::TimeDelta::FromSeconds(1), this,
               &ArcGnssBridge::OnPeriodicTask);
}

void ArcGnssBridge::OnInstanceClosed() {
  LOG(INFO) << "@@ " << __func__;

  timer_.Stop();
}

void ArcGnssBridge::Start() {
}

void ArcGnssBridge::Stop() {
  LOG(INFO) << "@@ " << __func__;
}

void ArcGnssBridge::OnPeriodicTask() {
  LOG(INFO) << "@@ #0" << __func__;

  auto* gnss_instance = ARC_GET_INSTANCE_FOR_METHOD(
      arc_bridge_service()->gnss(), OnLocationUpdate);
  if (!gnss_instance)
    return;

  LOG(INFO) << "@@ #1" << __func__;
  arc::mojom::GnssLocationPtr location = arc::mojom::GnssLocation::New();

  latitude_ -= 1;
  if (latitude_ < -90) latitude_ = 90;
  if (latitude_ > 90) latitude_ = -90;

  longitude_ += 1;
  if (longitude_ < -180) longitude_ = 180;
  if (longitude_ > 180) longitude_ = -180;

  altitude_ += 10;
  if (altitude_ < 200) altitude_ = 10;

  speed_ += 5;
  if (speed_ >= 100) speed_ = 50;

  bearing_ += 1;
  if (bearing_ >= 360) bearing_ = 0;

  location->longitude = longitude_;
  location->latitude = latitude_;
  location->altitude = altitude_;
  location->speed = speed_;
  location->bearing = bearing_;
  location->accuracy = 1;
  location->timestamp = static_cast<int64_t>(base::Time::Now().ToTimeT());

  gnss_instance->OnLocationUpdate(std::move(location));
}

}  // namespace arc
