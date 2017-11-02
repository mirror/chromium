// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Basic test of the PublicIpAddressGeolocator implementation of the
// device::mojom::Geolocation interface, and basic test of the
// PublicIpAddressGeolocationProvider interface that supplies instances thereof.

#include "device/geolocation/public/interfaces/geolocation.mojom.h"
#include "net/url_request/test_url_fetcher_factory.h"
#include "services/device/device_service_test_base.h"
#include "services/device/geolocation/public_ip_address_geolocation_provider.h"
#include "services/device/public/interfaces/constants.mojom.h"
#include "services/device/public/interfaces/public_ip_address_geolocation_provider.mojom.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace device {
namespace {

class PublicIpAddressGeolocatorTest : public DeviceServiceTestBase {
 public:
  PublicIpAddressGeolocatorTest() = default;
  ~PublicIpAddressGeolocatorTest() override = default;

 protected:
  void SetUp() override {
    DeviceServiceTestBase::SetUp();

    // Get a PublicIpAddressGeolocationProvider.
    connector()->BindInterface(mojom::kServiceName,
                               &public_ip_address_geolocation_provider_);

    // Use the PublicIpAddressGeolocationProvider to get a Geolocation.
    public_ip_address_geolocation_provider_->CreateGeolocation(
        mojo::MakeRequest(&public_ip_address_geolocator_));
  }

  // Invokes QueryNextPosition on |public_ip_address_geolocator_|, and runs
  // |done_closure| when the response comes back.
  void QueryNextPosition(base::Closure done_closure) {
    public_ip_address_geolocator_->QueryNextPosition(base::BindOnce(
        &PublicIpAddressGeolocatorTest::OnQueryNextPositionResponse,
        base::Unretained(this), done_closure));
  }

  // Callback for QueryNextPosition() that records the result in |position_| and
  // then invokes |done_closure|.
  void OnQueryNextPositionResponse(base::Closure done_closure,
                                   mojom::GeopositionPtr position) {
    position_ = std::move(position);
    done_closure.Run();
  }

  // Result of the latest completed call to QueryNextPosition.
  mojom::GeopositionPtr position_;

  // The interfaces under test.
  mojom::PublicIpAddressGeolocationProviderPtr
      public_ip_address_geolocation_provider_;
  mojom::GeolocationPtr public_ip_address_geolocator_;

  DISALLOW_COPY_AND_ASSIGN(PublicIpAddressGeolocatorTest);
};

// Observer that waits until a TestURLFetcher with the specified fetcher_id
// starts, after which it is made available through .fetcher().
class TestURLFetcherObserver : public net::TestURLFetcher::DelegateForTests {
 public:
  explicit TestURLFetcherObserver(int expected_fetcher_id)
      : expected_fetcher_id_(expected_fetcher_id) {
    factory_.SetDelegateForTests(this);
  }
  virtual ~TestURLFetcherObserver() {}

  void Wait() { loop_.Run(); }

  net::TestURLFetcher* fetcher() { return fetcher_; }

  // net::TestURLFetcher::DelegateForTests:
  void OnRequestStart(int fetcher_id) override {
    if (fetcher_id == expected_fetcher_id_) {
      fetcher_ = factory_.GetFetcherByID(fetcher_id);
      fetcher_->SetDelegateForTests(nullptr);
      factory_.SetDelegateForTests(nullptr);
      loop_.Quit();
    }
  }
  void OnChunkUpload(int fetcher_id) override {}
  void OnRequestEnd(int fetcher_id) override {}

 private:
  const int expected_fetcher_id_;
  net::TestURLFetcher* fetcher_ = nullptr;
  net::TestURLFetcherFactory factory_;
  base::RunLoop loop_;
};

// Basic test of binding a mojom::Geolocation via a
// PublicIpAddressGeolocationProvider and invoking QueryNextPosition.
TEST_F(PublicIpAddressGeolocatorTest, BindAndQuery) {
  // Intercept the URLFetcher from network geolocation request.
  TestURLFetcherObserver observer(
      device::NetworkLocationRequest::url_fetcher_id_for_tests);

  // Invoke QueryNextPositiona and wait for a URLFetcher.
  base::RunLoop loop;
  QueryNextPosition(loop.QuitClosure());
  observer.Wait();
  DCHECK(observer.fetcher());

  // Issue a valid response to the URLFetcher.
  observer.fetcher()->set_url(observer.fetcher()->GetOriginalURL());
  observer.fetcher()->set_status(net::URLRequestStatus());
  observer.fetcher()->set_response_code(200);
  observer.fetcher()->SetResponseString(R"({
        "accuracy": 100.0,
        "location": {
          "lat": 10.0,
          "lng": 20.0
        }
      })");
  // The response must be issued on the IO thread.
  io_thread_.task_runner()->PostTask(
      FROM_HERE,
      base::BindOnce(&net::URLFetcherDelegate::OnURLFetchComplete,
                     base::Unretained(observer.fetcher()->delegate()),
                     base::Unretained(observer.fetcher())));

  // Wait for QueryNextPosition to return.
  loop.Run();

  EXPECT_THAT(position_->accuracy, testing::Eq(100.0));
  EXPECT_THAT(position_->latitude, testing::Eq(10.0));
  EXPECT_THAT(position_->longitude, testing::Eq(20.0));
}

}  // namespace
}  // namespace device
