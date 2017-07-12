// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/geolocation/geolocation_service_impl.h"

#include "content/test/mock_permission_manager.h"
#include "content/test/test_render_frame_host.h"
#include "device/geolocation/geolocation_context.h"
#include "device/geolocation/geoposition.h"
#include "device/geolocation/public/interfaces/geolocation.mojom.h"
#include "device/geolocation/public/interfaces/geoposition.mojom.h"
#include "testing/gtest/include/gtest/gtest.h"

using blink::mojom::PermissionStatus;

namespace device {
namespace {

class TestPermissionManager : public content::MockPermissionManager {
 public:
  ~TestPermissionManager() override = default;

  int RequestPermission(
      content::PermissionType permissions,
      content::RenderFrameHost* render_frame_host,
      const GURL& requesting_origin,
      bool user_gesture,
      const base::Callback<void(PermissionStatus)>& callback) override {
    callback.Run(status_);
    return 0;
  }

  void SetPermissionStatus(PermissionStatus status) { status_ = status; }

 private:
  PermissionStatus status_;
};

class GeolocationServiceTest : public content::RenderViewHostImplTestHarness {
 public:
  void OnGeolocationConnectionError() { error_ = true; }

  void OnPositionUpdated(mojom::GeopositionPtr geoposition) {
    geoposition_ = *geoposition;
  }

 protected:
  GeolocationServiceTest() {}

  ~GeolocationServiceTest() override {}

  void SetUp() override {
    RenderViewHostImplTestHarness::SetUp();
    service_.reset(new GeolocationServiceImpl(&context_, &permission_manager_,
                                              main_rfh()));
    error_ = false;
  }

  mojom::GeolocationService* service() { return service_.get(); }

  GeolocationContext* context() { return &context_; }

  bool error() { return error_; }

  mojom::Geoposition geoposition() { return geoposition_; }

  void SetGranted(bool granted) {
    permission_manager_.SetPermissionStatus(granted ? PermissionStatus::GRANTED
                                                    : PermissionStatus::DENIED);
  }

 private:
  std::unique_ptr<GeolocationServiceImpl> service_;
  GeolocationContext context_;
  mojom::Geoposition geoposition_;
  TestPermissionManager permission_manager_;
  bool error_;

  DISALLOW_COPY_AND_ASSIGN(GeolocationServiceTest);
};

}  // namespace

TEST_F(GeolocationServiceTest, PermissionGranted) {
  SetGranted(true);
  mojom::GeolocationPtr geolocation;
  service()->CreateGeolocation(mojo::MakeRequest(&geolocation), true);
  geolocation.set_connection_error_handler(
      base::Bind(&GeolocationServiceTest::OnGeolocationConnectionError,
                 base::Unretained(this)));

  geolocation->QueryNextPosition(base::Bind(
      &GeolocationServiceTest::OnPositionUpdated, base::Unretained(this)));
  std::unique_ptr<Geoposition> coordinates = base::MakeUnique<Geoposition>();
  coordinates->latitude = 1.0;
  coordinates->longitude = 10.0;
  context()->SetOverride(std::move(coordinates));
  base::RunLoop().RunUntilIdle();

  EXPECT_FALSE(error());
  EXPECT_DOUBLE_EQ(1.0, geoposition().latitude);
  EXPECT_DOUBLE_EQ(10.0, geoposition().longitude);
}

TEST_F(GeolocationServiceTest, PermissionDenied) {
  SetGranted(false);
  mojom::GeolocationPtr geolocation;
  service()->CreateGeolocation(mojo::MakeRequest(&geolocation), true);
  geolocation.set_connection_error_handler(
      base::Bind(&GeolocationServiceTest::OnGeolocationConnectionError,
                 base::Unretained(this)));

  geolocation->QueryNextPosition(base::Bind(
      &GeolocationServiceTest::OnPositionUpdated, base::Unretained(this)));
  std::unique_ptr<Geoposition> coordinates = base::MakeUnique<Geoposition>();
  coordinates->latitude = 1.0;
  coordinates->longitude = 10.0;
  context()->SetOverride(std::move(coordinates));
  base::RunLoop().RunUntilIdle();

  EXPECT_TRUE(error());
  EXPECT_DOUBLE_EQ(0.0, geoposition().latitude);
  EXPECT_DOUBLE_EQ(0.0, geoposition().longitude);
}

}  // namespace device
