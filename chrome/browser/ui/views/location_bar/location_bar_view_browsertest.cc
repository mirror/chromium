// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/location_bar/location_bar_view.h"

#include "base/files/file_path.h"
#include "base/path_service.h"
#include "base/run_loop.h"
#include "base/strings/utf_string_conversions.h"
#include "base/test/scoped_feature_list.h"
#include "chrome/browser/ssl/cert_verifier_browser_test.h"
#include "chrome/browser/ssl/security_state_tab_helper.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_commands.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/browser/ui/views/location_bar/location_icon_view.h"
#include "chrome/browser/ui/views/location_bar/zoom_bubble_view.h"
#include "chrome/browser/ui/views/location_bar/zoom_view.h"
#include "chrome/common/chrome_paths.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/security_state/core/security_state.h"
#include "components/toolbar/test_toolbar_model.h"
#include "components/toolbar/toolbar_field_trial.h"
#include "components/toolbar/toolbar_model_impl.h"
#include "components/zoom/zoom_controller.h"
#include "content/public/common/page_zoom.h"
#include "net/cert/ct_policy_status.h"
#include "net/dns/mock_host_resolver.h"
#include "net/test/cert_test_util.h"
#include "net/test/test_data_directory.h"
#include "net/test/url_request/url_request_mock_http_job.h"
#include "net/url_request/url_request_filter.h"

class LocationBarViewBrowserTest : public InProcessBrowserTest {
 public:
  LocationBarViewBrowserTest() = default;

  LocationBarView* GetLocationBarView() {
    BrowserView* browser_view =
        BrowserView::GetBrowserViewForBrowser(browser());
    return browser_view->GetLocationBarView();
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(LocationBarViewBrowserTest);
};

// Ensure the location bar decoration is added when zooming, and is removed when
// the bubble is closed, but only if zoom was reset.
IN_PROC_BROWSER_TEST_F(LocationBarViewBrowserTest, LocationBarDecoration) {
  content::WebContents* web_contents =
      browser()->tab_strip_model()->GetActiveWebContents();
  zoom::ZoomController* zoom_controller =
      zoom::ZoomController::FromWebContents(web_contents);
  ZoomView* zoom_view = GetLocationBarView()->zoom_view();

  ASSERT_TRUE(zoom_view);
  EXPECT_FALSE(zoom_view->visible());
  EXPECT_FALSE(ZoomBubbleView::GetZoomBubble());

  // Altering zoom should display a bubble. Note ZoomBubbleView closes
  // asynchronously, so precede checks with a run loop flush.
  zoom_controller->SetZoomLevel(content::ZoomFactorToZoomLevel(1.5));
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(zoom_view->visible());
  EXPECT_TRUE(ZoomBubbleView::GetZoomBubble());

  // Close the bubble at other than 100% zoom. Icon should remain visible.
  ZoomBubbleView::CloseCurrentBubble();
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(zoom_view->visible());
  EXPECT_FALSE(ZoomBubbleView::GetZoomBubble());

  // Show the bubble again.
  zoom_controller->SetZoomLevel(content::ZoomFactorToZoomLevel(2.0));
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(zoom_view->visible());
  EXPECT_TRUE(ZoomBubbleView::GetZoomBubble());

  // Remains visible at 100% until the bubble is closed.
  zoom_controller->SetZoomLevel(content::ZoomFactorToZoomLevel(1.0));
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(zoom_view->visible());
  EXPECT_TRUE(ZoomBubbleView::GetZoomBubble());

  // Closing at 100% hides the icon.
  ZoomBubbleView::CloseCurrentBubble();
  base::RunLoop().RunUntilIdle();
  EXPECT_FALSE(zoom_view->visible());
  EXPECT_FALSE(ZoomBubbleView::GetZoomBubble());
}

const char kMockSecureHostname[] = "example-secure.test";
const GURL kMockSecureURL = GURL("https://example-secure.test");

class TestHttpsURLRequestJob : public net::URLRequestMockHTTPJob {
 public:
  TestHttpsURLRequestJob(net::URLRequest* request,
                         net::NetworkDelegate* network_delegate,
                         const base::FilePath& file_path,
                         scoped_refptr<net::X509Certificate> cert,
                         net::CertStatus cert_status)
      : net::URLRequestMockHTTPJob(request, network_delegate, file_path),
        cert_(std::move(cert)),
        cert_status_(cert_status) {}

  void GetResponseInfo(net::HttpResponseInfo* info) override {
    net::URLRequestMockHTTPJob::GetResponseInfo(info);
    info->ssl_info.cert = cert_;
    info->ssl_info.cert_status = cert_status_;
    info->ssl_info.ct_policy_compliance =
        net::ct::CTPolicyCompliance::CT_POLICY_COMPLIES_VIA_SCTS;
  }

 protected:
  ~TestHttpsURLRequestJob() override {}

 private:
  const scoped_refptr<net::X509Certificate> cert_;
  const net::CertStatus cert_status_;

  DISALLOW_COPY_AND_ASSIGN(TestHttpsURLRequestJob);
};

class TestHttpsInterceptor : public net::URLRequestInterceptor {
 public:
  TestHttpsInterceptor(const base::FilePath& base_path,
                       scoped_refptr<net::X509Certificate> cert,
                       net::CertStatus cert_status)
      : base_path_(base_path),
        cert_(std::move(cert)),
        cert_status_(cert_status) {}
  ~TestHttpsInterceptor() override {}

  // net::URLRequestInterceptor:
  net::URLRequestJob* MaybeInterceptRequest(
      net::URLRequest* request,
      net::NetworkDelegate* network_delegate) const override {
    return new TestHttpsURLRequestJob(request, network_delegate, base_path_,
                                      cert_, cert_status_);
  }

 private:
  const base::FilePath base_path_;
  const scoped_refptr<net::X509Certificate> cert_;
  const net::CertStatus cert_status_;

  DISALLOW_COPY_AND_ASSIGN(TestHttpsInterceptor);
};

void AddUrlHandler(const base::FilePath& base_path,
                   scoped_refptr<net::X509Certificate> cert,
                   net::CertStatus cert_status) {
  net::URLRequestFilter* filter = net::URLRequestFilter::GetInstance();
  filter->AddHostnameInterceptor(
      "https", kMockSecureHostname,
      std::unique_ptr<net::URLRequestInterceptor>(
          new TestHttpsInterceptor(base_path, cert, cert_status)));
}

void RemoveUrlHandler() {
  net::URLRequestFilter* filter = net::URLRequestFilter::GetInstance();
  filter->RemoveHostnameHandler("https", kMockSecureHostname);
}

class SecurityIndicatorTest : public InProcessBrowserTest {
 public:
  SecurityIndicatorTest() : InProcessBrowserTest(), cert_(nullptr) {}

  void SetUpInProcessBrowserTestFixture() override {
    cert_ =
        net::ImportCertFromFile(net::GetTestCertsDirectory(), "ok_cert.pem");
    ASSERT_TRUE(cert_);
    ASSERT_TRUE(embedded_test_server()->Start());
  }

  LocationBarView* GetLocationBarView() {
    BrowserView* browser_view =
        BrowserView::GetBrowserViewForBrowser(browser());
    return browser_view->GetLocationBarView();
  }

  void SetUpInterceptor(net::CertStatus cert_status) {
    base::FilePath serve_file;
    PathService::Get(chrome::DIR_TEST_DATA, &serve_file);
    serve_file = serve_file.Append(FILE_PATH_LITERAL("title1.html"));
    content::BrowserThread::PostTask(
        content::BrowserThread::IO, FROM_HERE,
        base::BindOnce(&AddUrlHandler, serve_file, cert_, cert_status));
  }

  void ResetInterceptor() {
    content::BrowserThread::PostTask(content::BrowserThread::IO, FROM_HERE,
                                     base::BindOnce(&RemoveUrlHandler));
  }

 private:
  scoped_refptr<net::X509Certificate> cert_;

  DISALLOW_COPY_AND_ASSIGN(SecurityIndicatorTest);
};

// Check that the security indicator text is correctly set for the various
// variations of the EV UI Study (crbug.com/803501)
IN_PROC_BROWSER_TEST_F(SecurityIndicatorTest, CheckIndicatorText) {
  security_state::SecurityInfo security_info;
  content::WebContents* tab =
      browser()->tab_strip_model()->GetActiveWebContents();
  ASSERT_TRUE(tab);
  SecurityStateTabHelper* helper = SecurityStateTabHelper::FromWebContents(tab);
  ASSERT_TRUE(helper);
  LocationBarView* location_bar_view = GetLocationBarView();

  // Default behavior.
  {
    base::test::ScopedFeatureList scoped_feature_list;
    scoped_feature_list.InitAndDisableFeature(
        toolbar::features::kSimplifyHttpsIndicator);

    // EV HTTPS site
    SetUpInterceptor(net::CERT_STATUS_IS_EV);
    ui_test_utils::NavigateToURL(browser(), kMockSecureURL);
    helper->GetSecurityInfo(&security_info);
    EXPECT_EQ(security_state::EV_SECURE, security_info.security_level);
    EXPECT_TRUE(location_bar_view->ShouldShowLocationIconText());
    EXPECT_EQ(base::ASCIIToUTF16("Test CA [US]"),
              location_bar_view->GetLocationIconText());
    ResetInterceptor();

    // Non-EV HTTPS site
    SetUpInterceptor(0);
    ui_test_utils::NavigateToURL(browser(), kMockSecureURL);
    helper->GetSecurityInfo(&security_info);
    EXPECT_EQ(security_state::SECURE, security_info.security_level);
    EXPECT_TRUE(location_bar_view->ShouldShowLocationIconText());
    EXPECT_EQ(base::ASCIIToUTF16("Secure"),
              location_bar_view->GetLocationIconText());
    ResetInterceptor();

    // HTTP site
    ui_test_utils::NavigateToURL(
        browser(), embedded_test_server()->GetURL("example.test", "/"));
    helper->GetSecurityInfo(&security_info);
    EXPECT_EQ(security_state::NONE, security_info.security_level);
    EXPECT_FALSE(location_bar_view->ShouldShowLocationIconText());
    EXPECT_EQ(base::ASCIIToUTF16(""), location_bar_view->GetLocationIconText());
  }

  // Variation: EV to Secure. Ensure that only the EV UI changes.
  {
    base::test::ScopedFeatureList scoped_feature_list;
    scoped_feature_list.InitAndEnableFeatureWithParameters(
        toolbar::features::kSimplifyHttpsIndicator,
        {{toolbar::features::kSimplifyHttpsIndicatorParameterName,
          toolbar::features::kSimplifyHttpsIndicatorParameterEVToSecure}});

    // EV HTTPS site
    SetUpInterceptor(net::CERT_STATUS_IS_EV);
    ui_test_utils::NavigateToURL(browser(), kMockSecureURL);
    helper->GetSecurityInfo(&security_info);
    EXPECT_EQ(security_state::EV_SECURE, security_info.security_level);
    EXPECT_TRUE(location_bar_view->ShouldShowLocationIconText());
    EXPECT_EQ(base::ASCIIToUTF16("Secure"),
              location_bar_view->GetLocationIconText());
    ResetInterceptor();

    // Non-EV HTTPS site
    SetUpInterceptor(0);
    ui_test_utils::NavigateToURL(browser(), kMockSecureURL);
    helper->GetSecurityInfo(&security_info);
    EXPECT_EQ(security_state::SECURE, security_info.security_level);
    EXPECT_TRUE(location_bar_view->ShouldShowLocationIconText());
    EXPECT_EQ(base::ASCIIToUTF16("Secure"),
              location_bar_view->GetLocationIconText());
    ResetInterceptor();

    // HTTP site
    ui_test_utils::NavigateToURL(
        browser(), embedded_test_server()->GetURL("example.test", "/"));
    helper->GetSecurityInfo(&security_info);
    EXPECT_EQ(security_state::NONE, security_info.security_level);
    EXPECT_FALSE(location_bar_view->ShouldShowLocationIconText());
    EXPECT_EQ(base::ASCIIToUTF16(""), location_bar_view->GetLocationIconText());
  }

  // Variation: Secure to Lock. Ensure that only the non-EV Secure UI changes.
  {
    base::test::ScopedFeatureList scoped_feature_list;
    scoped_feature_list.InitAndEnableFeatureWithParameters(
        toolbar::features::kSimplifyHttpsIndicator,
        {{toolbar::features::kSimplifyHttpsIndicatorParameterName,
          toolbar::features::kSimplifyHttpsIndicatorParameterSecureToLock}});

    // EV HTTPS site
    SetUpInterceptor(net::CERT_STATUS_IS_EV);
    ui_test_utils::NavigateToURL(browser(), kMockSecureURL);
    helper->GetSecurityInfo(&security_info);
    EXPECT_EQ(security_state::EV_SECURE, security_info.security_level);
    EXPECT_TRUE(location_bar_view->ShouldShowLocationIconText());
    EXPECT_EQ(base::ASCIIToUTF16("Test CA [US]"),
              location_bar_view->GetLocationIconText());
    ResetInterceptor();

    // Non-EV HTTPS site
    SetUpInterceptor(0);
    ui_test_utils::NavigateToURL(browser(), kMockSecureURL);
    helper->GetSecurityInfo(&security_info);
    EXPECT_EQ(security_state::SECURE, security_info.security_level);
    EXPECT_FALSE(location_bar_view->ShouldShowLocationIconText());
    EXPECT_EQ(base::ASCIIToUTF16(""), location_bar_view->GetLocationIconText());
    ResetInterceptor();

    // HTTP site
    ui_test_utils::NavigateToURL(
        browser(), embedded_test_server()->GetURL("example.test", "/"));
    helper->GetSecurityInfo(&security_info);
    EXPECT_EQ(security_state::NONE, security_info.security_level);
    EXPECT_FALSE(location_bar_view->ShouldShowLocationIconText());
    EXPECT_EQ(base::ASCIIToUTF16(""), location_bar_view->GetLocationIconText());
  }

  // Variation: Both to Lock. Ensure all HTTPS only shows lock icon.
  {
    base::test::ScopedFeatureList scoped_feature_list;
    scoped_feature_list.InitAndEnableFeatureWithParameters(
        toolbar::features::kSimplifyHttpsIndicator,
        {{toolbar::features::kSimplifyHttpsIndicatorParameterName,
          toolbar::features::kSimplifyHttpsIndicatorParameterBothToLock}});

    // EV HTTPS site
    SetUpInterceptor(net::CERT_STATUS_IS_EV);
    ui_test_utils::NavigateToURL(browser(), kMockSecureURL);
    helper->GetSecurityInfo(&security_info);
    EXPECT_EQ(security_state::EV_SECURE, security_info.security_level);
    EXPECT_FALSE(location_bar_view->ShouldShowLocationIconText());
    EXPECT_EQ(base::ASCIIToUTF16(""), location_bar_view->GetLocationIconText());
    ResetInterceptor();

    // Non-EV HTTPS site
    SetUpInterceptor(0);
    ui_test_utils::NavigateToURL(browser(), kMockSecureURL);
    helper->GetSecurityInfo(&security_info);
    EXPECT_EQ(security_state::SECURE, security_info.security_level);
    EXPECT_FALSE(location_bar_view->ShouldShowLocationIconText());
    EXPECT_EQ(base::ASCIIToUTF16(""), location_bar_view->GetLocationIconText());
    ResetInterceptor();

    // HTTP site
    ui_test_utils::NavigateToURL(
        browser(), embedded_test_server()->GetURL("example.test", "/"));
    helper->GetSecurityInfo(&security_info);
    EXPECT_EQ(security_state::NONE, security_info.security_level);
    EXPECT_FALSE(location_bar_view->ShouldShowLocationIconText());
    EXPECT_EQ(base::ASCIIToUTF16(""), location_bar_view->GetLocationIconText());
  }
}
