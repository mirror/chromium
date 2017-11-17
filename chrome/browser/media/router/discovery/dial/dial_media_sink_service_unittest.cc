// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/media/router/discovery/dial/dial_media_sink_service.h"
#include "base/run_loop.h"
#include "base/test/mock_callback.h"
#include "base/test/test_simple_task_runner.h"
#include "chrome/browser/media/router/discovery/dial/dial_media_sink_service_impl.h"
#include "chrome/browser/media/router/test_helper.h"
#include "chrome/test/base/testing_profile.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using ::testing::_;
using ::testing::InvokeWithoutArgs;
using ::testing::Return;

namespace media_router {

class MockDialMediaSinkServiceImpl : public DialMediaSinkServiceImpl {
 public:
  MockDialMediaSinkServiceImpl(
      const OnSinksDiscoveredCallback& sinks_discovered_cb,
      const OnDialSinkAddedCallback& dial_sink_added_cb,
      const scoped_refptr<net::URLRequestContextGetter>& request_context,
      const scoped_refptr<base::SingleThreadTaskRunner>& task_runner)
      : DialMediaSinkServiceImpl(sinks_discovered_cb,
                                 dial_sink_added_cb,
                                 request_context),
        sinks_discovered_cb_(sinks_discovered_cb) {
    SetTaskRunnerForTest(task_runner);
  }
  ~MockDialMediaSinkServiceImpl() override = default;

  MOCK_METHOD0(Start, void());
  MOCK_METHOD0(Stop, void());
  MOCK_METHOD0(ForceSinkDiscoveryCallback, void());

  OnSinksDiscoveredCallback sinks_discovered_cb() {
    return sinks_discovered_cb_;
  }

 private:
  OnSinksDiscoveredCallback sinks_discovered_cb_;
};

class TestDialMediaSinkService : public DialMediaSinkService {
 public:
  explicit TestDialMediaSinkService(
      content::BrowserContext* context,
      const scoped_refptr<base::TestSimpleTaskRunner>& task_runner)
      : DialMediaSinkService(context), task_runner_(task_runner) {}
  ~TestDialMediaSinkService() override = default;

  std::unique_ptr<DialMediaSinkServiceImpl> CreateImpl(
      const OnSinksDiscoveredCallback& sinks_discovered_cb,
      const OnDialSinkAddedCallback& dial_sink_added_cb,
      const scoped_refptr<net::URLRequestContextGetter>& request_context)
      override {
    auto mock_impl = std::make_unique<MockDialMediaSinkServiceImpl>(
        sinks_discovered_cb, dial_sink_added_cb, request_context, task_runner_);
    mock_impl_ = mock_impl.get();
    return mock_impl;
  }

  MockDialMediaSinkServiceImpl* mock_impl() { return mock_impl_; }

 private:
  scoped_refptr<base::TestSimpleTaskRunner> task_runner_;
  MockDialMediaSinkServiceImpl* mock_impl_ = nullptr;
};

class DialMediaSinkServiceTest : public ::testing::Test {
 public:
  DialMediaSinkServiceTest()
      : task_runner_(new base::TestSimpleTaskRunner()),
        service_(&profile_, task_runner_) {}

  void SetUp() override {
    service_.Start(
        base::BindRepeating(&DialMediaSinkServiceTest::OnSinksDiscovered,
                            base::Unretained(this)),
        OnDialSinkAddedCallback());
    mock_impl_ = service_.mock_impl();
    ASSERT_TRUE(mock_impl_);
    EXPECT_CALL(*mock_impl_, Start()).WillOnce(InvokeWithoutArgs([this]() {
      EXPECT_TRUE(this->task_runner_->RunsTasksInCurrentSequence());
    }));
    task_runner_->RunUntilIdle();
  }

  void TearDown() override {
    if (mock_impl_) {
      EXPECT_CALL(*mock_impl_, Stop()).WillOnce(InvokeWithoutArgs([this]() {
        EXPECT_TRUE(this->task_runner_->RunsTasksInCurrentSequence());
      }));
      service_.Stop();
      task_runner_->RunUntilIdle();
    }
  }

  MOCK_METHOD1(OnSinksDiscovered, void(std::vector<MediaSinkInternal> sinks));

 protected:
  content::TestBrowserThreadBundle thread_bundle_;
  TestingProfile profile_;

  scoped_refptr<base::TestSimpleTaskRunner> task_runner_;
  TestDialMediaSinkService service_;
  MockDialMediaSinkServiceImpl* mock_impl_ = nullptr;

  DISALLOW_COPY_AND_ASSIGN(DialMediaSinkServiceTest);
};

TEST_F(DialMediaSinkServiceTest, TestForceSinkDiscoveryCallback) {
  EXPECT_CALL(*mock_impl_, ForceSinkDiscoveryCallback())
      .WillOnce(InvokeWithoutArgs([this]() {
        EXPECT_TRUE(this->task_runner_->RunsTasksInCurrentSequence());
      }));

  service_.ForceSinkDiscoveryCallback();
  task_runner_->RunUntilIdle();

  // Actually invoke the callback.
  task_runner_->PostTask(FROM_HERE,
                         base::BindOnce(mock_impl_->sinks_discovered_cb(),
                                        std::vector<MediaSinkInternal>()));
  task_runner_->RunUntilIdle();

  EXPECT_CALL(*this, OnSinksDiscovered(_)).WillOnce(InvokeWithoutArgs([]() {
    EXPECT_TRUE(
        content::BrowserThread::CurrentlyOn(content::BrowserThread::UI));
  }));
  base::RunLoop().RunUntilIdle();
}

}  // namespace media_router
