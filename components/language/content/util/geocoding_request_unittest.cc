// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/language/content/util/geocoding_request.h"
#include "base/macros.h"
#include "base/message_loop/message_loop.h"
#include "base/run_loop.h"
#include "net/http/http_status_code.h"
#include "net/url_request/test_url_fetcher_factory.h"
#include "net/url_request/url_fetcher_impl.h"
#include "net/url_request/url_request_status.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace language {
namespace {

const char kSuccessResponse[] = R"(
  {
   "results" : [
      {
         "address_components" : [
            {
               "long_name" : "Manipur",
               "short_name" : "MN",
               "types" : [ "administrative_area_level_1", "political" ]
            },
            {
               "long_name" : "India",
               "short_name" : "IN",
               "types" : [ "country", "political" ]
            }
         ],
         "formatted_address" : "Manipur, India",
         "geometry" : {
            "bounds" : {
               "northeast" : {
                  "lat" : 25.691874,
                  "lng" : 94.74324
               },
               "southwest" : {
                  "lat" : 23.8360479,
                  "lng" : 92.97107799999999
               }
            },
            "location" : {
               "lat" : 24.6637173,
               "lng" : 93.90626879999999
            },
            "location_type" : "APPROXIMATE",
            "viewport" : {
               "northeast" : {
                  "lat" : 25.691874,
                  "lng" : 94.74324
               },
               "southwest" : {
                  "lat" : 23.8360479,
                  "lng" : 92.97107799999999
               }
            }
         },
         "place_id" : "ChIJ25Bj8VsmSTcRrymo4BppwYw",
         "types" : [ "administrative_area_level_1", "political" ]
      }
   ],
   "status" : "OK"
}
)";

// Receive and valid the result.
class GeocodingReceiver {
 public:
  GeocodingReceiver(const std::string& expected_result,
                    bool expect_server_error)
      : expected_result_(expected_result),
        expect_server_error_(expect_server_error) {}

  void OnRequestDone(std::string result, bool server_error) {
    EXPECT_EQ(result, expected_result_);
    EXPECT_EQ(server_error, expect_server_error_);

    message_loop_runner_->Quit();
  }

  void WaitUntilRequestDone() {
    message_loop_runner_.reset(new base::RunLoop);
    message_loop_runner_->Run();
  }

 private:
  const std::string& expected_result_;
  const bool expect_server_error_;
  std::unique_ptr<base::RunLoop> message_loop_runner_;
};

class GeocodingRequestTest : public testing::Test {
 public:
  GeocodingRequestTest() : url_fetcher_factory_(nullptr) {
    if (!base::MessageLoop::current()) {
      // Create a temporary message loop if the current thread does not already
      // have one so we can use its task runner to create a request object.
      message_loop_.reset(new base::MessageLoopForIO);
    }
  }
  void SetFakeResponse(const GURL& url,
                       const std::string& data,
                       net::HttpStatusCode code,
                       net::URLRequestStatus::Status status) {
    url_fetcher_factory_.SetFakeResponse(url, data, code, status);
  }

 private:
  net::FakeURLFetcherFactory url_fetcher_factory_;
  std::unique_ptr<base::MessageLoopForIO> message_loop_;
};

}  // namespace

// Correct result, the json is correctly parsed and return the admin district
// area 1.
TEST_F(GeocodingRequestTest, CorrectResult) {
  const GURL test_url("https://maps.googleapis.com/maps/api/geocode/json");
  GeocodingRequest request(nullptr, test_url, 100.0, 100.0);
  SetFakeResponse(request.GetRequestURL(), kSuccessResponse, net::HTTP_OK,
                  net::URLRequestStatus::SUCCESS);

  GeocodingReceiver receiver("Manipur", false);
  request.MakeRequest(base::Bind(&GeocodingReceiver::OnRequestDone,
                                 base::Unretained(&receiver)));
  receiver.WaitUntilRequestDone();
}

// Google maps cannot find result and unknown should be returned.
TEST_F(GeocodingRequestTest, CannotFindResult) {
  const GURL test_url("https://maps.googleapis.com/maps/api/gecode/json");
  GeocodingRequest request(nullptr, test_url, 100.0, 100.0);
  SetFakeResponse(request.GetRequestURL(), "{\"results\":[]}", net::HTTP_OK,
                  net::URLRequestStatus::SUCCESS);

  GeocodingReceiver receiver("unknown", true);
  request.MakeRequest(base::Bind(&GeocodingReceiver::OnRequestDone,
                                 base::Unretained(&receiver)));
  receiver.WaitUntilRequestDone();
}

// Server error and unknown should be returned.
TEST_F(GeocodingRequestTest, ServerError) {
  const GURL test_url("https://maps.otherapis.com/maps/api/gecode/json");
  GeocodingRequest request(nullptr, test_url, 100.0, 100.0);
  LOG(ERROR) << request.GetRequestURL().GetContent();
  SetFakeResponse(request.GetRequestURL(), "", net::HTTP_NOT_FOUND,
                  net::URLRequestStatus::FAILED);

  GeocodingReceiver receiver("unknown", true);
  request.MakeRequest(base::Bind(&GeocodingReceiver::OnRequestDone,
                                 base::Unretained(&receiver)));
  receiver.WaitUntilRequestDone();
}

}  // namespace language