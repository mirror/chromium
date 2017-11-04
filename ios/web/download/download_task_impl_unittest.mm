// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/web/download/download_task_impl.h"

#import <Foundation/Foundation.h>
#import <WebKit/WebKit.h>

#include "base/compiler_specific.h"
#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/memory/ptr_util.h"
#include "base/run_loop.h"
#include "base/strings/utf_string_conversions.h"
#import "ios/testing/wait_util.h"
#import "ios/web/net/cookies/wk_cookie_util.h"
#import "ios/web/public/test/fakes/test_web_state.h"
#include "ios/web/public/test/web_test.h"
#include "net/base/net_errors.h"
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

void NoOp(int) {}

using testing::kWaitForPageLoadTimeout;
using testing::WaitUntilConditionOrTimeout;

@interface TestNSURLSessionTask : NSURLSessionDataTask
- (instancetype)initWithURL:(NSURL*)URL;
- (instancetype)init NS_UNAVAILABLE;
@property(nonatomic) NSURLSessionTaskState state;
@property(nonatomic) int64_t countOfBytesReceived;
@property(nonatomic) int64_t countOfBytesExpectedToReceive;

@property(nullable, readonly, copy) NSURLRequest* currentRequest;
@property(nullable, readonly, copy) NSURLRequest* originalRequest;
@property(nonatomic) BOOL cancelled;
@property(nonatomic) BOOL resumed;

- (void)cancel;
- (void)resume;
@end

@implementation TestNSURLSessionTask
@synthesize state = _state;
@synthesize countOfBytesReceived = _countOfBytesReceived;
@synthesize countOfBytesExpectedToReceive = _countOfBytesExpectedToReceive;
@synthesize currentRequest = _currentRequest;
@synthesize originalRequest = _originalRequest;
@synthesize cancelled = _cancelled;
@synthesize resumed = _resumed;

- (instancetype)initWithURL:(NSURL*)URL {
  self = [super init];
  if (self) {
    _currentRequest = [NSURLRequest requestWithURL:URL];
    _originalRequest = [NSURLRequest requestWithURL:URL];
  }
  return self;
}

- (void)cancel {
  self.cancelled = YES;
}
- (void)resume {
  self.resumed = YES;
}

- (NSString*)_storagePartitionIdentifier {
  return nil;
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
                              id<NSURLSessionDataDelegate> delegate,
                              NSOperationQueue* delegate_quueue) {
    EXPECT_EQ(NSOperationQueue.mainQueue, delegate_quueue);
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
    web_state_.SetBrowserState(GetBrowserState());
  }

  // Starts download and return NSURLSessionDataTask mock for this task.
  TestNSURLSessionTask* Start(
      std::unique_ptr<net::URLFetcherResponseWriter> writer) {
    NSURL* url = [NSURL URLWithString:@(kUrl)];
    TestNSURLSessionTask* session_task =
        [[TestNSURLSessionTask alloc] initWithURL:url];
    OCMExpect([task_delegate_.session() dataTaskWithURL:url])
        .andReturn(session_task);
    task_->SetResponseWriter(std::move(writer));
    task_->Start();
    bool success = WaitUntilConditionOrTimeout(kWaitForPageLoadTimeout, ^{
      base::RunLoop().RunUntilIdle();
      return session_task.resumed;
    });
    return success ? session_task : nil;
  }

  // Starts download and return NSURLSessionDataTask mock for this task.
  // Uses URLFetcherStringWriter as response writer.
  TestNSURLSessionTask* Start() {
    return Start(base::MakeUnique<net::URLFetcherStringWriter>());
  }

  // Sets cookie for the test browser state.
  bool SetCookie(NSHTTPCookie* cookie) WARN_UNUSED_RESULT
      API_AVAILABLE(ios(11.0)) {
    WKHTTPCookieStore* store =
        web::WKCookieStoreForBrowserState(GetBrowserState());
    __block bool cookie_was_set = false;
    [store setCookie:cookie
        completionHandler:^{
          cookie_was_set = true;
        }];
    return WaitUntilConditionOrTimeout(kWaitForPageLoadTimeout, ^{
      return cookie_was_set;
    });
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
  EXPECT_EQ(-1, task_->GetPercentComplete());
  EXPECT_EQ(kContentDisposition, task_->GetContentDisposition());
  EXPECT_EQ(kMimeType, task_->GetMimeType());
  EXPECT_EQ("file.test", base::UTF16ToUTF8(task_->GetSuggestedFilename()));

  EXPECT_CALL(task_delegate_, OnTaskDestroyed(task_.get()));
}

// Tests sucessfull download of response without content.
// (No URLSession:dataTask:didReceiveData: callback).
TEST_F(DownloadTaskImplTest, EmptyContentDownload) {
  EXPECT_CALL(task_delegate_, OnTaskUpdated(task_.get()));
  TestNSURLSessionTask* session_task = Start();
  ASSERT_TRUE(session_task);
  testing::Mock::VerifyAndClearExpectations(&task_delegate_);

  EXPECT_CALL(task_delegate_, OnTaskUpdated(task_.get()));
  session_task.state = NSURLSessionTaskStateCompleted;
  [session_delegate() URLSession:session()
                            task:session_task
            didCompleteWithError:nil];
  testing::Mock::VerifyAndClearExpectations(&task_delegate_);
  ASSERT_TRUE(WaitUntilConditionOrTimeout(kWaitForPageLoadTimeout, ^{
    return task_->IsDone();
  }));
  EXPECT_EQ(0, task_->GetErrorCode());
  EXPECT_EQ(0, task_->GetTotalBytes());
  EXPECT_EQ(100, task_->GetPercentComplete());

  EXPECT_CALL(task_delegate_, OnTaskDestroyed(task_.get()));
}

// Tests sucessfull download of response with only one
// URLSession:dataTask:didReceiveData: callback.
TEST_F(DownloadTaskImplTest, SmallResponseDownload) {
  EXPECT_CALL(task_delegate_, OnTaskUpdated(task_.get()));
  TestNSURLSessionTask* session_task = Start();
  ASSERT_TRUE(session_task);
  testing::Mock::VerifyAndClearExpectations(&task_delegate_);

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
  ASSERT_TRUE(WaitUntilConditionOrTimeout(kWaitForPageLoadTimeout, ^{
    return task_->IsDone();
  }));
  EXPECT_EQ(0, task_->GetErrorCode());
  EXPECT_EQ(kDataSize, task_->GetTotalBytes());
  EXPECT_EQ(100, task_->GetPercentComplete());
  EXPECT_EQ(kData, task_->GetResponseWriter()->AsStringWriter()->data());

  EXPECT_CALL(task_delegate_, OnTaskDestroyed(task_.get()));
}

// Tests sucessfull download of response with multiple
// URLSession:dataTask:didReceiveData: callbacks.
TEST_F(DownloadTaskImplTest, LargeResponseDownload) {
  EXPECT_CALL(task_delegate_, OnTaskUpdated(task_.get()));
  TestNSURLSessionTask* session_task = Start();
  ASSERT_TRUE(session_task);
  testing::Mock::VerifyAndClearExpectations(&task_delegate_);

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
  ASSERT_TRUE(WaitUntilConditionOrTimeout(kWaitForPageLoadTimeout, ^{
    return task_->IsDone();
  }));
  EXPECT_EQ(0, task_->GetErrorCode());
  EXPECT_EQ(kData1Size + kData2Size, task_->GetTotalBytes());
  EXPECT_EQ(100, task_->GetPercentComplete());
  EXPECT_EQ(std::string(kData1) + kData2, writer->data());

  EXPECT_CALL(task_delegate_, OnTaskDestroyed(task_.get()));
}

// Tests failed download when URLSession:dataTask:didReceiveData: callback was
// not even called.
TEST_F(DownloadTaskImplTest, FailureInTheBeginning) {
  EXPECT_CALL(task_delegate_, OnTaskUpdated(task_.get()));
  TestNSURLSessionTask* session_task = Start();
  ASSERT_TRUE(session_task);
  testing::Mock::VerifyAndClearExpectations(&task_delegate_);

  EXPECT_CALL(task_delegate_, OnTaskUpdated(task_.get()));
  NSError* error = [NSError errorWithDomain:NSURLErrorDomain
                                       code:NSURLErrorNotConnectedToInternet
                                   userInfo:nil];
  session_task.state = NSURLSessionTaskStateCompleted;
  [session_delegate() URLSession:session()
                            task:session_task
            didCompleteWithError:error];
  ASSERT_TRUE(WaitUntilConditionOrTimeout(kWaitForPageLoadTimeout, ^{
    return task_->IsDone();
  }));
  EXPECT_TRUE(task_->GetErrorCode() == net::ERR_INTERNET_DISCONNECTED);
  EXPECT_EQ(0, task_->GetTotalBytes());
  EXPECT_EQ(100, task_->GetPercentComplete());

  EXPECT_CALL(task_delegate_, OnTaskDestroyed(task_.get()));
}

// Tests failed download when URLSession:dataTask:didReceiveData: callback was
// called once.
TEST_F(DownloadTaskImplTest, FailureInTheMiddle) {
  EXPECT_CALL(task_delegate_, OnTaskUpdated(task_.get()));
  TestNSURLSessionTask* session_task = Start();
  ASSERT_TRUE(session_task);
  testing::Mock::VerifyAndClearExpectations(&task_delegate_);

  // A part of the response has arrived.
  EXPECT_CALL(task_delegate_, OnTaskUpdated(task_.get()));
  const char kReceivedData[] = "foo";
  int64_t kReceivedDataSize = strlen(kReceivedData);
  int64_t kExpectedDataSize = kReceivedDataSize + 10;
  session_task.countOfBytesExpectedToReceive = kExpectedDataSize;
  session_task.countOfBytesReceived = kReceivedDataSize;
  NSData* data = [NSData dataWithBytes:kReceivedData length:kReceivedDataSize];
  [session_delegate() URLSession:session()
                        dataTask:session_task
                  didReceiveData:data];
  testing::Mock::VerifyAndClearExpectations(&task_delegate_);
  EXPECT_FALSE(task_->IsDone());
  EXPECT_EQ(0, task_->GetErrorCode());
  EXPECT_EQ(kExpectedDataSize, task_->GetTotalBytes());
  EXPECT_EQ(23, task_->GetPercentComplete());
  net::URLFetcherStringWriter* writer =
      task_->GetResponseWriter()->AsStringWriter();
  EXPECT_EQ(kReceivedData, writer->data());

  // Download has failed.
  EXPECT_CALL(task_delegate_, OnTaskUpdated(task_.get()));
  NSError* error = [NSError errorWithDomain:NSURLErrorDomain
                                       code:NSURLErrorNotConnectedToInternet
                                   userInfo:nil];
  session_task.state = NSURLSessionTaskStateCompleted;
  [session_delegate() URLSession:session()
                            task:session_task
            didCompleteWithError:error];
  ASSERT_TRUE(WaitUntilConditionOrTimeout(kWaitForPageLoadTimeout, ^{
    return task_->IsDone();
  }));
  EXPECT_TRUE(task_->GetErrorCode() == net::ERR_INTERNET_DISCONNECTED);
  EXPECT_EQ(kExpectedDataSize, task_->GetTotalBytes());
  EXPECT_EQ(23, task_->GetPercentComplete());
  EXPECT_EQ(kReceivedData, writer->data());

  EXPECT_CALL(task_delegate_, OnTaskDestroyed(task_.get()));
}

// Tests that NSURLSessionConfiguration contains up to date cookie from browser
// state before the download started.
TEST_F(DownloadTaskImplTest, Cookie) {
  if (@available(iOS 11, *)) {
    // Remove all cookies from the session configuration.
    NSHTTPCookieStorage* storage =
        task_delegate_.configuration().HTTPCookieStorage;
    for (NSHTTPCookie* cookie in storage.cookies)
      [storage deleteCookie:cookie];

    // Add a cookie to BrowserState.
    NSURL* cookie_url = [NSURL URLWithString:@(kUrl)];
    NSHTTPCookie* cookie = [NSHTTPCookie cookieWithProperties:@{
      NSHTTPCookieName : @"name",
      NSHTTPCookieValue : @"value",
      NSHTTPCookiePath : cookie_url.path,
      NSHTTPCookieDomain : cookie_url.host,
      NSHTTPCookieVersion : @1,
    }];
    ASSERT_TRUE(SetCookie(cookie));

    // Start the download and make sure that all cookie from BrowserState were
    // picked up.
    EXPECT_CALL(task_delegate_, OnTaskUpdated(task_.get()));
    ASSERT_TRUE(Start());
    EXPECT_EQ(1U, storage.cookies.count);
    EXPECT_NSEQ(cookie, storage.cookies.firstObject);
  }

  EXPECT_CALL(task_delegate_, OnTaskDestroyed(task_.get()));
}

// Tests that URLFetcherFileWriter deletes the file of download has failed with
// error.
TEST_F(DownloadTaskImplTest, FileDeletion) {
  // Create URLFetcherFileWriter.
  base::ScopedTempDir temp_dir;
  ASSERT_TRUE(temp_dir.CreateUniqueTempDir());
  base::FilePath temp_file = temp_dir.GetPath().AppendASCII("DownloadTaskImpl");
  base::DeleteFile(temp_file, false);
  ASSERT_FALSE(base::PathExists(temp_file));
  std::unique_ptr<net::URLFetcherResponseWriter> writer =
      base::MakeUnique<net::URLFetcherFileWriter>(
          base::ThreadTaskRunnerHandle::Get(), temp_file);
  writer->Initialize(base::Bind(&NoOp));

  // Start the download.
  EXPECT_CALL(task_delegate_, OnTaskUpdated(task_.get()));
  TestNSURLSessionTask* session_task = Start(std::move(writer));
  ASSERT_TRUE(session_task);

  // Deliver the response and verify that download file exists.
  EXPECT_CALL(task_delegate_, OnTaskUpdated(task_.get()));
  const char kReceivedData[] = "foo";
  NSData* data =
      [NSData dataWithBytes:kReceivedData length:strlen(kReceivedData)];
  [session_delegate() URLSession:session()
                        dataTask:session_task
                  didReceiveData:data];
  ASSERT_TRUE(base::PathExists(temp_file));

  // Fail the download and verify that file was deleted.
  EXPECT_CALL(task_delegate_, OnTaskUpdated(task_.get()));
  NSError* error = [NSError errorWithDomain:NSURLErrorDomain
                                       code:NSURLErrorNotConnectedToInternet
                                   userInfo:nil];
  session_task.state = NSURLSessionTaskStateCompleted;
  [session_delegate() URLSession:session()
                            task:session_task
            didCompleteWithError:error];
  ASSERT_TRUE(WaitUntilConditionOrTimeout(kWaitForPageLoadTimeout, ^{
    return task_->IsDone();
  }));
  ASSERT_TRUE(WaitUntilConditionOrTimeout(kWaitForPageLoadTimeout, ^{
    base::RunLoop().RunUntilIdle();
    return !base::PathExists(temp_file);
  }));

  EXPECT_CALL(task_delegate_, OnTaskDestroyed(task_.get()));
}

// Tests that destructing DownloadTaskImpl calls -[NSURLSessionDataTask cancel]
// and OnTaskDestroyed().
TEST_F(DownloadTaskImplTest, DownloadTaskDestruction) {
  EXPECT_CALL(task_delegate_, OnTaskUpdated(task_.get()));
  TestNSURLSessionTask* session_task = Start();
  ASSERT_TRUE(session_task);
  testing::Mock::VerifyAndClearExpectations(&task_delegate_);
  EXPECT_CALL(task_delegate_, OnTaskDestroyed(task_.get()));
  task_ = nullptr;  // Destruct DownloadTaskImpl.
  EXPECT_TRUE(session_task.cancelled);
}

// Tests that shutting down DownloadTaskImpl calls
// -[NSURLSessionDataTask cancel], but does not call OnTaskDestroyed().
TEST_F(DownloadTaskImplTest, DownloadTaskShutdown) {
  EXPECT_CALL(task_delegate_, OnTaskUpdated(task_.get()));
  TestNSURLSessionTask* session_task = Start();
  ASSERT_TRUE(session_task);
  testing::Mock::VerifyAndClearExpectations(&task_delegate_);

  task_->ShutDown();
  EXPECT_TRUE(session_task.cancelled);
}

}  // namespace web
