// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cronet_perf_test_base.h"

#include "components/grpc_support/test/quic_test_server.h"
#include "crypto/sha2.h"
#include "net/base/net_errors.h"
#include "net/cert/asn1_util.h"
#include "net/cert/mock_cert_verifier.h"
#include "net/test/cert_test_util.h"
#include "net/test/test_data_directory.h"

#pragma mark

@implementation PerfTestDelegate {
  // Completion semaphore for this PerfTestDelegate. When the request this
  // delegate is attached to finishes (either successfully or with an error),
  // this delegate signals this semaphore.
  // dispatch_semaphore_t _semaphore;
  NSMutableDictionary* _semaphores;
}

@synthesize error = _error;
@synthesize totalBytesReceivedPerTask = _totalBytesReceivedPerTask;
@synthesize expectedContentLengthPerTask = _expectedContentLengthPerTask;

- (id)init {
  if (self = [super init]) {
    //_semaphore = dispatch_semaphore_create(0);
    _semaphores = [NSMutableDictionary dictionaryWithCapacity:0];
  }
  return self;
}

- (void)reset {
  _error = nil;
  _totalBytesReceivedPerTask = [NSMutableDictionary dictionaryWithCapacity:0];
  _expectedContentLengthPerTask =
      [NSMutableDictionary dictionaryWithCapacity:0];
}

- (NSString*)responseBody {
  return nil;
}

- (BOOL)waitForDone:(NSURLSessionDataTask*)task {
  int64_t deadline_ns = 20 * NSEC_PER_SEC;
  // return dispatch_semaphore_wait(
  //           _semaphore, dispatch_time(DISPATCH_TIME_NOW, deadline_ns)) == 0;

  if (!_semaphores[task]) {
    _semaphores[task] = dispatch_semaphore_create(0);
  }
  return dispatch_semaphore_wait(
             _semaphores[task],
             dispatch_time(DISPATCH_TIME_NOW, deadline_ns)) == 0;

  // return dispatch_semaphore_wait(_semaphore, DISPATCH_TIME_FOREVER);
}

- (void)URLSession:(NSURLSession*)session
    didBecomeInvalidWithError:(NSError*)error {
}

- (void)URLSession:(NSURLSession*)session
                    task:(NSURLSessionTask*)task
    didCompleteWithError:(NSError*)error {
  if (error)
    [self setError:error];
  // dispatch_semaphore_signal(_semaphore);

  if (!_semaphores[task]) {
    _semaphores[task] = dispatch_semaphore_create(0);
  }
  dispatch_semaphore_signal(_semaphores[task]);
}

- (void)URLSession:(NSURLSession*)session
                   task:(NSURLSessionTask*)task
    didReceiveChallenge:(NSURLAuthenticationChallenge*)challenge
      completionHandler:
          (void (^)(NSURLSessionAuthChallengeDisposition disp,
                    NSURLCredential* credential))completionHandler {
  completionHandler(NSURLSessionAuthChallengeUseCredential, nil);
}

- (void)URLSession:(NSURLSession*)session
              dataTask:(NSURLSessionDataTask*)dataTask
    didReceiveResponse:(NSURLResponse*)response
     completionHandler:(void (^)(NSURLSessionResponseDisposition disposition))
                           completionHandler {
  _expectedContentLengthPerTask[dataTask] =
      [NSNumber numberWithInt:[response expectedContentLength]];
  completionHandler(NSURLSessionResponseAllow);
}

- (void)URLSession:(NSURLSession*)session
          dataTask:(NSURLSessionDataTask*)dataTask
    didReceiveData:(NSData*)data {
  if (_totalBytesReceivedPerTask[dataTask])
    _totalBytesReceivedPerTask[dataTask] = [NSNumber
        numberWithInt:[_totalBytesReceivedPerTask[dataTask] intValue] +
                      [data length]];
  else
    _totalBytesReceivedPerTask[dataTask] =
        [NSNumber numberWithInt:[data length]];
}

- (void)URLSession:(NSURLSession*)session
             dataTask:(NSURLSessionDataTask*)dataTask
    willCacheResponse:(NSCachedURLResponse*)proposedResponse
    completionHandler:
        (void (^)(NSCachedURLResponse* cachedResponse))completionHandler {
  completionHandler(proposedResponse);
}

@end

namespace cronet {

void CronetPerfTestBase::SetUp() {
  ::testing::Test::SetUp();
  grpc_support::StartQuicTestServer();
  delegate_ = [[PerfTestDelegate alloc] init];
}

void CronetPerfTestBase::TearDown() {
  grpc_support::ShutdownQuicTestServer();
  ::testing::Test::TearDown();
}

// Launches the supplied |task| and blocks until it completes, with a timeout
// of 1 second.
void CronetPerfTestBase::StartDataTaskAndWaitForCompletion(
    NSURLSessionDataTask* task) {
  [delegate_ reset];
  [task resume];
  CHECK([delegate_ waitForDone:task]);
}

::testing::AssertionResult CronetPerfTestBase::IsResponseSuccessful() {
  if ([delegate_ error])
    return ::testing::AssertionFailure() << "error in response: " <<
           [[[delegate_ error] description]
               cStringUsingEncoding:NSUTF8StringEncoding];
  else
    return ::testing::AssertionSuccess() << "no errors in response";
}

std::unique_ptr<net::MockCertVerifier>
CronetPerfTestBase::CreateMockCertVerifier(
    const std::vector<std::string>& certs,
    bool known_root) {
  std::unique_ptr<net::MockCertVerifier> mock_cert_verifier(
      new net::MockCertVerifier());
  for (const auto& cert : certs) {
    net::CertVerifyResult verify_result;
    verify_result.verified_cert =
        net::ImportCertFromFile(net::GetTestCertsDirectory(), cert);

    // By default, HPKP verification is enabled for known trust roots only.
    verify_result.is_issued_by_known_root = known_root;

    // Calculate the public key hash and add it to the verify_result.
    net::HashValue hashValue;
    CHECK(CalculatePublicKeySha256(*verify_result.verified_cert.get(),
                                   &hashValue));
    verify_result.public_key_hashes.push_back(hashValue);

    mock_cert_verifier->AddResultForCert(verify_result.verified_cert.get(),
                                         verify_result, net::OK);
  }
  return mock_cert_verifier;
}

bool CronetPerfTestBase::CalculatePublicKeySha256(
    const net::X509Certificate& cert,
    net::HashValue* out_hash_value) {
  // Convert the cert to DER encoded bytes.
  std::string der_cert_bytes;
  net::X509Certificate::OSCertHandle cert_handle = cert.os_cert_handle();
  if (!net::X509Certificate::GetDEREncoded(cert_handle, &der_cert_bytes)) {
    LOG(INFO) << "Unable to convert the given cert to DER encoding";
    return false;
  }
  // Extract the public key from the cert.
  base::StringPiece spki_bytes;
  if (!net::asn1::ExtractSPKIFromDERCert(der_cert_bytes, &spki_bytes)) {
    LOG(INFO) << "Unable to retrieve the public key from the DER cert";
    return false;
  }
  // Calculate SHA256 hash of public key bytes.
  out_hash_value->tag = net::HASH_VALUE_SHA256;
  crypto::SHA256HashString(spki_bytes, out_hash_value->data(),
                           crypto::kSHA256Length);
  return true;
}

}  // namespace cronet
