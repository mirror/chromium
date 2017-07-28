// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/service_worker/service_worker_url_loader_job.h"

#include "content/browser/loader/url_loader_request_handler.h"
#include "content/browser/service_worker/embedded_worker_test_helper.h"
#include "content/browser/service_worker/service_worker_context_core.h"
#include "content/browser/service_worker/service_worker_registration.h"
#include "content/browser/service_worker/service_worker_version.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "storage/browser/blob/blob_storage_context.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace content {

// ServiceWorkerURLLoaderJobTest is for testing the handling of requests
// by a service worker via ServiceWorkerURLLoaderJob.
//
// ServiceWorkerURLLoaderJobTest is also a ServiceWorkerURLLoaderJob::Delegate.
// In production code, ServiceWorkerControlleeRequestHandler is the Delegate
// (for non-"foreign fetch" request interceptions). So this class also basically
// mocks that part of ServiceWorkerControlleeRequestHandler.
class ServiceWorkerURLLoaderJobTest
    : public testing::Test,
      public ServiceWorkerURLLoaderJob::Delegate {
 public:
  ServiceWorkerURLLoaderJobTest()
      : thread_bundle_(TestBrowserThreadBundle::IO_MAINLOOP),
        helper_(base::MakeUnique<EmbeddedWorkerTestHelper>(base::FilePath())) {}
  ~ServiceWorkerURLLoaderJobTest() override = default;

  void SetUp() override {
    // Create an active service worker.
    registration_ = new ServiceWorkerRegistration(
        ServiceWorkerRegistrationOptions(GURL("https://example.com/")), 1L,
        helper_->context()->AsWeakPtr());
    version_ = new ServiceWorkerVersion(
        registration_.get(), GURL("https://example.com/service_worker.js"), 1L,
        helper_->context()->AsWeakPtr());
    version_->set_fetch_handler_existence(
        ServiceWorkerVersion::FetchHandlerExistence::EXISTS);
    version_->SetStatus(ServiceWorkerVersion::ACTIVATED);
    registration_->SetActiveVersion(version_);
  }

  void TearDown() override {}

  base::WeakPtr<storage::BlobStorageContext> GetStorageContext() {
    return blob_context_.AsWeakPtr();
  }

 private:
  // ServiceWorkerURLLoaderJob::Delegate --------------------------------------
  void OnPrepareToRestart() override {}

  ServiceWorkerVersion* GetServiceWorkerVersion(
      ServiceWorkerMetrics::URLRequestJobResult* result) override {
    return version_.get();
  }

  bool RequestStillValid(
      ServiceWorkerMetrics::URLRequestJobResult* result) override {
    return true;
  }

  void MainResourceLoadFailed() override {}
  // --------------------------------------------------------------------------

  TestBrowserThreadBundle thread_bundle_;

  std::unique_ptr<EmbeddedWorkerTestHelper> helper_;
  scoped_refptr<ServiceWorkerRegistration> registration_;
  scoped_refptr<ServiceWorkerVersion> version_;
  storage::BlobStorageContext blob_context_;
};

TEST_F(ServiceWorkerURLLoaderJobTest, Basic) {
  ResourceRequest request;
  request.url = GURL("https://www.example.com/");
  request.method = "GET";

  LoaderCallback callback;
  auto job = base::MakeUnique<ServiceWorkerURLLoaderJob>(
      std::move(callback), this, request, GetStorageContext());
  job->ForwardToServiceWorker();
}

}  // namespace content
