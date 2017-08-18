// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <memory>
#include <string>
#include <vector>

#include "base/bind.h"
#include "base/callback.h"
#include "base/macros.h"
#include "base/memory/ptr_util.h"
#include "base/test/scoped_feature_list.h"
#include "base/time/time.h"
#include "base/values.h"
#include "components/network_error_logging/network_error_logging_service.h"
#include "net/base/ip_address.h"
#include "net/base/net_errors.h"
#include "net/reporting/reporting_service.h"
#include "net/socket/next_proto.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"
#include "url/origin.h"

namespace {

class TestReportingService : public net::ReportingService {
 public:
  struct Report {
    Report() {}

    Report(Report&& other)
        : url(other.url),
          group(other.group),
          type(other.type),
          body(std::move(other.body)) {}

    Report(const GURL& url,
           const std::string& group,
           const std::string& type,
           std::unique_ptr<const base::Value> body)
        : url(url), group(group), type(type), body(std::move(body)) {}

    ~Report() {}

    GURL url;
    std::string group;
    std::string type;
    std::unique_ptr<const base::Value> body;

   private:
    DISALLOW_COPY(Report);
  };

  TestReportingService() {}

  const std::vector<Report>& reports() const { return reports_; }

  // net::ReportingService implementation:

  ~TestReportingService() override {}

  void QueueReport(const GURL& url,
                   const std::string& group,
                   const std::string& type,
                   std::unique_ptr<const base::Value> body) override {
    reports_.push_back(Report(url, group, type, std::move(body)));
  }

  void ProcessHeader(const GURL& url,
                     const std::string& header_value) override {
    NOTREACHED();
  }

  void RemoveBrowsingData(
      int data_type_mask,
      base::Callback<bool(const GURL&)> origin_filter) override {
    NOTREACHED();
  }

 private:
  std::vector<Report> reports_;

  DISALLOW_COPY_AND_ASSIGN(TestReportingService);
};

class NetworkErrorLoggingServiceTest : public ::testing::Test {
 protected:
  NetworkErrorLoggingServiceTest() {
    scoped_feature_list_.InitAndEnableFeature(features::kNetworkErrorLogging);
    service_ = network_error_logging::NetworkErrorLoggingService::Create();
    CreateReportingService();
  }

  void CreateReportingService() {
    DCHECK(!reporting_service_);

    reporting_service_ = base::MakeUnique<TestReportingService>();
    service_->SetReportingService(reporting_service_.get());
  }

  void DestroyReportingService() {
    DCHECK(reporting_service_);

    service_->SetReportingService(nullptr);
    reporting_service_.reset();
  }

  network_error_logging::NetworkErrorLoggingService* service() {
    return service_.get();
  }
  const std::vector<TestReportingService::Report>& reports() {
    return reporting_service_->reports();
  }

  const GURL kUrl_ = GURL("https://example.com/");
  const url::Origin kOrigin_ = url::Origin(kUrl_);

  const std::string kHeader_ = "{\"report-to\":\"nel\",\"max-age\":86400}";

 private:
  base::test::ScopedFeatureList scoped_feature_list_;
  std::unique_ptr<network_error_logging::NetworkErrorLoggingService> service_;
  std::unique_ptr<TestReportingService> reporting_service_;
};

// TODO(juliatuttle): This should be a lambda.
void FillErrorDetails(
    GURL url,
    net::Error error_type,
    net::NetworkErrorLoggingDelegate::ErrorDetails* details_out) {
  details_out->uri = url;
  details_out->referrer = GURL("https://referrer.com/");
  details_out->server_ip = net::IPAddress::IPv4AllZeros();
  details_out->protocol = net::kProtoUnknown;
  details_out->status_code = 0;
  details_out->elapsed_time = base::TimeDelta::FromSeconds(1);
  details_out->time = base::TimeTicks::Now();
  details_out->type = error_type;
}

TEST_F(NetworkErrorLoggingServiceTest, FeatureDisabled) {
  // N.B. This test does not actually use the test fixture.

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndDisableFeature(features::kNetworkErrorLogging);

  auto service = network_error_logging::NetworkErrorLoggingService::Create();
  EXPECT_FALSE(service);
}

TEST_F(NetworkErrorLoggingServiceTest, FeatureEnabled) {
  EXPECT_TRUE(service());
}

TEST_F(NetworkErrorLoggingServiceTest, ReportQueued) {
  service()->OnHeader(kOrigin_, kHeader_);

  service()->OnNetworkError(
      kOrigin_, net::ERR_CONNECTION_REFUSED,
      base::Bind(&FillErrorDetails, kUrl_, net::ERR_CONNECTION_REFUSED));

  EXPECT_EQ(1u, reports().size());
}

TEST_F(NetworkErrorLoggingServiceTest, NoPolicyForOrigin) {
  service()->OnNetworkError(
      kOrigin_, net::ERR_CONNECTION_REFUSED,
      base::Bind(&FillErrorDetails, kUrl_, net::ERR_CONNECTION_REFUSED));

  EXPECT_TRUE(reports().empty());
}

TEST_F(NetworkErrorLoggingServiceTest, OriginInsecure) {
  const GURL kInsecureUrl("http://insecure.com/");
  const url::Origin kInsecureOrigin(kInsecureUrl);

  service()->OnHeader(kInsecureOrigin, kHeader_);

  service()->OnNetworkError(
      kInsecureOrigin, net::ERR_CONNECTION_REFUSED,
      base::Bind(&FillErrorDetails, kInsecureUrl, net::ERR_CONNECTION_REFUSED));

  EXPECT_TRUE(reports().empty());
}

TEST_F(NetworkErrorLoggingServiceTest, NoReportingService) {
  // TODO
}

}  // namespace
