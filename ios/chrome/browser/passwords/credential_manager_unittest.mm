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
// a secure or insecure context.
@interface CRWSSLStatusUpdaterTestDataSource
    : NSObject<CRWSSLStatusUpdaterDataSource> {
  StatusQueryHandler _verificationCompletionHandler;
}

// Calls completion handler passed in
// |SSLStatusUpdater:querySSLStatusForTrust:host:completionHandler|.
- (void)finishVerificationWithCertStatus:(net::CertStatus)certStatus
                           securityStyle:(web::SecurityStyle)securityStyle;

@end

@implementation CRWSSLStatusUpdaterTestDataSource

- (void)finishVerificationWithCertStatus:(net::CertStatus)certStatus
                           securityStyle:(web::SecurityStyle)securityStyle {
  if (_verificationCompletionHandler) {
    _verificationCompletionHandler(securityStyle, certStatus);
  }
}

#pragma mark CRWSSLStatusUpdaterDataSource

// Called from CRWSSLStatusUpdater. Saves the received callback to be run
// manually from the test case.
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

// SSL certificate to load for testing.
constexpr char kCertFileName[] = "ok_cert.pem";

}  // namespace

// TODO(crbug.com/435048): once JSCredentialManager is implemented and used
// from CredentialManager methods, mock it and write unit tests for
// CredentialManager methods SendGetResponse, SendStoreResponse and
// SendPreventSilentAccessResponse.
class CredentialManagerTest : public web::WebTestWithWebState {
 public:
  void SetUp() override {
    WebTestWithWebState::SetUp();

    // Used by IsContextSecure function.
    IOSSecurityStateTabHelper::CreateForWebState(web_state());
  }

  void LoadUrl(GURL url) { LoadHtml(@"<html></html>", url); }

  // Uses CRWSSLStatusUpdater to update SSLStatus on web_state()'s visible
  // NavigationItem. CRWSSLStatusUpdater creates a callback which is then called
  // by mock CRWSSLStatusUpdaterTestDataSource. It is expected that certificate
  // should be verfied in the meantime, but here we manually pass CertStatus
  // and SecurityStyle calculated for the certificate. Therefore actual
  // certificate content does not matter and the same is used for all test
  // cases.
  // |has_only_secure_content| indicates whether web_state()'s content contains
  // mixed content.
  void UpdateSSLStatusWithCert(net::CertStatus cert_status,
                               web::SecurityStyle security_style,
                               bool has_only_secure_content = true) {
    // Loads certificate from file.
    scoped_refptr<net::X509Certificate> cert =
        net::ImportCertFromFile(net::GetTestCertsDirectory(), kCertFileName);
    base::ScopedCFTypeRef<CFMutableArrayRef> chain(
        net::x509_util::CreateSecCertificateArrayForX509Certificate(
            cert.get()));
    base::ScopedCFTypeRef<SecTrustRef> trust = web::CreateServerTrustFromChain(
        base::mac::CFToNSCast(chain.get()), kHostName);
    CRWSSLStatusUpdaterTestDataSource* data_source =
        [[CRWSSLStatusUpdaterTestDataSource alloc] init];
    id delegate =
        [OCMockObject mockForProtocol:@protocol(CRWSSLStatusUpdaterDelegate)];
    CRWSSLStatusUpdater* ssl_status_updater = [[CRWSSLStatusUpdater alloc]
        initWithDataSource:data_source
         navigationManager:web_state()->GetNavigationManager()];
    [ssl_status_updater setDelegate:delegate];

    // Make sure that item change callback was called twice for changing
    // content_status and security style.
    [[delegate expect] SSLStatusUpdater:ssl_status_updater
        didChangeSSLStatusForNavigationItem:web_state()
                                                ->GetNavigationManager()
                                                ->GetVisibleItem()];
    [[delegate expect] SSLStatusUpdater:ssl_status_updater
        didChangeSSLStatusForNavigationItem:web_state()
                                                ->GetNavigationManager()
                                                ->GetVisibleItem()];

    // Call SSLStatusUpdater, which produces a callback and passes it to
    // data_source.
    [ssl_status_updater
        updateSSLStatusForNavigationItem:web_state()
                                             ->GetNavigationManager()
                                             ->GetVisibleItem()
                            withCertHost:kHostName
                                   trust:trust
                    hasOnlySecureContent:has_only_secure_content];
    // Instead of verifying the certificate, pass |cert_status| and
    // |security_style| and run the callback.
    [data_source finishVerificationWithCertStatus:cert_status
                                    securityStyle:security_style];
  }
};

// Test that IsContextSecure returns true for HTTPS origin with valid SSL
// certificate.
TEST_F(CredentialManagerTest, AcceptHttpsUrls) {
  LoadUrl(GURL(kHttpsWebOrigin));
  UpdateSSLStatusWithCert(net::CERT_STATUS_IS_EV,
                          web::SECURITY_STYLE_AUTHENTICATED);
  EXPECT_TRUE(IsContextSecure(web_state()));
}

// Test that IsContextSecure returns false for HTTP origin.
TEST_F(CredentialManagerTest, HttpIsNotSecureContext) {
  LoadUrl(GURL(kHttpWebOrigin));
  UpdateSSLStatusWithCert(net::CERT_STATUS_IS_EV,
                          web::SECURITY_STYLE_AUTHENTICATED);
  EXPECT_FALSE(IsContextSecure(web_state()));
}

// Test that IsContextSecure returns false for HTTPS origin with valid SSL
// certificate but mixed contents.
TEST_F(CredentialManagerTest, InsecureContent) {
  LoadUrl(GURL(kHttpsWebOrigin));
  UpdateSSLStatusWithCert(net::CERT_STATUS_IS_EV,
                          web::SECURITY_STYLE_AUTHENTICATED, false);
  EXPECT_FALSE(IsContextSecure(web_state()));
}

// Test that IsContextSecure returns false for HTTPS origin with invalid SSL
// certificate.
TEST_F(CredentialManagerTest, InvalidSSLCertificate) {
  LoadUrl(GURL(kHttpsWebOrigin));
  UpdateSSLStatusWithCert(net::CERT_STATUS_INVALID,
                          web::SECURITY_STYLE_UNAUTHENTICATED);
  EXPECT_FALSE(IsContextSecure(web_state()));
}

}  // namespace credential_manager
