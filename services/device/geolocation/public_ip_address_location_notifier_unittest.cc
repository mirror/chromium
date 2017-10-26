// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/device/geolocation/public_ip_address_location_notifier.h"

#include "base/bind.h"
#include "base/test/scoped_task_environment.h"
#include "net/url_request/test_url_fetcher_factory.h"
#include "net/url_request/url_request_test_util.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace device {
namespace {

const char kExpectedRequestBaseUrl[] =
    "https://www.googleapis.com/geolocation/v1/geolocate";

// Simple request context producer that immediately produces a
// TestURLRequestContextGetter.
void TestRequestContextProducer(
    const scoped_refptr<base::SingleThreadTaskRunner>& network_task_runner,
    base::OnceCallback<void(scoped_refptr<net::URLRequestContextGetter>)>
        response_callback) {
  std::move(response_callback)
      .Run(base::MakeRefCounted<net::TestURLRequestContextGetter>(
          network_task_runner));
}

class PublicIpAddressLocationNotifierTest : public testing::Test {
 protected:
  using QueryNextPositionCallback =
      PublicIpAddressLocationNotifier::QueryNextPositionCallback;

  PublicIpAddressLocationNotifierTest()
      : scoped_task_environment_(
            base::test::ScopedTaskEnvironment::MainThreadType::IO),
        notifier_(
            base::Bind(&TestRequestContextProducer,
                       scoped_task_environment_.GetMainThreadTaskRunner()),
            std::string() /* api_key */) {}

  void ExpectAndRespondToUrlFetch(const float latitude, const float longitude) {
    // Expect a URLFetcher to have been created.
    net::URLFetcher* const fetcher = url_fetcher_factory_.GetFetcherByID(0);
    ASSERT_TRUE(fetcher);

    // Check that the URL looks right.
    EXPECT_THAT(fetcher->GetOriginalURL().possibly_invalid_spec(),
                testing::StartsWith(kExpectedRequestBaseUrl));

    // Respond with the specified latitude, longitude.
    // TODO(amoylan).
  }

  base::test::ScopedTaskEnvironment scoped_task_environment_;
  PublicIpAddressLocationNotifier notifier_;
  net::TestURLFetcherFactory url_fetcher_factory_;
};

TEST_F(PublicIpAddressLocationNotifierTest, Ok) {
  QueryNextPositionCallback callback = base::BindOnce([](const Geoposition&
                                                             position) {
    LOG(INFO)
        << "This should be called once the Respond TODO is implemented above.";
  });
  notifier_.QueryNextPositionAfterTimestamp(base::Time::Now(),
                                            std::move(callback));
  ExpectAndRespondToUrlFetch(1.2, 3.4);
}

}  // namespace
}  // namespace device
