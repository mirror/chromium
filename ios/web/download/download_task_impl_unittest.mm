// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/web/download/download_task_impl.h"

#import <Foundation/Foundation.h>

#include "base/memory/ptr_util.h"
#include "base/strings/utf_string_conversions.h"
#import "ios/testing/wait_util.h"
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

using testing::WaitUntilConditionOrTimeout;

@interface TestNSURLSessionTask : NSURLSessionDataTask
@property(nonatomic) NSURLSessionTaskState state;
@property(nonatomic) int64_t countOfBytesReceived;
@property(nonatomic) int64_t countOfBytesExpectedToReceive;

@property(nonatomic) BOOL cancelled;
@property(nonatomic) BOOL resumed;

- (void)cancel;
- (void)resume;
@end

@implementation TestNSURLSessionTask
@synthesize state = _state;
@synthesize countOfBytesReceived = _countOfBytesReceived;
@synthesize countOfBytesExpectedToReceive = _countOfBytesExpectedToReceive;
@synthesize cancelled = _cancelled;
@synthesize resumed = _resumed;

- (void)cancel {
  self.cancelled = YES;
}
- (void)resume {
  self.resumed = YES;
}

@end

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
                              id<NSURLSessionDataDelegate> delegate) {
    EXPECT_FALSE(session_);
    EXPECT_FALSE(session_delegate_);
    session_delegate_ = delegate;
    session_ = OCMStrictClassMock([NSURLSession class]);
    OCMStub([session_ configuration]).andReturn(configuration_);
    return session_;
  }

  NSURLSessionConfiguration* configuration() { return configuration_; }
  id session() { return session_; }
  id<NSURLSessionDataDelegate> session_delegate() { return session_delegate_; }

 private:
  id<NSURLSessionDataDelegate> session_delegate_;
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
                                   -1,
                                   kMimeType,
                                   task_delegate_.configuration().identifier,
                                   &task_delegate_)) {
  }

  ~DownloadTaskImplTest() override {
    if (task_) {
      EXPECT_CALL(task_delegate_, OnTaskDestroyed(task_.get()));
    }
  }

  // Starts download and return NSURLSessionDataTask mock for this task.
  TestNSURLSessionTask* Start() {
    NSURL* url = [NSURL URLWithString:@(kUrl)];
    TestNSURLSessionTask* session_task = [[TestNSURLSessionTask alloc] init];
    OCMExpect([task_delegate_.session() dataTaskWithURL:url])
        .andReturn(session_task);
    task_->SetResponseWriter(base::MakeUnique<net::URLFetcherStringWriter>());
    task_->Start();
    EXPECT_TRUE(session_task.resumed);
    return session_task;
  }

  NSURLSession* session() { return task_delegate_.session(); }

  id<NSURLSessionDataDelegate> session_delegate() {
    return task_delegate_.session_delegate();
  }

  testing::StrictMock<MockDownloadTaskImplDelegate> task_delegate_;
  TestWebState web_state_;
  scoped_refptr<DownloadTaskImpl> task_;  // task_ has to outlive delegate_.
};

// Tests DownloadTaskImpl default state after construction.
TEST_F(DownloadTaskImplTest, DefaultState) {
  EXPECT_EQ(&web_state_, task_->web_state());
  EXPECT_FALSE(task_->GetResponseWriter());
  EXPECT_NSEQ(task_delegate_.configuration().identifier,
              task_->GetIndentifier());
  EXPECT_EQ(kUrl, task_->GetOriginalUrl());
  EXPECT_FALSE(task_->IsDone());
  EXPECT_EQ(0, task_->GetErrorCode());
  EXPECT_EQ(-1, task_->GetTotalBytes());
  EXPECT_EQ(0, task_->GetPercentComplete());
  EXPECT_EQ(kContentDisposition, task_->GetContentDisposition());
  EXPECT_EQ(kMimeType, task_->GetMimeType());
  EXPECT_EQ("file.test", base::UTF16ToUTF8(task_->GetSuggestedFilename()));
}

// Tests sucessfull download of response without content.
// (No URLSession:dataTask:didReceiveData: callback).
TEST_F(DownloadTaskImplTest, EmptyContentDownload) {
  TestNSURLSessionTask* session_task = Start();

  EXPECT_CALL(task_delegate_, OnTaskUpdated(task_.get()));
  session_task.state = NSURLSessionTaskStateCompleted;
  [session_delegate() URLSession:session()
                            task:session_task
            didCompleteWithError:nil];
  testing::Mock::VerifyAndClearExpectations(&task_delegate_);
  EXPECT_TRUE(task_->IsDone());
  EXPECT_EQ(0, task_->GetErrorCode());
  EXPECT_EQ(0, task_->GetTotalBytes());
  EXPECT_EQ(100, task_->GetPercentComplete());
}

// Tests sucessfull download of response with only one
// URLSession:dataTask:didReceiveData: callback.
TEST_F(DownloadTaskImplTest, SmallResponseDownload) {
  TestNSURLSessionTask* session_task = Start();

  EXPECT_CALL(task_delegate_, OnTaskUpdated(task_.get()));
  const char kData[] = "foo";
  int64_t kDataSize = strlen(kData);
  session_task.countOfBytesExpectedToReceive = kDataSize;
  session_task.countOfBytesReceived = kDataSize;
  NSData* data = [NSData dataWithBytes:kData length:kDataSize];
  [session_delegate() URLSession:session()
                        dataTask:session_task
                  didReceiveData:data];
  testing::Mock::VerifyAndClearExpectations(&task_delegate_);
  EXPECT_FALSE(task_->IsDone());
  EXPECT_EQ(0, task_->GetErrorCode());
  EXPECT_EQ(kDataSize, task_->GetTotalBytes());
  EXPECT_EQ(100, task_->GetPercentComplete());
  EXPECT_EQ(kData, task_->GetResponseWriter()->AsStringWriter()->data());

  EXPECT_CALL(task_delegate_, OnTaskUpdated(task_.get()));
  session_task.state = NSURLSessionTaskStateCompleted;
  [session_delegate() URLSession:session()
                            task:session_task
            didCompleteWithError:nil];
  testing::Mock::VerifyAndClearExpectations(&task_delegate_);
  EXPECT_TRUE(task_->IsDone());
  EXPECT_EQ(0, task_->GetErrorCode());
  EXPECT_EQ(kDataSize, task_->GetTotalBytes());
  EXPECT_EQ(100, task_->GetPercentComplete());
  EXPECT_EQ(kData, task_->GetResponseWriter()->AsStringWriter()->data());
}

// Tests sucessfull download of response with multiple
// URLSession:dataTask:didReceiveData: callbacks.
TEST_F(DownloadTaskImplTest, LargeResponseDownload) {
  TestNSURLSessionTask* session_task = Start();

  // The first part of the response has arrived.
  EXPECT_CALL(task_delegate_, OnTaskUpdated(task_.get()));
  const char kData1[] = "foo";
  const char kData2[] = "buzz";
  int64_t kData1Size = strlen(kData1);
  int64_t kData2Size = strlen(kData2);
  session_task.countOfBytesExpectedToReceive = kData1Size + kData2Size;
  session_task.countOfBytesReceived = kData1Size;
  NSData* data1 = [NSData dataWithBytes:kData1 length:kData1Size];
  [session_delegate() URLSession:session()
                        dataTask:session_task
                  didReceiveData:data1];
  testing::Mock::VerifyAndClearExpectations(&task_delegate_);
  EXPECT_FALSE(task_->IsDone());
  EXPECT_EQ(0, task_->GetErrorCode());
  EXPECT_EQ(kData1Size + kData2Size, task_->GetTotalBytes());
  EXPECT_EQ(42, task_->GetPercentComplete());
  net::URLFetcherStringWriter* writer =
      task_->GetResponseWriter()->AsStringWriter();
  EXPECT_EQ(kData1, writer->data());

  // The second part of the response has arrived.
  EXPECT_CALL(task_delegate_, OnTaskUpdated(task_.get()));
  session_task.countOfBytesReceived = kData1Size + kData2Size;
  NSData* data2 = [NSData dataWithBytes:kData2 length:kData2Size];
  [session_delegate() URLSession:session()
                        dataTask:session_task
                  didReceiveData:data2];
  testing::Mock::VerifyAndClearExpectations(&task_delegate_);
  EXPECT_FALSE(task_->IsDone());
  EXPECT_EQ(0, task_->GetErrorCode());
  EXPECT_EQ(kData1Size + kData2Size, task_->GetTotalBytes());
  EXPECT_EQ(100, task_->GetPercentComplete());
  EXPECT_EQ(std::string(kData1) + kData2, writer->data());

  // Download has finished.
  EXPECT_CALL(task_delegate_, OnTaskUpdated(task_.get()));
  session_task.state = NSURLSessionTaskStateCompleted;
  [session_delegate() URLSession:session()
                            task:session_task
            didCompleteWithError:nil];
  testing::Mock::VerifyAndClearExpectations(&task_delegate_);
  EXPECT_TRUE(task_->IsDone());
  EXPECT_EQ(0, task_->GetErrorCode());
  EXPECT_EQ(kData1Size + kData2Size, task_->GetTotalBytes());
  EXPECT_EQ(100, task_->GetPercentComplete());
  EXPECT_EQ(std::string(kData1) + kData2, writer->data());
}

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
  TestNSURLSessionTask* session_task = Start();
  EXPECT_CALL(task_delegate_, OnTaskDestroyed(task_.get()));
  task_ = nullptr;  // Destruct DownloadTaskImpl.
  EXPECT_TRUE(session_task.cancelled);
}

}  // namespace web
