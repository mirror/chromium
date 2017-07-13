// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/geolocation/geolocation_service_impl.h"

#include "content/public/browser/permission_type.h"
#include "content/test/mock_permission_manager.h"
#include "content/test/mock_permission_manager.h"
#include "content/test/test_render_frame_host.h"
#include "device/geolocation/geolocation_context.h"
#include "device/geolocation/geoposition.h"
#include "device/geolocation/public/interfaces/geolocation.mojom.h"
#include "device/geolocation/public/interfaces/geoposition.mojom.h"
#include "testing/gtest/include/gtest/gtest.h"

using blink::mojom::PermissionStatus;
using device::GeolocationContext;
using device::Geoposition;
using device::mojom::GeolocationPtr;
using device::mojom::GeopositionPtr;
using device::mojom::GeolocationService;

namespace content {
namespace {

double kMockLatitude = 1.0;
double kMockLongitude = 10.0;

class TestPermissionManager : public MockPermissionManager {
 public:
  ~TestPermissionManager() override = default;

  int RequestPermission(
      PermissionType permissions,
      RenderFrameHost* render_frame_host,
      const GURL& requesting_origin,
      bool user_gesture,
      const base::Callback<void(PermissionStatus)>& callback) override {
    EXPECT_EQ(permissions, PermissionType::GEOLOCATION);
    EXPECT_TRUE(user_gesture);
    callback.Run(status_);
    return 0;
  }

  void SetPermissionStatus(PermissionStatus status) { status_ = status; }

 private:
  PermissionStatus status_;
};

class GeolocationServiceTest : public RenderViewHostImplTestHarness {
 protected:
  GeolocationServiceTest() {}

  ~GeolocationServiceTest() override {}

  void SetUp() override {
    RenderViewHostImplTestHarness::SetUp();
    service_.reset(new GeolocationServiceImpl(&context_, &permission_manager_,
                                              main_rfh()));
  }

  GeolocationService* service() { return service_.get(); }

  GeolocationContext* context() { return &context_; }

  void SetGranted(bool granted) {
    permission_manager_.SetPermissionStatus(granted ? PermissionStatus::GRANTED
                                                    : PermissionStatus::DENIED);
  }

 private:
  std::unique_ptr<GeolocationServiceImpl> service_;
  GeolocationContext context_;
  TestPermissionManager permission_manager_;

  DISALLOW_COPY_AND_ASSIGN(GeolocationServiceTest);
};

}  // namespace

TEST_F(GeolocationServiceTest, PermissionGranted) {
  SetGranted(true);
  GeolocationPtr geolocation;
  service()->CreateGeolocation(mojo::MakeRequest(&geolocation), true);

  base::RunLoop loop;
  geolocation.set_connection_error_handler(base::Bind([] {
    ADD_FAILURE() << "Connection error handler called unexpectedly";
  }));
  geolocation->QueryNextPosition(
      base::Bind([](base::Callback<void()> callback,
                    GeopositionPtr geoposition) {
    EXPECT_DOUBLE_EQ(kMockLatitude, geoposition->latitude);
    EXPECT_DOUBLE_EQ(kMockLongitude, geoposition->longitude);
    callback.Run();
  }, loop.QuitClosure()));
  std::unique_ptr<Geoposition> mock_geoposition =
      base::MakeUnique<Geoposition>();
  mock_geoposition->latitude = kMockLatitude;
  mock_geoposition->longitude = kMockLongitude;
  context()->SetOverride(std::move(mock_geoposition));
  loop.Run();

}

TEST_F(GeolocationServiceTest, PermissionDenied) {
  SetGranted(false);
  GeolocationPtr geolocation;
  service()->CreateGeolocation(mojo::MakeRequest(&geolocation), true);

  base::RunLoop loop;
  geolocation.set_connection_error_handler(
      base::Bind([](base::Callback<void()> callback) {
    callback.Run();
  }, loop.QuitClosure()));
  geolocation->QueryNextPosition(base::Bind([](GeopositionPtr geoposition) {
    ADD_FAILURE() << "Position updated unexpectedly";
  }));
  std::unique_ptr<Geoposition> mock_geoposition =
      base::MakeUnique<Geoposition>();
  mock_geoposition->latitude = kMockLatitude;
  mock_geoposition->longitude = kMockLongitude;
  context()->SetOverride(std::move(mock_geoposition));
  loop.Run();
}

}  // namespace content
