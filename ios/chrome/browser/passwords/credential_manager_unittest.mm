// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/passwords/credential_manager.h"

#include "base/mac/foundation_util.h"
#include "ios/chrome/browser/passwords/credential_manager_util.h"
#include "ios/chrome/browser/ssl/ios_security_state_tab_helper.h"
#import "ios/web/net/crw_ssl_status_updater.h"
#include "ios/web/public/navigation_item.h"
#include "ios/web/public/navigation_manager.h"
#include "ios/web/public/ssl_status.h"
#include "ios/web/public/test/web_test_with_web_state.h"
#import "ios/web/web_state/wk_web_view_security_util.h"
#include "net/cert/x509_util_ios_and_mac.h"
#include "net/test/cert_test_util.h"
#include "net/test/test_data_directory.h"
#include "third_party/ocmock/OCMock/OCMock.h"

// Used with CRWSSLStatusUpdater to update SSLStatus of web_state() to represent
// a secure context.
@interface CRWSSLStatusUpdaterTestDataSource
    : NSObject<CRWSSLStatusUpdaterDataSource> {
  StatusQueryHandler _verificationCompletionHandler;
}

// Yes if |SSLStatusUpdater:querySSLStatusForTrust:host:completionHandler| was
// called.
//@property(nonatomic, readonly) BOOL certVerificationRequested;

// Calls completion handler passed in
// |SSLStatusUpdater:querySSLStatusForTrust:host:completionHandler|.
- (void)finishVerificationWithCertStatus:(net::CertStatus)certStatus
                           securityStyle:(web::SecurityStyle)securityStyle;

@end

@implementation CRWSSLStatusUpdaterTestDataSource

//- (BOOL)certVerificationRequested {
//  return _verificationCompletionHandler ? YES : NO;
//}

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
// HTTPS origin corresponding to kHostName.
constexpr char kHttpsWebOrigin[] = "https://www.example.com/";
// HTTP origin corresponding to kHostName.
constexpr char kHttpWebOrigin[] = "http://www.example.com/";

// OK certificate for secure context.
constexpr char kOkCertFile[] = "ok_cert.pem";

// Bad certificate for testing if insecure context is detected correctly.
constexpr char kExpiredCertFile[] = "expired_cert.pem";

}  // namespace

class CredentialManagerTest : public web::WebTestWithWebState {
 public:
  void SetUp() override {
    WebTestWithWebState::SetUp();
    IOSSecurityStateTabHelper::CreateForWebState(web_state());
  }

  void LoadUrl(GURL url) { LoadHtml(@"<html></html>", url); }

  // Uses CRWSSLStatusUpdater to update SSLStatus on web_state()'s visible
  // NavigationItem. |cert_name| is a filename of certificate to load from test
  // certs directory, |cert_status| is a status of the certificate and
  // |security_style| is SecurityStyle to be applied to SSLStatus on
  // web_state(). |has_only_secure_content| indicates whether
  // DISPLAYED_INSECURE_CONTENT flag on SSLStatus.content_status should be set.
  void UpdateSSLStatusWithCert(std::string cert_name, net::CertStatus cert_status, web::SecurityStyle security_style, bool has_only_secure_content=true) {
    // Loads certificate from file.
    scoped_refptr<net::X509Certificate> cert =
        net::ImportCertFromFile(net::GetTestCertsDirectory(), cert_name);
    base::ScopedCFTypeRef<CFMutableArrayRef> chain(
        net::x509_util::CreateSecCertificateArrayForX509Certificate(
            cert.get()));
    base::ScopedCFTypeRef<SecTrustRef> trust_ = web::CreateServerTrustFromChain(base::mac::CFToNSCast(chain.get()),
                                             kHostName);
    CRWSSLStatusUpdaterTestDataSource* data_source_ = [[CRWSSLStatusUpdaterTestDataSource alloc] init];
    id delegate_ =
        [OCMockObject mockForProtocol:@protocol(CRWSSLStatusUpdaterDelegate)];
    CRWSSLStatusUpdater* ssl_status_updater_ = [[CRWSSLStatusUpdater alloc]
        initWithDataSource:data_source_
         navigationManager:web_state()->GetNavigationManager()];
    [ssl_status_updater_ setDelegate:delegate_];

    // Make sure that item change callback was called twice for changing
    // content_status and security style.
    [[delegate_ expect] SSLStatusUpdater:ssl_status_updater_
        didChangeSSLStatusForNavigationItem:web_state()
                                                ->GetNavigationManager()
                                                ->GetVisibleItem()];
    [[delegate_ expect] SSLStatusUpdater:ssl_status_updater_
        didChangeSSLStatusForNavigationItem:web_state()
                                                ->GetNavigationManager()
                                                ->GetVisibleItem()];

    // 
    [ssl_status_updater_
        updateSSLStatusForNavigationItem:web_state()
                                             ->GetNavigationManager()
                                             ->GetVisibleItem()
                            withCertHost:kHostName
                                   trust:trust_
                    hasOnlySecureContent:has_only_secure_content];
    [data_source_
        finishVerificationWithCertStatus:cert_status
                           securityStyle:security_style];
  }

};

// Test that IsContextSecure returns true for HTTPS origin with valid SSL
// certificate.
TEST_F(CredentialManagerTest, AcceptHttpsUrls) {
  LoadUrl(GURL(kHttpsWebOrigin));
  UpdateSSLStatusWithCert(kOkCertFile, 0, web::SECURITY_STYLE_AUTHENTICATED);
  EXPECT_TRUE(IsContextSecure(web_state()));
}

// Test that IsContextSecure returns false for HTTP origin.
TEST_F(CredentialManagerTest, HttpIsNotSecureContext) {
  LoadUrl(GURL(kHttpWebOrigin));
  UpdateSSLStatusWithCert(kOkCertFile, 0, web::SECURITY_STYLE_AUTHENTICATED);
  EXPECT_FALSE(IsContextSecure(web_state()));
}

// Test that IsContextSecure returns false for HTTPS origin with valid SSL
// certificate but mixed contents.
TEST_F(CredentialManagerTest, InsecureContent) {
  LoadUrl(GURL(kHttpsWebOrigin));
  UpdateSSLStatusWithCert(kOkCertFile, 0, web::SECURITY_STYLE_AUTHENTICATED, false);
  EXPECT_FALSE(IsContextSecure(web_state()));
}

// Test that IsContextSecure returns false for HTTPS origin with invalid SSL
// certificate.
TEST_F(CredentialManagerTest, InvalidSSLCertificate) {
  // Check against expired certificate file
  LoadUrl(GURL(kHttpsWebOrigin));
  UpdateSSLStatusWithCert(kExpiredCertFile, net::CERT_STATUS_INVALID, web::SECURITY_STYLE_UNAUTHENTICATED);
  EXPECT_FALSE(IsContextSecure(web_state()));
}

}  // namespace credential_manager
