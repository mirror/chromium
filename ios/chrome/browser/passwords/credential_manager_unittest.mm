// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/passwords/credential_manager.h"

#include "ios/chrome/browser/passwords/credential_manager_util.h"
#include "ios/chrome/browser/ssl/ios_security_state_tab_helper.h"
#include "ios/web/public/navigation_item.h"
#include "ios/web/public/navigation_manager.h"
#include "ios/web/public/ssl_status.h"
#include "ios/web/public/test/web_test_with_web_state.h"

#import "ios/web/net/crw_ssl_status_updater.h"

#include "base/mac/foundation_util.h"
#include "base/mac/scoped_block.h"
#include "base/strings/sys_string_conversions.h"
#import "ios/web/navigation/crw_session_controller+private_constructors.h"
#import "ios/web/navigation/crw_session_controller.h"
#import "ios/web/navigation/legacy_navigation_manager_impl.h"
#import "ios/web/navigation/navigation_manager_impl.h"
#import "ios/web/public/navigation_item.h"
#include "ios/web/public/ssl_status.h"
#include "ios/web/public/test/web_test.h"
#import "ios/web/test/fakes/test_navigation_manager_delegate.h"
#import "ios/web/web_state/wk_web_view_security_util.h"
#include "net/cert/x509_util_ios_and_mac.h"
#include "net/test/cert_test_util.h"
#include "net/test/test_data_directory.h"
#include "third_party/ocmock/OCMock/OCMock.h"
#include "third_party/ocmock/gtest_support.h"

// Mocks CRWSSLStatusUpdaterTestDataSource.
@interface CRWSSLStatusUpdaterTestDataSource
    : NSObject<CRWSSLStatusUpdaterDataSource> {
  StatusQueryHandler _verificationCompletionHandler;
}

// Yes if |SSLStatusUpdater:querySSLStatusForTrust:host:completionHandler| was
// called.
@property(nonatomic, readonly) BOOL certVerificationRequested;

// Calls completion handler passed in
// |SSLStatusUpdater:querySSLStatusForTrust:host:completionHandler|.
- (void)finishVerificationWithCertStatus:(net::CertStatus)certStatus
                           securityStyle:(web::SecurityStyle)securityStyle;

@end

@implementation CRWSSLStatusUpdaterTestDataSource

- (BOOL)certVerificationRequested {
  return _verificationCompletionHandler ? YES : NO;
}

- (void)finishVerificationWithCertStatus:(net::CertStatus)certStatus
                           securityStyle:(web::SecurityStyle)securityStyle {
  if (_verificationCompletionHandler) {
    _verificationCompletionHandler(securityStyle, certStatus);
  }
}

#pragma mark CRWSSLStatusUpdaterDataSource

- (void)SSLStatusUpdater:(CRWSSLStatusUpdater*)SSLStatusUpdater
    querySSLStatusForTrust:(base::ScopedCFTypeRef<SecTrustRef>)trust
                      host:(NSString*)host
         completionHandler:(StatusQueryHandler)completionHandler {
  _verificationCompletionHandler = [completionHandler copy];
}

@end

namespace credential_manager {

namespace {

// Test hostname for cert verification.
NSString* const kHostName = @"www.example.com";
constexpr char kTestWebOrigin[] = "https://www.example.com/";

}  // namespace

class CredentialManagerTest : public web::WebTestWithWebState {
 public:
  void SetUp() override {
    WebTestWithWebState::SetUp();
    IOSSecurityStateTabHelper::CreateForWebState(web_state());
  }

  void LoadUrl(GURL url) { LoadHtml(@"<html></html>", url); }

  void UpdateSSL() {
    scoped_refptr<net::X509Certificate> cert =
        net::ImportCertFromFile(net::GetTestCertsDirectory(), "ok_cert.pem");
    base::ScopedCFTypeRef<CFMutableArrayRef> chain(
        net::x509_util::CreateSecCertificateArrayForX509Certificate(
            cert.get()));
    trust_ = web::CreateServerTrustFromChain(base::mac::CFToNSCast(chain.get()),
                                             kHostName);
    data_source_ = [[CRWSSLStatusUpdaterTestDataSource alloc] init];
    delegate_ =
        [OCMockObject mockForProtocol:@protocol(CRWSSLStatusUpdaterDelegate)];
    ssl_status_updater_ = [[CRWSSLStatusUpdater alloc]
        initWithDataSource:data_source_
         navigationManager:web_state()->GetNavigationManager()];
    [ssl_status_updater_ setDelegate:delegate_];
    [[delegate_ expect] SSLStatusUpdater:ssl_status_updater_
        didChangeSSLStatusForNavigationItem:web_state()
                                                ->GetNavigationManager()
                                                ->GetVisibleItem()];
    [[delegate_ expect] SSLStatusUpdater:ssl_status_updater_
        didChangeSSLStatusForNavigationItem:web_state()
                                                ->GetNavigationManager()
                                                ->GetVisibleItem()];

    [ssl_status_updater_
        updateSSLStatusForNavigationItem:web_state()
                                             ->GetNavigationManager()
                                             ->GetVisibleItem()
                            withCertHost:kHostName
                                   trust:trust_
                    hasOnlySecureContent:YES];
    [data_source_
        finishVerificationWithCertStatus:0
                           securityStyle:web::SECURITY_STYLE_AUTHENTICATED];
  }

 private:
  CRWSSLStatusUpdaterTestDataSource* data_source_;
  id delegate_;
  std::unique_ptr<web::NavigationManagerImpl> nav_manager_;
  CRWSSLStatusUpdater* ssl_status_updater_;
  base::ScopedCFTypeRef<SecTrustRef> trust_;
};

// Test that IsContextSecure returns true for HTTPS.
TEST_F(CredentialManagerTest, AcceptHttpsUrls) {
  LoadUrl(GURL(kTestWebOrigin));
  UpdateSSL();
  EXPECT_TRUE(IsContextSecure(web_state()));
}

// Test that IsContextSecure returns false for HTTP.
TEST_F(CredentialManagerTest, HttpIsNotSecureContext) {
  LoadUrl(GURL("http://example.com"));
  UpdateSSL();
  EXPECT_FALSE(IsContextSecure(web_state()));
}

// Test that IsContextSecure returns false for HTTPS with insecure contents.
TEST_F(CredentialManagerTest, InsecureContent) {
  LoadUrl(GURL(kTestWebOrigin));
  UpdateSSL();
  web_state()
      ->GetNavigationManager()
      ->GetVisibleItem()
      ->GetSSL()
      .content_status =
      web::SSLStatus::ContentStatusFlags::DISPLAYED_INSECURE_CONTENT;
  EXPECT_FALSE(IsContextSecure(web_state()));
}
};
