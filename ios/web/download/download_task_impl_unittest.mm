// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/web/download/download_task_impl.h"

#import <Foundation/Foundation.h>

#include "base/memory/ptr_util.h"
#include "base/strings/utf_string_conversions.h"
#import "ios/web/public/test/fakes/test_web_state.h"
#include "ios/web/public/test/web_test.h"
#include "net/url_request/url_fetcher_response_writer.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "testing/gtest_mac.h"
#import "third_party/ocmock/OCMock/OCMock.h"
#include "third_party/ocmock/gtest_support.h"
#include "url/gurl.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace web {

namespace {

const char kUrl[] = "chromium://download.test/";
const char kContentDisposition[] = "attachment; filename=file.test";
const char kMimeType[] = "application/pdf";

class MockDownloadTaskImplDelegate : public DownloadTaskImpl::Delegate {
 public:
  MockDownloadTaskImplDelegate()
      : configuration_([NSURLSessionConfiguration
            backgroundSessionConfigurationWithIdentifier:
                [NSUUID UUID].UUIDString]) {}
  MOCK_METHOD1(OnTaskUpdated, void(DownloadTaskImpl* task));
  MOCK_METHOD1(OnTaskDestroyed, void(DownloadTaskImpl* task));
  NSURLSession* CreateSession(NSString* identifier,
                              id<NSURLSessionDelegate> delegate) {
    EXPECT_FALSE(session_);
    session_ = OCMStrictClassMock([NSURLSession class]);
    OCMStub([session_ configuration]).andReturn(configuration_);
    return session_;
  }

  NSURLSessionConfiguration* configuration() { return configuration_; }
  id session() { return session_; }

 private:
  id configuration_;
  id session_;
};

}  //  namespace

// Test fixture for testing DownloadTaskImplTest class.
class DownloadTaskImplTest : public WebTest {
 protected:
  DownloadTaskImplTest()
      : task_(new DownloadTaskImpl(&web_state_,
                                   GURL(kUrl),
                                   kContentDisposition,
                                   kMimeType,
                                   delegate_.configuration().identifier,
                                   &delegate_)) {
    EXPECT_CALL(delegate_, OnTaskDestroyed(task_.get()));
  }

  // Starts download and return NSURLSessionDataTask mock for this task.
  id Start() {
    NSURL* url = [NSURL URLWithString:@(kUrl)];
    id session_task = OCMStrictClassMock([NSURLSessionDataTask class]);
    OCMExpect([delegate_.session() dataTaskWithURL:url])
        .andReturn(session_task);
    OCMExpect([session_task resume]);
    task_->SetResponseWriter(base::MakeUnique<net::URLFetcherStringWriter>());
    task_->Start();
    return session_task;
  }

  MockDownloadTaskImplDelegate delegate_;
  TestWebState web_state_;
  scoped_refptr<DownloadTaskImpl> task_;  // task_ has to outlive delegate_.
};

// Tests DownloadTaskImpl default state after construction.
TEST_F(DownloadTaskImplTest, DefaultState) {
  EXPECT_EQ(&web_state_, task_->web_state());
  EXPECT_FALSE(task_->GetResponseWriter());
  EXPECT_NSEQ(delegate_.configuration().identifier, task_->GetIndentifier());
  EXPECT_EQ(kUrl, task_->GetOriginalUrl());
  EXPECT_FALSE(task_->IsDone());
  EXPECT_EQ(0, task_->GetErrorCode());
  EXPECT_EQ(0, task_->GetTotalBytes());
  EXPECT_EQ(0, task_->GetPercentComplete());
  EXPECT_EQ(kContentDisposition, task_->GetContentDisposition());
  EXPECT_EQ(kMimeType, task_->GetMimeType());
  EXPECT_EQ("file.test", base::UTF16ToUTF8(task_->GetSuggestedFilename()));
  base::Time time;
  EXPECT_FALSE(task_->GetTimeRemaining(&time));
}

// Tests sucessfull download of response with only one
// URLSession:dataTask:didReceiveData: callback.
TEST_F(DownloadTaskImplTest, SmallResponseDownload) {
  id session_task = Start();
  OCMExpect([session_task cancel]);
}

// Tests sucessfull download of response with multiple
// URLSession:dataTask:didReceiveData: callbacks.
TEST_F(DownloadTaskImplTest, LargeResponseDownload) {}

// Tests failed download when URLSession:dataTask:didReceiveData: callback was
// not even called.
TEST_F(DownloadTaskImplTest, FailureInTheBeginning) {}

// Tests failed download when URLSession:dataTask:didReceiveData: callback was
// called once.
TEST_F(DownloadTaskImplTest, FailureInTheMiddle) {}

// Tests sucessfull download of a response with bad SSL cert if user has
// previously accepted the load for this cert.
TEST_F(DownloadTaskImplTest, AcceptBadCert) {}

// Tests download rejection of a response with bad SSL cert.
TEST_F(DownloadTaskImplTest, RejectBadCert) {}

// Tests that NSURLSessionConfiguration contains up to date cookie from browser
// state before the download started.
TEST_F(DownloadTaskImplTest, Cookie) {}

// Tests that destructing DownloadTaskImpl calls -[NSURLSessionDataTask cancel].
TEST_F(DownloadTaskImplTest, DownloadTaskDestruction) {
  id session_task = Start();
  OCMExpect([session_task cancel]);
  task_ = nullptr;  // Destruct DownloadTaskImpl.
}

}  // namespace web
