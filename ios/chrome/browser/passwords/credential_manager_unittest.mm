// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/passwords/credential_manager.h"

#include "base/mac/foundation_util.h"
#include "ios/chrome/browser/passwords/credential_manager_util.h"
#include "ios/chrome/browser/ssl/ios_security_state_tab_helper.h"
#include "ios/web/public/navigation_item.h"
#include "ios/web/public/navigation_manager.h"
#include "ios/web/public/ssl_status.h"
#include "ios/web/public/test/web_test_with_web_state.h"
#include "net/ssl/ssl_connection_status_flags.h"
#include "net/test/cert_test_util.h"
#include "net/test/test_data_directory.h"

#include "ios/web/public/origin_util.h"

namespace credential_manager {

namespace {

// Test hostname for cert verification.
constexpr char kHostName[] = "www.example.com";
// HTTPS origin corresponding to kHostName.
constexpr char kHttpsWebOrigin[] = "https://www.example.com/";
// HTTP origin corresponding to kHostName.
constexpr char kHttpWebOrigin[] = "http://www.example.com/";
// HTTP origin representing localhost. It should be considered secure.
constexpr char kLocalhostOrigin[] = "http://localhost";
// Origin with data scheme. It should be considered insecure.
constexpr char kDataUriSchemeOrigin[] = "data://www.example.com";

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

  void UpdateSslStatus(net::CertStatus cert_status,
                       web::SecurityStyle security_style,
                       web::SSLStatus::ContentStatusFlags content_status) {
    scoped_refptr<net::X509Certificate> cert =
        net::ImportCertFromFile(net::GetTestCertsDirectory(), kCertFileName);
    web_state()
        ->GetNavigationManager()
        ->GetVisibleItem()
        ->GetSSL()
        .security_style = security_style;
    web_state()
        ->GetNavigationManager()
        ->GetVisibleItem()
        ->GetSSL()
        .certificate = cert;
    web_state()
        ->GetNavigationManager()
        ->GetVisibleItem()
        ->GetSSL()
        .cert_status = cert_status;
    web_state()
        ->GetNavigationManager()
        ->GetVisibleItem()
        ->GetSSL()
        .connection_status = net::SSL_CONNECTION_VERSION_SSL3;
    web_state()
        ->GetNavigationManager()
        ->GetVisibleItem()
        ->GetSSL()
        .content_status = content_status;
    web_state()
        ->GetNavigationManager()
        ->GetVisibleItem()
        ->GetSSL()
        .cert_status_host = kHostName;
  }
};

// Test that HTTPS websites with valid SSL certificate are recognized as
// secure.
TEST_F(CredentialManagerTest, AcceptHttpsUrls) {
  LoadUrl(GURL(kHttpsWebOrigin));
  UpdateSslStatus(net::CERT_STATUS_IS_EV, web::SECURITY_STYLE_AUTHENTICATED,
                  web::SSLStatus::NORMAL_CONTENT);

  EXPECT_TRUE(WebStateContentIsSecureHTML(web_state()));
}

// Test that IsContextSecure returns false for HTTP origin.
TEST_F(CredentialManagerTest, HttpIsNotSecureContext) {
  LoadUrl(GURL(kHttpWebOrigin));
  UpdateSslStatus(net::CERT_STATUS_IS_EV, web::SECURITY_STYLE_AUTHENTICATED,
                  web::SSLStatus::NORMAL_CONTENT);

  EXPECT_FALSE(WebStateContentIsSecureHTML(web_state()));
}

// Test that IsContextSecure returns false for HTTPS origin with valid SSL
// certificate but mixed contents.
TEST_F(CredentialManagerTest, InsecureContent) {
  LoadUrl(GURL(kHttpsWebOrigin));
  UpdateSslStatus(net::CERT_STATUS_IS_EV, web::SECURITY_STYLE_AUTHENTICATED,
                  web::SSLStatus::DISPLAYED_INSECURE_CONTENT);

  EXPECT_FALSE(WebStateContentIsSecureHTML(web_state()));
}

// Test that IsContextSecure returns false for HTTPS origin with invalid SSL
// certificate.
TEST_F(CredentialManagerTest, InvalidSSLCertificate) {
  LoadUrl(GURL(kHttpsWebOrigin));
  UpdateSslStatus(net::CERT_STATUS_INVALID, web::SECURITY_STYLE_UNAUTHENTICATED,
                  web::SSLStatus::NORMAL_CONTENT);
  EXPECT_FALSE(WebStateContentIsSecureHTML(web_state()));
}

// Test that data:// URI scheme is not accepted.
TEST_F(CredentialManagerTest, DataUriSchemeIsNotSecureContext) {
  LoadUrl(GURL(kDataUriSchemeOrigin));
  UpdateSslStatus(net::CERT_STATUS_IS_EV, web::SECURITY_STYLE_AUTHENTICATED,
                  web::SSLStatus::NORMAL_CONTENT);

  EXPECT_FALSE(WebStateContentIsSecureHTML(web_state()));
}

// Test that localhost is accepted as secure context.
TEST_F(CredentialManagerTest, LocalhostIsSecureContext) {
  LoadUrl(GURL(kLocalhostOrigin));
  EXPECT_TRUE(WebStateContentIsSecureHTML(web_state()));
}

}  // namespace credential_manager
