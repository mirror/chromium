// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/run_loop.h"
#include "device/geolocation/network_location_request.h"
#include "google_apis/google_api_keys.h"
#include "mojo/public/cpp/bindings/interface_ptr.h"
#include "net/base/escape.h"
#include "net/url_request/test_url_fetcher_factory.h"
#include "services/device/device_service_test_base.h"
#include "services/device/public/interfaces/constants.mojom.h"
#include "services/device/public/interfaces/geolocation.mojom.h"
#include "services/device/public/interfaces/geolocation_context.mojom.h"
#include "services/device/public/interfaces/geolocation_control.mojom.h"

namespace device {

namespace {

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

class GeolocationServiceUnitTest : public DeviceServiceTestBase {
 public:
  GeolocationServiceUnitTest() = default;
  ~GeolocationServiceUnitTest() override = default;

 protected:
  void SetUp() override {
    DeviceServiceTestBase::SetUp();
    connector()->BindInterface(mojom::kServiceName, &geolocation_control_);
    geolocation_control_->UserDidOptIntoLocationServices();

    connector()->BindInterface(mojom::kServiceName, &geolocation_context_);
    geolocation_context_->BindGeolocation(MakeRequest(&geolocation_));
  }

  mojom::GeolocationControlPtr geolocation_control_;
  mojom::GeolocationContextPtr geolocation_context_;
  mojom::GeolocationPtr geolocation_;

  DISALLOW_COPY_AND_ASSIGN(GeolocationServiceUnitTest);
};

#if defined(OS_CHROMEOS)
// ChromeOS fails to perform network geolocation when zero wifi networks are
// detected in a scan: https://crbug.com/767300.
#else
TEST_F(GeolocationServiceUnitTest, UrlWithApiKey) {
  // Unique ID (derived from Gerrit CL number):
  device::NetworkLocationRequest::url_fetcher_id_for_tests = 675023;

  // Intercept the URLFetcher from network geolocation request.
  TestURLFetcherObserver observer(
      device::NetworkLocationRequest::url_fetcher_id_for_tests);

  geolocation_->SetHighAccuracy(true);

  observer.Wait();
  DCHECK(observer.fetcher());

  // Verify full URL including Google API key.
  const std::string expected_url =
      "https://www.googleapis.com/geolocation/v1/geolocate?key=" +
      net::EscapeQueryParamValue(google_apis::GetAPIKey(), true);
  EXPECT_EQ(expected_url, observer.fetcher()->GetOriginalURL());
}
#endif

}  // namespace

}  // namespace device
