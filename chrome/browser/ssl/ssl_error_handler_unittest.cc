// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ssl/ssl_error_handler.h"

#include "base/callback.h"
#include "base/macros.h"
#include "base/memory/ptr_util.h"
#include "base/metrics/field_trial.h"
#include "base/run_loop.h"
#include "base/test/histogram_tester.h"
#include "base/test/scoped_feature_list.h"
#include "base/test/simple_test_clock.h"
#include "base/test/simple_test_tick_clock.h"
#include "base/time/time.h"
#include "chrome/browser/captive_portal/captive_portal_service.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ssl/common_name_mismatch_handler.h"
#include "chrome/browser/ssl/ssl_error_assistant.pb.h"
#include "chrome/common/features.h"
#include "chrome/test/base/chrome_render_view_host_test_harness.h"
#include "chrome/test/base/testing_profile.h"
#include "components/captive_portal/captive_portal_testing_utils.h"
#include "components/network_time/network_time_test_utils.h"
#include "components/network_time/network_time_tracker.h"
#include "components/prefs/testing_pref_service.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/notification_service.h"
#include "net/base/net_errors.h"
#include "net/cert/cert_status_flags.h"
#include "net/cert/x509_certificate.h"
#include "net/http/http_response_headers.h"
#include "net/ssl/ssl_info.h"
#include "net/test/cert_test_util.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "net/test/embedded_test_server/http_response.h"
#include "net/test/test_certificate_data.h"
#include "net/test/test_data_directory.h"
#include "net/url_request/url_request_test_util.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {

const char kCertDateErrorHistogram[] =
    "interstitial.ssl_error_handler.cert_date_error_delay";

const net::SHA256HashValue kCertPublicKeyHashValue = {{0x01, 0x02}};

const std::string kOkayCertName = "ok_cert.pem";

const std::string kMcAfeeCert =
    "-----BEGIN PRIVATE "
    "KEY-----\nMIIEvQIBADANBgkqhkiG9w0BAQEFAASCBKcwggSjAgEAAoIBAQCmN5NFlT/"
    "MRX40\neXR91XlTk/dlUU02MXGfoJlilOx6m4m864BQ2C8LHZ29FZkHhvy1LB+rqv/"
    "HlHs7\nSUUi6JzYUFI/gx270a2D4qdzBUOxTMh+kmxGTKNYAcqN1DLG4ALSxsucqipLSi2K\n/"
    "7IP2YAaLxKi7imC6NlVFIZzMSVdrf8DsyBurs33PKqsw+"
    "anQWflM3CQXKLW1ZVu\nZHiFtd7J6Vq+24/"
    "dIAOI5y8FQO1HX41EZzDBpjuJVUXHm6rqMgJss6usawmGOTAo\n5BgoqAL3zoHqa9GUZHKn9bD"
    "48dAjge5C5BA1O6SF/agAVDWOkEXlaznLSpIdLR4M\nsarzM/"
    "xvAgMBAAECggEAPVgHb0fM2tvO3xjwCDzP4Subw3XE9Do9P/"
    "dqhHqH6Bwd\nMm8L1Upj2ktc2trstl14SHj3W0jpy0OeU1wQOvj0YdUK9cuzHo0c566vtAB+"
    "YiH/\nYM8zTKKeKi3HDp8A+mrUP+wEW1Zl85F1y3Xgxn60C3XR//"
    "W7tkevZfoEQOEjdDXZ\ngZ+n0/d54xUvfR/lGfVT/PQzHdnvxi+qoK1QXdPMW0UzAXoUxcyvm/"
    "T6IyXn/"
    "VIO\nq8NLBjGMNMfFWpQM0KQmRSyLj06vdspUeIv661qaPtzhqlKZKur+"
    "c3S1fdUGxASh\nfwuoK3ZIMVVcTscttv30wDwQW4E46/"
    "W7hauzmHF8eQKBgQDar4SncQy9zBIpDplk\nw2je0wDpFOGtObmXkrPzcm2E7QfrmORpcAeYku"
    "iTpl+m81HiMgmhFyWJVUobyq99\nX9QE74RpF34tVJpGwdxrkc6Tkb+"
    "AJJrcovHyrpLdiQKj8MaILy5F3NtXPoH6E+"
    "sb\nEFtKP261v85aEShtqWeA17xAlQKBgQDClClKy3M9fqyyTUcM8l+XIm56NY9/"
    "KiZS\n41JEX2RQAhlup35x/O4GFjIxfCfJAk+mdXyjfuPH+LV2XB+HO/"
    "hqIeMkL3vSxNP0\n+53kCU0lVgsbOb3+zVp6/"
    "q0rffajhiGdkBKkr3VLQwAJ4KV4dSvqQavbojggU2mS\njqwF7dEz8wKBgEfztgbNqG6ORW9i0"
    "U+ZB9lAvazy5On08z6O00yHWseAobl/va9/"
    "\nX9Bwjbt+4zvwFQICiq6JCh2shUytnJ34uK7DC+L0CtzpWDVttZNjDv/41XRZFub/"
    "\nafH6RHJFkSvVnczTETuFVlnXRDa+"
    "9P0bnZ3JTyjXqhJyUMX8FgULdnIdAoGBAKyT\nWCQ8WhN6nJTvsUqsVVfoc+/"
    "0PTdeHOt7LuXxbTkBA6l+6MkUuc+"
    "wNAhrw6WmZRqQ\nLWGdE02zKs61WklJSURoldrsOnhvlkY8PIeaMFAjBmq5RegOHmfkJZbje7M"
    "BLiNf\nOwnRjMiZvhkEo/44fiCg6lo8gGc7U/"
    "FmlPAVQ209AoGAdv7YAwuWaKnVAoU9wgaT\npyIN8pQ4JMG+"
    "STfGBsc3tFk2AHJ303cOjGqMjsASSlPMmmZEuj12it1MyTl7oHXw\nUrx6Iqouzqr98lOrM3/"
    "qHZ0giNf+TFuD3oOHvWEW3Tu7EAE2hPthWhbOQrmzjplP\ngAXtO6FCQl1o4vWA+zXSYYA=\n-"
    "----END PRIVATE KEY-----\n-----BEGIN "
    "CERTIFICATE-----\nMIIEOzCCAyOgAwIBAgIJAJpsNuOa+"
    "Ek7MA0GCSqGSIb3DQEBCwUAMIGzMQswCQYD\nVQQGEwJVUzETMBEGA1UECAwKQ2FsaWZvcm5pY"
    "TEWMBQGA1UEBwwNU2FuIEZyYW5j\naXNjbzEbMBkGA1UECgwSTWNBZmVlIFdlYiBHYXRld2F5M"
    "RswGQYDVQQLDBJNY0Fm\nZWUgV2ViIEdhdGV3YXkxGzAZBgNVBAMMEk1jQWZlZSBXZWIgR2F0Z"
    "XdheTEgMB4G\nCSqGSIb3DQEJARYRZXhhbXBsZUBnbWFpbC5jb20wHhcNMTcwODAxMDAxMjA0W"
    "hcN\nMTgwODAxMDAxMjA0WjCBszELMAkGA1UEBhMCVVMxEzARBgNVBAgMCkNhbGlmb3Ju\naWE"
    "xFjAUBgNVBAcMDVNhbiBGcmFuY2lzY28xGzAZBgNVBAoMEk1jQWZlZSBXZWIg\nR2F0ZXdheTE"
    "bMBkGA1UECwwSTWNBZmVlIFdlYiBHYXRld2F5MRswGQYDVQQDDBJN\nY0FmZWUgV2ViIEdhdGV"
    "3YXkxIDAeBgkqhkiG9w0BCQEWEWV4YW1wbGVAZ21haWwu\nY29tMIIBIjANBgkqhkiG9w0BAQE"
    "FAAOCAQ8AMIIBCgKCAQEApjeTRZU/"
    "zEV+NHl0\nfdV5U5P3ZVFNNjFxn6CZYpTsepuJvOuAUNgvCx2dvRWZB4b8tSwfq6r/"
    "x5R7O0lF\nIuic2FBSP4Mdu9Gtg+KncwVDsUzIfpJsRkyjWAHKjdQyxuAC0sbLnKoqS0otiv+"
    "y\nD9mAGi8Sou4pgujZVRSGczElXa3/"
    "A7Mgbq7N9zyqrMPmp0Fn5TNwkFyi1tWVbmR4\nhbXeyelavtuP3SADiOcvBUDtR1+"
    "NRGcwwaY7iVVFx5uq6jICbLOrrGsJhjkwKOQY\nKKgC986B6mvRlGRyp/"
    "Ww+"
    "PHQI4HuQuQQNTukhf2oAFQ1jpBF5Ws5y0qSHS0eDLGq\n8zP8bwIDAQABo1AwTjAdBgNVHQ4EF"
    "gQUt6PpqiCkogIa5dDMGKRJwOxMWtwwHwYD\nVR0jBBgwFoAUt6PpqiCkogIa5dDMGKRJwOxMW"
    "twwDAYDVR0TBAUwAwEB/"
    "zANBgkq\nhkiG9w0BAQsFAAOCAQEACycC6vjzcMQ8OtBEf8vRU34Qw+3iKMBgM18zqoytLo+"
    "h\nyVr+2GG4wS/wewXIQiRgPfh+aiqiATfAZWfoBLV7RW5FREw9r6n/g/"
    "6y2msAtOMW\n6m0EOlpY28h2ouxRi8rfTeaNNSGxBG302eYGmrrjBKSEOseXlWcD1jsKcml2TN"
    "Or\nBVXOdr87HDfsZ29zYU9eyid4K45AXhJzMRjfGWywGqzZHpS6X5zoPh1FnXICJL4O\nx3az"
    "B92aP3FP/"
    "NuwN3y7YWasWOEow5zWmo+"
    "ZF2Twx0FDTVMS4vn56gx3NJTJppZc\noonlMHjNfmpwKaxjNE1g6cQ+6wmw6m2Gt6d9pvfvqA="
    "=\n-----END CERTIFICATE-----";
const std::string kCyberoamCert =
    "-----BEGIN PRIVATE "
    "KEY-----"
    "\nMIIEvQIBADANBgkqhkiG9w0BAQEFAASCBKcwggSjAgEAAoIBAQDF0hNIEKBX6oYH\nDer4e8"
    "TbGn31M5bCKNXJClUFZi7ay5EGLm1kA27BykwpUvAdJ6Ti26htZdwOj6er\nwKFpR1n16U+"
    "VzNdMlSiuJ4sOtXk/XsIdXlt3NrS5rupt+Nw1S3/"
    "z+t5jTFmIVNf4\nTIBVB03uciSglXPXBWnNCMSfdflTTNcbU0vqSE6aTYS5quAnyFSupikFzI+"
    "t+N7S\nuvS/OnLiZP3PbdA4CIgEKtP4NQ0/"
    "4vxcJxK+p1JmysCGySDij6lSt3pVhpZnr2Kx\ndVdf/"
    "oj1E2DRWeYCB3DwSLJ1zesve+"
    "uabmO8mYuMRLWEnyPVckFJ7E6xN0S2msw8\nTcIBLDCrAgMBAAECggEASIJmGuYtP8xw8CkD1a"
    "zYTU96l+QfAHAjXJty6pW32u7+\n63sdEanIcTQLkqIZ7TXY1GabZ+"
    "4MKTIyzVvATmye5upsB4ab7DBIRv2rlOqj1LyQ\n/"
    "l7Vu+7Lc7xPpZuwa8CO2Ak+drtHnhcrSpIQABq7UAl6r3nYihor28G5AJInn4ZH\n2/"
    "+59x0xGMf2f2rl90MI28bnD/+RkhXWQly564PkPmVBJjwBq7NdqizthWGChZFW\nG7jwW/"
    "yzlusu881/"
    "u9fmU23CnrujluJ7dZP7cgZNPbQ2kokTYbRKk9P7Ij+"
    "w04yj\ny1qXVYdCngAkGQW5ZF8WaOmYNTg2quTuOrq0y1Qv4QKBgQDklkFH7LpBtAvHP6F2\nQ"
    "1ENQWl2xADYWjbSDWohUYXVMQtEb5Oo0+HeUox/SLXwO5w4PgTi2wmdS/"
    "FBrpIO\nsh5RR9bPJ98xJTGkpnUW9ezKFRzXB9Nrevi2FxRWR101/"
    "r+SO7YoDebaMfCvOEMd\nlt/"
    "Wb2oo7O66iMLo6ath33QsswKBgQDdi0bwKN+"
    "dtKX3HaXdDJbSeCGImapzK1p2\nDw0LhhyhP8ZNK3NWTlYB8BU47oKsfVyS1DTFll0RNLHqydh"
    "FsLwBHFUrWq/q6bfs\n49p+dW6pBzUiR4lFiH0yPz/"
    "Asd+n64Oj3J27ldGIRImVyqq7WMLwyhjCmzUJ8YeQ\nzKVSZ77YKQKBgQDOyZq+"
    "zUnl3FH3avnO+bDeE/mrZj75egsz1MzY3V378gUmYOWs\nihk/vV+6WfW1IMf/"
    "HSPgcZRuBuB2sc0RnFLO5vVNq90+"
    "IU6ai7dLESin25eec0bw\naWuLlq9gKekJyq5VRM1RyNm5NApvCK+2meFepKXd5hFFECl6+"
    "MCAptv6/wKBgAw0\n71yuM9wBpx9dj9UFK44/"
    "NkHLjbu4yMw2ehaHdi5gUDHSUhGQUWbb7p9rbOHKKn6g\nmKPGbCxVhfQU2QjF+"
    "yXohLzVy1mbNRolV9Wd7NH9Zq0T2flt0tAg8gdRBay/6knu\nDvbWA2yJsjcBqrSV6G/"
    "SAtk+Kgb5Oun8jKNkufw5AoGAKWlFdAiFGuugqAhm9bTE\n9VCNjNjj3GpaPO8zl4RP+"
    "X2fJvcmFNxpRgZUzDhH40noHEoRrHviTXOW3uZLmG+o\nVxEOViMPSj4ec12U+t9KuJG/"
    "tkBLDjzty3IuY8IyPfT7A4grjbqLCtb2zV4tKgtu\nHJOYRbEYvDk+mrLAZvcW+dw=\n-----"
    "END PRIVATE KEY-----\n-----BEGIN "
    "CERTIFICATE-----"
    "\nMIIEIzCCAwugAwIBAgIJAJZS14qNbWmdMA0GCSqGSIb3DQEBCwUAMIGnMQswCQYD\nVQQGEw"
    "JVUzETMBEGA1UECAwKQ2FsaWZvcm5pYTEWMBQGA1UEBwwNU2FuIEZyYW5j\naXNjbzERMA8GA1"
    "UECgwIQ3liZXJvYW0xETAPBgNVBAsMCEN5YmVyb2FtMSQwIgYD\nVQQDDBtDeWJlcm9hbSBTU0"
    "wgQ0FfQUgzNDNKSzk4UjcxHzAdBgkqhkiG9w0BCQEW\nEHRlc3RAZXhhbXBsZS5jb20wHhcNMT"
    "cwODAxMTczMDA0WhcNMTgwODAxMTczMDA0\nWjCBpzELMAkGA1UEBhMCVVMxEzARBgNVBAgMCk"
    "NhbGlmb3JuaWExFjAUBgNVBAcM\nDVNhbiBGcmFuY2lzY28xETAPBgNVBAoMCEN5YmVyb2FtMR"
    "EwDwYDVQQLDAhDeWJl\ncm9hbTEkMCIGA1UEAwwbQ3liZXJvYW0gU1NMIENBX0FIMzQzSks5OF"
    "I3MR8wHQYJ\nKoZIhvcNAQkBFhB0ZXN0QGV4YW1wbGUuY29tMIIBIjANBgkqhkiG9w0BAQEFAA"
    "OC\nAQ8AMIIBCgKCAQEAxdITSBCgV+qGBw3q+"
    "HvE2xp99TOWwijVyQpVBWYu2suRBi5t\nZANuwcpMKVLwHSek4tuobWXcDo+"
    "nq8ChaUdZ9elPlczXTJUorieLDrV5P17CHV5b\ndza0ua7qbfjcNUt/8/"
    "reY0xZiFTX+"
    "EyAVQdN7nIkoJVz1wVpzQjEn3X5U0zXG1NL\n6khOmk2EuargJ8hUrqYpBcyPrfje0rr0vzpy4"
    "mT9z23QOAiIBCrT+DUNP+L8XCcS\nvqdSZsrAhskg4o+pUrd6VYaWZ69isXVXX/"
    "6I9RNg0VnmAgdw8Eiydc3rL3vrmm5j\nvJmLjES1hJ8j1XJBSexOsTdEtprMPE3CASwwqwIDAQ"
    "ABo1AwTjAdBgNVHQ4EFgQU\nQeAXfiKxtfTgJEJgm4Bg1zR4zWEwHwYDVR0jBBgwFoAUQeAXfi"
    "KxtfTgJEJgm4Bg\n1zR4zWEwDAYDVR0TBAUwAwEB/"
    "zANBgkqhkiG9w0BAQsFAAOCAQEApmGJ7Rv1wfiU\nSyxu6UMjHRbuWu7w40L82rbfiQj+"
    "0zssTf6Bjcyi1De3HX1mhfKwSeKKWEKeYMg0\nbmKaJrHuQ6vQ+"
    "OGDmYhlu2lzY2DTpB7lDdJJR8FgJP+pJWPpp3xvUYfSAfRda6hS\nHSUJkzrJ+rWNw8bWO5Ce+"
    "2Aysb7mkuOmRWkg1UcZw3hepqBg7cQHjeMtXV+"
    "QMT06\nT9Khb042X1MExPCc7xoIwxRbLcOCGyCbbcbB7w6HzgkJ2dA/"
    "mS+sNfDtGRq4PxLd\nyvmNowVyb9OGj2cm9zHTcQKX1eB8Ch4L1bxpk+"
    "L1TixiZPGE4byq2ZntjVY7YA3U\nZvlsiSgAag==\n-----END CERTIFICATE-----";

// Runs |quit_closure| on the UI thread once a URL request has been
// seen. Returns a request that hangs.
std::unique_ptr<net::test_server::HttpResponse> WaitForRequest(
    const base::Closure& quit_closure,
    const net::test_server::HttpRequest& request) {
  content::BrowserThread::PostTask(content::BrowserThread::UI, FROM_HERE,
                                   quit_closure);
  return base::MakeUnique<net::test_server::HungResponse>();
}

class TestSSLErrorHandler : public SSLErrorHandler {
 public:
  TestSSLErrorHandler(
      std::unique_ptr<Delegate> delegate,
      content::WebContents* web_contents,
      Profile* profile,
      int cert_error,
      const net::SSLInfo& ssl_info,
      const GURL& request_url,
      const base::Callback<void(content::CertificateRequestResultType)>&
          callback)
      : SSLErrorHandler(std::move(delegate),
                        web_contents,
                        profile,
                        cert_error,
                        ssl_info,
                        request_url,
                        callback) {}

  using SSLErrorHandler::StartHandlingError;
};

class TestSSLErrorHandlerDelegate : public SSLErrorHandler::Delegate {
 public:
  TestSSLErrorHandlerDelegate(Profile* profile,
                              content::WebContents* web_contents,
                              const net::SSLInfo& ssl_info)
      : profile_(profile),
        captive_portal_checked_(false),
        suggested_url_exists_(false),
        suggested_url_checked_(false),
        ssl_interstitial_shown_(false),
        bad_clock_interstitial_shown_(false),
        captive_portal_interstitial_shown_(false),
        content_filter_interstitial_shown_(false),
        redirected_to_suggested_url_(false),
        is_overridable_error_(true) {}

  void SendCaptivePortalNotification(
      captive_portal::CaptivePortalResult result) {
    CaptivePortalService::Results results;
    results.previous_result = captive_portal::RESULT_INTERNET_CONNECTED;
    results.result = result;
    content::NotificationService::current()->Notify(
        chrome::NOTIFICATION_CAPTIVE_PORTAL_CHECK_RESULT,
        content::Source<Profile>(profile_),
        content::Details<CaptivePortalService::Results>(&results));
  }

  void SendSuggestedUrlCheckResult(
      const CommonNameMismatchHandler::SuggestedUrlCheckResult& result,
      const GURL& suggested_url) {
    suggested_url_callback_.Run(result, suggested_url);
  }

  int captive_portal_checked() const { return captive_portal_checked_; }
  int ssl_interstitial_shown() const { return ssl_interstitial_shown_; }
  int captive_portal_interstitial_shown() const {
    return captive_portal_interstitial_shown_;
  }
  int content_filter_interstitial_shown() const {
    return content_filter_interstitial_shown_;
  }
  bool bad_clock_interstitial_shown() const {
    return bad_clock_interstitial_shown_;
  }
  bool suggested_url_checked() const { return suggested_url_checked_; }
  bool redirected_to_suggested_url() const {
    return redirected_to_suggested_url_;
  }

  void set_suggested_url_exists() { suggested_url_exists_ = true; }
  void set_non_overridable_error() { is_overridable_error_ = false; }

  void ClearSeenOperations() {
    captive_portal_checked_ = false;
    suggested_url_exists_ = false;
    suggested_url_checked_ = false;
    ssl_interstitial_shown_ = false;
    bad_clock_interstitial_shown_ = false;
    captive_portal_interstitial_shown_ = false;
    content_filter_interstitial_shown_ = false;
    redirected_to_suggested_url_ = false;
  }

 private:
  void CheckForCaptivePortal() override {
    captive_portal_checked_ = true;
  }

  bool GetSuggestedUrl(const std::vector<std::string>& dns_names,
                       GURL* suggested_url) const override {
    if (!suggested_url_exists_)
      return false;
    *suggested_url = GURL("www.example.com");
    return true;
  }

  void ShowSSLInterstitial() override { ssl_interstitial_shown_ = true; }

  void ShowBadClockInterstitial(const base::Time& now,
                                ssl_errors::ClockState clock_state) override {
    bad_clock_interstitial_shown_ = true;
  }

  void ShowCaptivePortalInterstitial(const GURL& landing_url) override {
    captive_portal_interstitial_shown_ = true;
  }

  void ShowContentFilterInterstitial() override {
    content_filter_interstitial_shown_ = true;
  }

  void CheckSuggestedUrl(
      const GURL& suggested_url,
      const CommonNameMismatchHandler::CheckUrlCallback& callback) override {
    DCHECK(suggested_url_callback_.is_null());
    suggested_url_checked_ = true;
    suggested_url_callback_ = callback;
  }

  void NavigateToSuggestedURL(const GURL& suggested_url) override {
    redirected_to_suggested_url_ = true;
  }

  bool IsErrorOverridable() const override { return is_overridable_error_; }

  Profile* profile_;
  bool captive_portal_checked_;
  bool suggested_url_exists_;
  bool suggested_url_checked_;
  bool ssl_interstitial_shown_;
  bool bad_clock_interstitial_shown_;
  bool captive_portal_interstitial_shown_;
  bool content_filter_interstitial_shown_;
  bool redirected_to_suggested_url_;
  bool is_overridable_error_;
  CommonNameMismatchHandler::CheckUrlCallback suggested_url_callback_;

  DISALLOW_COPY_AND_ASSIGN(TestSSLErrorHandlerDelegate);
};

}  // namespace

// A class to test name mismatch errors. Creates an error handler with a name
// mismatch error.
class SSLErrorHandlerNameMismatchTest : public ChromeRenderViewHostTestHarness {
 public:
  SSLErrorHandlerNameMismatchTest() : field_trial_list_(nullptr) {}
  ~SSLErrorHandlerNameMismatchTest() override {}

  void SetUp() override {
    ChromeRenderViewHostTestHarness::SetUp();
    SSLErrorHandler::ResetConfigForTesting();
    SSLErrorHandler::SetInterstitialDelayForTesting(base::TimeDelta());
    ssl_info_.cert = GetCertificate();
    ssl_info_.cert_status = net::CERT_STATUS_COMMON_NAME_INVALID;
    ssl_info_.public_key_hashes.push_back(
        net::HashValue(kCertPublicKeyHashValue));

    delegate_ =
        new TestSSLErrorHandlerDelegate(profile(), web_contents(), ssl_info_);
    error_handler_.reset(new TestSSLErrorHandler(
        std::unique_ptr<SSLErrorHandler::Delegate>(delegate_), web_contents(),
        profile(), net::MapCertStatusToNetError(ssl_info_.cert_status),
        ssl_info_,
        GURL(),  // request_url
        base::Callback<void(content::CertificateRequestResultType)>()));
  }

  void TearDown() override {
    EXPECT_FALSE(error_handler()->IsTimerRunningForTesting());
    error_handler_.reset(nullptr);
    SSLErrorHandler::ResetConfigForTesting();
    ChromeRenderViewHostTestHarness::TearDown();
  }

  TestSSLErrorHandler* error_handler() { return error_handler_.get(); }
  TestSSLErrorHandlerDelegate* delegate() { return delegate_; }

  const net::SSLInfo& ssl_info() { return ssl_info_; }

 private:
  // Returns a certificate for the test. Virtual to allow derived fixtures to
  // use a certificate with different characteristics.
  virtual scoped_refptr<net::X509Certificate> GetCertificate() {
    return net::ImportCertFromFile(net::GetTestCertsDirectory(),
                                   "subjectAltName_www_example_com.pem");
  }

  net::SSLInfo ssl_info_;
  std::unique_ptr<TestSSLErrorHandler> error_handler_;
  TestSSLErrorHandlerDelegate* delegate_;
  base::FieldTrialList field_trial_list_;

  DISALLOW_COPY_AND_ASSIGN(SSLErrorHandlerNameMismatchTest);
};

// A class to test name mismatch errors, where the certificate lacks a
// SubjectAltName. Creates an error handler with a name mismatch error.
class SSLErrorHandlerNameMismatchNoSANTest
    : public SSLErrorHandlerNameMismatchTest {
 public:
  SSLErrorHandlerNameMismatchNoSANTest() {}

 private:
  // Return a certificate that contains no SubjectAltName field.
  scoped_refptr<net::X509Certificate> GetCertificate() override {
    return net::ImportCertFromFile(net::GetTestCertsDirectory(), "ok_cert.pem");
  }

  DISALLOW_COPY_AND_ASSIGN(SSLErrorHandlerNameMismatchNoSANTest);
};

// A class to test the captive portal certificate list feature. Creates an error
// handler with a name mismatch error by default. The error handler can be
// recreated by calling ResetErrorHandlerForCaptivePortalTest() with an
// appropriate cert status.
class SSLErrorAssistantTest : public ChromeRenderViewHostTestHarness {
 public:
  SSLErrorAssistantTest() : field_trial_list_(nullptr) {}

  void SetUp() override {
    ChromeRenderViewHostTestHarness::SetUp();
    SSLErrorHandler::ResetConfigForTesting();
    SSLErrorHandler::SetInterstitialDelayForTesting(base::TimeDelta());
    ResetErrorHandler(kOkayCertName, std::string(),
                      net::CERT_STATUS_COMMON_NAME_INVALID);
  }

  void TearDown() override {
    EXPECT_FALSE(error_handler()->IsTimerRunningForTesting());
    error_handler_.reset(nullptr);
    SSLErrorHandler::ResetConfigForTesting();
    ChromeRenderViewHostTestHarness::TearDown();
  }

  TestSSLErrorHandler* error_handler() { return error_handler_.get(); }
  TestSSLErrorHandlerDelegate* delegate() { return delegate_; }

  const net::SSLInfo& ssl_info() { return ssl_info_; }

 protected:
  void SetCaptivePortalFeatureEnabled(bool enabled) {
    if (enabled) {
      scoped_feature_list_.InitFromCommandLine(
          "CaptivePortalCertificateList" /* enabled */,
          std::string() /* disabled */);
    } else {
      scoped_feature_list_.InitFromCommandLine(
          std::string(), "CaptivePortalCertificateList" /* disabled */);
    }
  }

  // Loads the contents of a certificate into an X509Certificate object from
  // a string.
  scoped_refptr<net::X509Certificate> LoadCertFromString(
      std::string cert_data) {
    net::CertificateList certs_found =
        net::X509Certificate::CreateCertificateListFromBytes(
            cert_data.data(), cert_data.size(),
            net::X509Certificate::FORMAT_AUTO);
    if (certs_found.empty()) {
      return NULL;
    }
    return certs_found[0];
  }

  // Resets the SSLErrorHandler to contain the certificate and the certificate
  // status specified. This function can either load a certificate into the
  // handler from a file name specified in the cert_name parameter, or from a
  // string cert_data containing the contents of the certificate. If both the
  // cert_name and cert_data parameters are provided, the function defaults to
  // loading the certificate from file.
  void ResetErrorHandler(std::string cert_name,
                         std::string cert_data,
                         net::CertStatus cert_status) {
    if (cert_name.empty() && cert_data.empty()) {
      return;
    }

    ssl_info_.Reset();
    // If a certificate name is provided, load the certificate from file.
    // Otherwise, load the certificate from the cert_data field.
    ssl_info_.cert =
        !cert_name.empty()
            ? net::ImportCertFromFile(net::GetTestCertsDirectory(), cert_name)
            : LoadCertFromString(cert_data);
    ssl_info_.cert_status = cert_status;
    ssl_info_.public_key_hashes.push_back(
        net::HashValue(kCertPublicKeyHashValue));

    delegate_ =
        new TestSSLErrorHandlerDelegate(profile(), web_contents(), ssl_info_);
    error_handler_.reset(new TestSSLErrorHandler(
        std::unique_ptr<SSLErrorHandler::Delegate>(delegate_), web_contents(),
        profile(), net::MapCertStatusToNetError(ssl_info_.cert_status),
        ssl_info_,
        GURL(),  // request_url
        base::Callback<void(content::CertificateRequestResultType)>()));
  }

  // Deletes the current error handler and creates a new one with the given
  // |cert_status|.
  void ResetErrorHandlerForCaptivePortalTest(net::CertStatus cert_status) {
    ResetErrorHandler(kOkayCertName, std::string(), cert_status);

    // Enable finch experiment for captive portal interstitials.
    ASSERT_TRUE(base::FieldTrialList::CreateFieldTrial(
        "CaptivePortalInterstitial", "Enabled"));
    // Enable finch experiment for SSL common name mismatch handling.
    ASSERT_TRUE(base::FieldTrialList::CreateFieldTrial(
        "SSLCommonNameMismatchHandling", "Enabled"));
  }

  void TestNoCaptivePortalInterstitial() {
    base::HistogramTester histograms;

    EXPECT_FALSE(error_handler()->IsTimerRunningForTesting());
    EXPECT_EQ(1u, ssl_info().public_key_hashes.size());

    auto config_proto =
        base::MakeUnique<chrome_browser_ssl::SSLErrorAssistantConfig>();
    config_proto->add_captive_portal_cert()->set_sha256_hash(
        "sha256/aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa");
    config_proto->add_captive_portal_cert()->set_sha256_hash(
        ssl_info().public_key_hashes[0].ToString());
    config_proto->add_captive_portal_cert()->set_sha256_hash(
        "sha256/bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb");
    SSLErrorHandler::SetErrorAssistantProto(std::move(config_proto));

    error_handler()->StartHandlingError();

    // Timer should start for captive portal detection.
    EXPECT_TRUE(error_handler()->IsTimerRunningForTesting());
    EXPECT_TRUE(delegate()->captive_portal_checked());
    EXPECT_FALSE(delegate()->ssl_interstitial_shown());
    EXPECT_FALSE(delegate()->captive_portal_interstitial_shown());
    EXPECT_FALSE(delegate()->suggested_url_checked());

    base::RunLoop().RunUntilIdle();

    EXPECT_FALSE(error_handler()->IsTimerRunningForTesting());
    EXPECT_TRUE(delegate()->captive_portal_checked());
    EXPECT_TRUE(delegate()->ssl_interstitial_shown());
    EXPECT_FALSE(delegate()->captive_portal_interstitial_shown());
    EXPECT_FALSE(delegate()->suggested_url_checked());

    // Check that the histogram for the captive portal cert was recorded.
    histograms.ExpectTotalCount(SSLErrorHandler::GetHistogramNameForTesting(),
                                2);
    histograms.ExpectBucketCount(SSLErrorHandler::GetHistogramNameForTesting(),
                                 SSLErrorHandler::HANDLE_ALL, 1);
    histograms.ExpectBucketCount(
        SSLErrorHandler::GetHistogramNameForTesting(),
        SSLErrorHandler::SHOW_SSL_INTERSTITIAL_OVERRIDABLE, 1);
  }

  // Sets up a mock SSL Error Assistant config with regexes that match a
  // McAfee Web Gateway certificate or a Cyberoam certificate and starts
  // handling certificate errors.
  void RunContentFilterTest() {
    auto config_proto =
        base::MakeUnique<chrome_browser_ssl::SSLErrorAssistantConfig>();
    chrome_browser_ssl::ContentFilterEntry* filter1 =
        config_proto->add_content_filter();
    filter1->set_name("McAfee Web Gateway");
    filter1->set_regex("McAfee Web Gateway");
    chrome_browser_ssl::ContentFilterEntry* filter2 =
        config_proto->add_content_filter();
    filter2->set_name("Cyberoam");
    filter2->set_regex("Cyberoam SSL CA_[A-Z0-9]+");
    SSLErrorHandler::SetErrorAssistantProto(std::move(config_proto));

    error_handler()->StartHandlingError();
    base::RunLoop().RunUntilIdle();
  }

 private:
  net::SSLInfo ssl_info_;
  std::unique_ptr<TestSSLErrorHandler> error_handler_;
  TestSSLErrorHandlerDelegate* delegate_;
  base::FieldTrialList field_trial_list_;
  base::test::ScopedFeatureList scoped_feature_list_;

  DISALLOW_COPY_AND_ASSIGN(SSLErrorAssistantTest);
};

class SSLErrorHandlerDateInvalidTest : public ChromeRenderViewHostTestHarness {
 public:
  SSLErrorHandlerDateInvalidTest()
      : field_trial_test_(new network_time::FieldTrialTest()),
        clock_(new base::SimpleTestClock),
        tick_clock_(new base::SimpleTestTickClock),
        test_server_(new net::EmbeddedTestServer) {
    SetThreadBundleOptions(content::TestBrowserThreadBundle::REAL_IO_THREAD);
    network_time::NetworkTimeTracker::RegisterPrefs(pref_service_.registry());
  }

  void SetUp() override {
    ChromeRenderViewHostTestHarness::SetUp();
    SSLErrorHandler::ResetConfigForTesting();

    field_trial_test()->SetNetworkQueriesWithVariationsService(
        false, 0.0,
        network_time::NetworkTimeTracker::FETCHES_IN_BACKGROUND_ONLY);
    tracker_.reset(new network_time::NetworkTimeTracker(
        std::unique_ptr<base::Clock>(clock_),
        std::unique_ptr<base::TickClock>(tick_clock_), &pref_service_,
        new net::TestURLRequestContextGetter(
            content::BrowserThread::GetTaskRunnerForThread(
                content::BrowserThread::IO))));

    // Do this to be sure that |is_null| returns false.
    clock_->Advance(base::TimeDelta::FromDays(111));
    tick_clock_->Advance(base::TimeDelta::FromDays(222));

    SSLErrorHandler::SetInterstitialDelayForTesting(base::TimeDelta());
    ssl_info_.cert =
        net::ImportCertFromFile(net::GetTestCertsDirectory(), "ok_cert.pem");
    ssl_info_.cert_status = net::CERT_STATUS_DATE_INVALID;

    delegate_ =
        new TestSSLErrorHandlerDelegate(profile(), web_contents(), ssl_info_);
    error_handler_.reset(new TestSSLErrorHandler(
        std::unique_ptr<SSLErrorHandler::Delegate>(delegate_), web_contents(),
        profile(), net::MapCertStatusToNetError(ssl_info_.cert_status),
        ssl_info_,
        GURL(),  // request_url
        base::Callback<void(content::CertificateRequestResultType)>()));
    error_handler_->SetNetworkTimeTrackerForTesting(tracker_.get());

    // Fix flakiness in case system time is off and triggers a bad clock
    // interstitial. https://crbug.com/666821#c50
    ssl_errors::SetBuildTimeForTesting(base::Time::Now());
  }

  void TearDown() override {
    if (error_handler()) {
      EXPECT_FALSE(error_handler()->IsTimerRunningForTesting());
      error_handler_.reset(nullptr);
    }
    SSLErrorHandler::ResetConfigForTesting();
    ChromeRenderViewHostTestHarness::TearDown();
  }

  TestSSLErrorHandler* error_handler() { return error_handler_.get(); }
  TestSSLErrorHandlerDelegate* delegate() { return delegate_; }

  network_time::FieldTrialTest* field_trial_test() {
    return field_trial_test_.get();
  }

  network_time::NetworkTimeTracker* tracker() { return tracker_.get(); }

  net::EmbeddedTestServer* test_server() { return test_server_.get(); }

  void ClearErrorHandler() { error_handler_.reset(nullptr); }

 private:
  net::SSLInfo ssl_info_;
  std::unique_ptr<TestSSLErrorHandler> error_handler_;
  TestSSLErrorHandlerDelegate* delegate_;

  std::unique_ptr<network_time::FieldTrialTest> field_trial_test_;
  base::SimpleTestClock* clock_;
  base::SimpleTestTickClock* tick_clock_;
  TestingPrefServiceSimple pref_service_;
  std::unique_ptr<network_time::NetworkTimeTracker> tracker_;
  std::unique_ptr<net::EmbeddedTestServer> test_server_;

  DISALLOW_COPY_AND_ASSIGN(SSLErrorHandlerDateInvalidTest);
};

#if BUILDFLAG(ENABLE_CAPTIVE_PORTAL_DETECTION)

TEST_F(SSLErrorHandlerNameMismatchTest,
       ShouldShowSSLInterstitialOnTimerExpired) {
  base::HistogramTester histograms;
  EXPECT_FALSE(error_handler()->IsTimerRunningForTesting());
  error_handler()->StartHandlingError();

  EXPECT_TRUE(error_handler()->IsTimerRunningForTesting());
  EXPECT_TRUE(delegate()->captive_portal_checked());
  EXPECT_FALSE(delegate()->ssl_interstitial_shown());
  EXPECT_FALSE(delegate()->captive_portal_interstitial_shown());

  delegate()->ClearSeenOperations();
  base::RunLoop().RunUntilIdle();

  EXPECT_FALSE(error_handler()->IsTimerRunningForTesting());
  EXPECT_FALSE(delegate()->captive_portal_checked());
  EXPECT_TRUE(delegate()->ssl_interstitial_shown());
  EXPECT_FALSE(delegate()->captive_portal_interstitial_shown());

  histograms.ExpectTotalCount(SSLErrorHandler::GetHistogramNameForTesting(), 2);
  histograms.ExpectBucketCount(SSLErrorHandler::GetHistogramNameForTesting(),
                               SSLErrorHandler::HANDLE_ALL, 1);
  histograms.ExpectBucketCount(
      SSLErrorHandler::GetHistogramNameForTesting(),
      SSLErrorHandler::SHOW_SSL_INTERSTITIAL_OVERRIDABLE, 1);
}

TEST_F(SSLErrorHandlerNameMismatchTest,
       ShouldShowCustomInterstitialOnCaptivePortalResult) {
  base::HistogramTester histograms;
  EXPECT_FALSE(error_handler()->IsTimerRunningForTesting());
  error_handler()->StartHandlingError();

  EXPECT_TRUE(error_handler()->IsTimerRunningForTesting());
  EXPECT_TRUE(delegate()->captive_portal_checked());
  EXPECT_FALSE(delegate()->ssl_interstitial_shown());
  EXPECT_FALSE(delegate()->captive_portal_interstitial_shown());
  // Fake a captive portal result.
  delegate()->ClearSeenOperations();
  delegate()->SendCaptivePortalNotification(
      captive_portal::RESULT_BEHIND_CAPTIVE_PORTAL);
  base::RunLoop().RunUntilIdle();

  EXPECT_FALSE(error_handler()->IsTimerRunningForTesting());
  EXPECT_FALSE(delegate()->captive_portal_checked());
  EXPECT_FALSE(delegate()->ssl_interstitial_shown());
  EXPECT_TRUE(delegate()->captive_portal_interstitial_shown());

  histograms.ExpectTotalCount(SSLErrorHandler::GetHistogramNameForTesting(), 2);
  histograms.ExpectBucketCount(SSLErrorHandler::GetHistogramNameForTesting(),
                               SSLErrorHandler::HANDLE_ALL, 1);
  histograms.ExpectBucketCount(
      SSLErrorHandler::GetHistogramNameForTesting(),
      SSLErrorHandler::SHOW_CAPTIVE_PORTAL_INTERSTITIAL_OVERRIDABLE, 1);
}

TEST_F(SSLErrorHandlerNameMismatchTest,
       ShouldShowSSLInterstitialOnNoCaptivePortalResult) {
  base::HistogramTester histograms;
  EXPECT_FALSE(error_handler()->IsTimerRunningForTesting());
  error_handler()->StartHandlingError();

  EXPECT_TRUE(error_handler()->IsTimerRunningForTesting());
  EXPECT_TRUE(delegate()->captive_portal_checked());
  EXPECT_FALSE(delegate()->ssl_interstitial_shown());
  EXPECT_FALSE(delegate()->captive_portal_interstitial_shown());
  // Fake a "connected to internet" result for the captive portal check.
  // This should immediately trigger an SSL interstitial without waiting for
  // the timer to expire.
  delegate()->ClearSeenOperations();
  delegate()->SendCaptivePortalNotification(
      captive_portal::RESULT_INTERNET_CONNECTED);
  base::RunLoop().RunUntilIdle();

  EXPECT_FALSE(error_handler()->IsTimerRunningForTesting());
  EXPECT_FALSE(delegate()->captive_portal_checked());
  EXPECT_TRUE(delegate()->ssl_interstitial_shown());
  EXPECT_FALSE(delegate()->captive_portal_interstitial_shown());

  histograms.ExpectTotalCount(SSLErrorHandler::GetHistogramNameForTesting(), 2);
  histograms.ExpectBucketCount(SSLErrorHandler::GetHistogramNameForTesting(),
                               SSLErrorHandler::HANDLE_ALL, 1);
  histograms.ExpectBucketCount(
      SSLErrorHandler::GetHistogramNameForTesting(),
      SSLErrorHandler::SHOW_SSL_INTERSTITIAL_OVERRIDABLE, 1);
}

TEST_F(SSLErrorHandlerNameMismatchTest,
       ShouldNotCheckSuggestedUrlIfNoSuggestedUrl) {
  base::HistogramTester histograms;
  error_handler()->StartHandlingError();

  EXPECT_TRUE(delegate()->captive_portal_checked());
  EXPECT_TRUE(error_handler()->IsTimerRunningForTesting());
  EXPECT_FALSE(delegate()->suggested_url_checked());
  base::RunLoop().RunUntilIdle();

  EXPECT_FALSE(error_handler()->IsTimerRunningForTesting());
  EXPECT_TRUE(delegate()->ssl_interstitial_shown());

  histograms.ExpectTotalCount(SSLErrorHandler::GetHistogramNameForTesting(), 2);
  histograms.ExpectBucketCount(SSLErrorHandler::GetHistogramNameForTesting(),
                               SSLErrorHandler::HANDLE_ALL, 1);
  histograms.ExpectBucketCount(
      SSLErrorHandler::GetHistogramNameForTesting(),
      SSLErrorHandler::SHOW_SSL_INTERSTITIAL_OVERRIDABLE, 1);
}

TEST_F(SSLErrorHandlerNameMismatchTest,
       ShouldNotCheckCaptivePortalIfSuggestedUrlExists) {
  base::HistogramTester histograms;
  EXPECT_FALSE(error_handler()->IsTimerRunningForTesting());
  delegate()->set_suggested_url_exists();
  error_handler()->StartHandlingError();

  EXPECT_TRUE(error_handler()->IsTimerRunningForTesting());
  EXPECT_TRUE(delegate()->suggested_url_checked());
  EXPECT_FALSE(delegate()->captive_portal_checked());
  base::RunLoop().RunUntilIdle();

  EXPECT_FALSE(error_handler()->IsTimerRunningForTesting());
  EXPECT_TRUE(delegate()->ssl_interstitial_shown());

  // Note that the suggested URL check is never completed, so there is no entry
  // for WWW_MISMATCH_URL_AVAILABLE or WWW_MISMATCH_URL_NOT_AVAILABLE.
  histograms.ExpectTotalCount(SSLErrorHandler::GetHistogramNameForTesting(), 3);
  histograms.ExpectBucketCount(SSLErrorHandler::GetHistogramNameForTesting(),
                               SSLErrorHandler::HANDLE_ALL, 1);
  histograms.ExpectBucketCount(SSLErrorHandler::GetHistogramNameForTesting(),
                               SSLErrorHandler::WWW_MISMATCH_FOUND_IN_SAN, 1);
  histograms.ExpectBucketCount(
      SSLErrorHandler::GetHistogramNameForTesting(),
      SSLErrorHandler::SHOW_SSL_INTERSTITIAL_OVERRIDABLE, 1);
}

TEST_F(SSLErrorHandlerNameMismatchTest,
       ShouldNotHandleNameMismatchOnNonOverridableError) {
  base::HistogramTester histograms;
  delegate()->set_non_overridable_error();
  delegate()->set_suggested_url_exists();
  error_handler()->StartHandlingError();

  EXPECT_FALSE(delegate()->suggested_url_checked());
  EXPECT_TRUE(delegate()->captive_portal_checked());
  EXPECT_TRUE(error_handler()->IsTimerRunningForTesting());
  base::RunLoop().RunUntilIdle();

  EXPECT_FALSE(error_handler()->IsTimerRunningForTesting());
  EXPECT_TRUE(delegate()->ssl_interstitial_shown());

  histograms.ExpectTotalCount(SSLErrorHandler::GetHistogramNameForTesting(), 2);
  histograms.ExpectBucketCount(SSLErrorHandler::GetHistogramNameForTesting(),
                               SSLErrorHandler::HANDLE_ALL, 1);
  histograms.ExpectBucketCount(
      SSLErrorHandler::GetHistogramNameForTesting(),
      SSLErrorHandler::SHOW_SSL_INTERSTITIAL_NONOVERRIDABLE, 1);
}

#else  // #if !BUILDFLAG(ENABLE_CAPTIVE_PORTAL_DETECTION)

TEST_F(SSLErrorHandlerNameMismatchTest,
       ShouldShowSSLInterstitialOnCaptivePortalDetectionDisabled) {
  base::HistogramTester histograms;
  EXPECT_FALSE(error_handler()->IsTimerRunningForTesting());
  error_handler()->StartHandlingError();
  EXPECT_FALSE(error_handler()->IsTimerRunningForTesting());
  EXPECT_FALSE(delegate()->captive_portal_checked());
  EXPECT_TRUE(delegate()->ssl_interstitial_shown());
  EXPECT_FALSE(delegate()->captive_portal_interstitial_shown());

  histograms.ExpectTotalCount(SSLErrorHandler::GetHistogramNameForTesting(), 2);
  histograms.ExpectBucketCount(SSLErrorHandler::GetHistogramNameForTesting(),
                               SSLErrorHandler::HANDLE_ALL, 1);
  histograms.ExpectBucketCount(
      SSLErrorHandler::GetHistogramNameForTesting(),
      SSLErrorHandler::SHOW_SSL_INTERSTITIAL_OVERRIDABLE, 1);
}

#endif  // BUILDFLAG(ENABLE_CAPTIVE_PORTAL_DETECTION)

TEST_F(SSLErrorHandlerNameMismatchTest,
       ShouldShowSSLInterstitialOnTimerExpiredWhenSuggestedUrlExists) {
  base::HistogramTester histograms;
  delegate()->set_suggested_url_exists();
  error_handler()->StartHandlingError();

  EXPECT_TRUE(error_handler()->IsTimerRunningForTesting());
  EXPECT_TRUE(delegate()->suggested_url_checked());
  EXPECT_FALSE(delegate()->ssl_interstitial_shown());
  EXPECT_FALSE(delegate()->redirected_to_suggested_url());

  base::RunLoop().RunUntilIdle();

  EXPECT_FALSE(error_handler()->IsTimerRunningForTesting());
  EXPECT_TRUE(delegate()->ssl_interstitial_shown());
  EXPECT_FALSE(delegate()->redirected_to_suggested_url());

  // Note that the suggested URL check is never completed, so there is no entry
  // for WWW_MISMATCH_URL_AVAILABLE or WWW_MISMATCH_URL_NOT_AVAILABLE.
  histograms.ExpectTotalCount(SSLErrorHandler::GetHistogramNameForTesting(), 3);
  histograms.ExpectBucketCount(SSLErrorHandler::GetHistogramNameForTesting(),
                               SSLErrorHandler::HANDLE_ALL, 1);
  histograms.ExpectBucketCount(SSLErrorHandler::GetHistogramNameForTesting(),
                               SSLErrorHandler::WWW_MISMATCH_FOUND_IN_SAN, 1);
  histograms.ExpectBucketCount(
      SSLErrorHandler::GetHistogramNameForTesting(),
      SSLErrorHandler::SHOW_SSL_INTERSTITIAL_OVERRIDABLE, 1);
}

TEST_F(SSLErrorHandlerNameMismatchTest,
       ShouldRedirectOnSuggestedUrlCheckResult) {
  base::HistogramTester histograms;
  delegate()->set_suggested_url_exists();
  error_handler()->StartHandlingError();

  EXPECT_TRUE(error_handler()->IsTimerRunningForTesting());
  EXPECT_TRUE(delegate()->suggested_url_checked());
  EXPECT_FALSE(delegate()->ssl_interstitial_shown());
  EXPECT_FALSE(delegate()->redirected_to_suggested_url());
  // Fake a valid suggested URL check result.
  // The URL returned by |SuggestedUrlCheckResult| can be different from
  // |suggested_url|, if there is a redirect.
  delegate()->SendSuggestedUrlCheckResult(
      CommonNameMismatchHandler::SuggestedUrlCheckResult::
          SUGGESTED_URL_AVAILABLE,
      GURL("https://random.example.com"));

  EXPECT_FALSE(error_handler()->IsTimerRunningForTesting());
  EXPECT_FALSE(delegate()->ssl_interstitial_shown());
  EXPECT_TRUE(delegate()->redirected_to_suggested_url());

  histograms.ExpectTotalCount(SSLErrorHandler::GetHistogramNameForTesting(), 3);
  histograms.ExpectBucketCount(SSLErrorHandler::GetHistogramNameForTesting(),
                               SSLErrorHandler::HANDLE_ALL, 1);
  histograms.ExpectBucketCount(SSLErrorHandler::GetHistogramNameForTesting(),
                               SSLErrorHandler::WWW_MISMATCH_FOUND_IN_SAN, 1);
  histograms.ExpectBucketCount(SSLErrorHandler::GetHistogramNameForTesting(),
                               SSLErrorHandler::WWW_MISMATCH_URL_AVAILABLE, 1);
}

// No suggestions should be requested if certificate lacks a SubjectAltName.
TEST_F(SSLErrorHandlerNameMismatchNoSANTest,
       SSLCommonNameMismatchHandlingRequiresSubjectAltName) {
  base::HistogramTester histograms;
  EXPECT_FALSE(error_handler()->IsTimerRunningForTesting());
  delegate()->set_suggested_url_exists();
  error_handler()->StartHandlingError();

  EXPECT_FALSE(delegate()->suggested_url_checked());
  base::RunLoop().RunUntilIdle();

  EXPECT_TRUE(delegate()->ssl_interstitial_shown());
  EXPECT_FALSE(delegate()->redirected_to_suggested_url());

  histograms.ExpectTotalCount(SSLErrorHandler::GetHistogramNameForTesting(), 2);
  histograms.ExpectBucketCount(SSLErrorHandler::GetHistogramNameForTesting(),
                               SSLErrorHandler::HANDLE_ALL, 1);
  histograms.ExpectBucketCount(SSLErrorHandler::GetHistogramNameForTesting(),
                               SSLErrorHandler::WWW_MISMATCH_FOUND_IN_SAN, 0);
  histograms.ExpectBucketCount(
      SSLErrorHandler::GetHistogramNameForTesting(),
      SSLErrorHandler::SHOW_SSL_INTERSTITIAL_OVERRIDABLE, 1);
}

TEST_F(SSLErrorHandlerNameMismatchTest,
       ShouldShowSSLInterstitialOnInvalidUrlCheckResult) {
  base::HistogramTester histograms;
  delegate()->set_suggested_url_exists();
  error_handler()->StartHandlingError();

  EXPECT_TRUE(error_handler()->IsTimerRunningForTesting());
  EXPECT_TRUE(delegate()->suggested_url_checked());
  EXPECT_FALSE(delegate()->ssl_interstitial_shown());
  EXPECT_FALSE(delegate()->redirected_to_suggested_url());
  // Fake an Invalid Suggested URL Check result.
  delegate()->SendSuggestedUrlCheckResult(
      CommonNameMismatchHandler::SuggestedUrlCheckResult::
          SUGGESTED_URL_NOT_AVAILABLE,
      GURL());

  EXPECT_FALSE(error_handler()->IsTimerRunningForTesting());
  EXPECT_TRUE(delegate()->ssl_interstitial_shown());
  EXPECT_FALSE(delegate()->redirected_to_suggested_url());

  histograms.ExpectTotalCount(SSLErrorHandler::GetHistogramNameForTesting(), 4);
  histograms.ExpectBucketCount(SSLErrorHandler::GetHistogramNameForTesting(),
                               SSLErrorHandler::HANDLE_ALL, 1);
  histograms.ExpectBucketCount(SSLErrorHandler::GetHistogramNameForTesting(),
                               SSLErrorHandler::WWW_MISMATCH_FOUND_IN_SAN, 1);
  histograms.ExpectBucketCount(SSLErrorHandler::GetHistogramNameForTesting(),
                               SSLErrorHandler::WWW_MISMATCH_URL_NOT_AVAILABLE,
                               1);
  histograms.ExpectBucketCount(
      SSLErrorHandler::GetHistogramNameForTesting(),
      SSLErrorHandler::SHOW_SSL_INTERSTITIAL_OVERRIDABLE, 1);
}

TEST_F(SSLErrorHandlerDateInvalidTest, TimeQueryStarted) {
  base::HistogramTester histograms;
  base::Time network_time;
  base::TimeDelta uncertainty;
  SSLErrorHandler::SetInterstitialDelayForTesting(
      base::TimeDelta::FromHours(1));
  EXPECT_EQ(network_time::NetworkTimeTracker::NETWORK_TIME_NO_SYNC_ATTEMPT,
            tracker()->GetNetworkTime(&network_time, &uncertainty));

  // Enable network time queries and handle the error. A bad clock interstitial
  // should be shown.
  test_server()->RegisterRequestHandler(
      base::Bind(&network_time::GoodTimeResponseHandler));
  EXPECT_TRUE(test_server()->Start());
  tracker()->SetTimeServerURLForTesting(test_server()->GetURL("/"));
  field_trial_test()->SetNetworkQueriesWithVariationsService(
      true, 0.0, network_time::NetworkTimeTracker::FETCHES_ON_DEMAND_ONLY);
  error_handler()->StartHandlingError();

  EXPECT_TRUE(error_handler()->IsTimerRunningForTesting());
  tracker()->WaitForFetchForTesting(123123123);
  base::RunLoop().RunUntilIdle();

  EXPECT_TRUE(delegate()->bad_clock_interstitial_shown());
  EXPECT_FALSE(error_handler()->IsTimerRunningForTesting());
  // Check that the histogram for the delay was recorded.
  histograms.ExpectTotalCount(kCertDateErrorHistogram, 1);
}

// Tests that an SSL interstitial is shown if the accuracy of the system
// clock can't be determined because network time is unavailable.
TEST_F(SSLErrorHandlerDateInvalidTest, NoTimeQueries) {
  base::HistogramTester histograms;
  base::Time network_time;
  base::TimeDelta uncertainty;
  EXPECT_EQ(network_time::NetworkTimeTracker::NETWORK_TIME_NO_SYNC_ATTEMPT,
            tracker()->GetNetworkTime(&network_time, &uncertainty));

  // Handle the error without enabling time queries. A bad clock interstitial
  // should not be shown.
  error_handler()->StartHandlingError();

  EXPECT_FALSE(error_handler()->IsTimerRunningForTesting());
  EXPECT_FALSE(delegate()->bad_clock_interstitial_shown());
  EXPECT_TRUE(delegate()->ssl_interstitial_shown());
  // Check that the histogram for the delay was recorded.
  histograms.ExpectTotalCount(kCertDateErrorHistogram, 1);
}

// Tests that an SSL interstitial is shown if determing the accuracy of
// the system clock times out (e.g. because a network time query hangs).
TEST_F(SSLErrorHandlerDateInvalidTest, TimeQueryHangs) {
  base::HistogramTester histograms;
  base::Time network_time;
  base::TimeDelta uncertainty;
  EXPECT_EQ(network_time::NetworkTimeTracker::NETWORK_TIME_NO_SYNC_ATTEMPT,
            tracker()->GetNetworkTime(&network_time, &uncertainty));

  // Enable network time queries and handle the error. Because the
  // network time cannot be determined before the timer elapses, an SSL
  // interstitial should be shown.
  base::RunLoop wait_for_time_query_loop;
  test_server()->RegisterRequestHandler(
      base::Bind(&WaitForRequest, wait_for_time_query_loop.QuitClosure()));
  EXPECT_TRUE(test_server()->Start());
  tracker()->SetTimeServerURLForTesting(test_server()->GetURL("/"));
  field_trial_test()->SetNetworkQueriesWithVariationsService(
      true, 0.0, network_time::NetworkTimeTracker::FETCHES_ON_DEMAND_ONLY);
  error_handler()->StartHandlingError();
  EXPECT_TRUE(error_handler()->IsTimerRunningForTesting());
  wait_for_time_query_loop.Run();
  base::RunLoop().RunUntilIdle();

  EXPECT_FALSE(delegate()->bad_clock_interstitial_shown());
  EXPECT_TRUE(delegate()->ssl_interstitial_shown());
  EXPECT_FALSE(error_handler()->IsTimerRunningForTesting());

  // Check that the histogram for the delay was recorded.
  histograms.ExpectTotalCount(kCertDateErrorHistogram, 1);

  // Clear the error handler to test that, when the request completes,
  // it doesn't try to call a callback on a deleted SSLErrorHandler.
  ClearErrorHandler();

  // Shut down the server to cancel the pending request.
  ASSERT_TRUE(test_server()->ShutdownAndWaitUntilComplete());
}

#if BUILDFLAG(ENABLE_CAPTIVE_PORTAL_DETECTION)

// Tests that a certificate marked as a known captive portal certificate causes
// the captive portal interstitial to be shown.
TEST_F(SSLErrorAssistantTest, TestCaptivePortalInterstitialWithFeatureEnabled) {
  SetCaptivePortalFeatureEnabled(true);

  EXPECT_FALSE(error_handler()->IsTimerRunningForTesting());
  EXPECT_EQ(1u, ssl_info().public_key_hashes.size());

  auto config_proto =
      base::MakeUnique<chrome_browser_ssl::SSLErrorAssistantConfig>();
  config_proto->add_captive_portal_cert()->set_sha256_hash(
      "sha256/aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa");
  config_proto->add_captive_portal_cert()->set_sha256_hash(
      ssl_info().public_key_hashes[0].ToString());
  config_proto->add_captive_portal_cert()->set_sha256_hash(
      "sha256/bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb");
  SSLErrorHandler::SetErrorAssistantProto(std::move(config_proto));

  base::HistogramTester histograms;
  error_handler()->StartHandlingError();

  // Timer shouldn't start for a known captive portal certificate.
  EXPECT_FALSE(error_handler()->IsTimerRunningForTesting());
  EXPECT_FALSE(delegate()->captive_portal_checked());
  EXPECT_FALSE(delegate()->ssl_interstitial_shown());
  EXPECT_TRUE(delegate()->captive_portal_interstitial_shown());
  EXPECT_FALSE(delegate()->suggested_url_checked());

  // A buggy SSL error handler might have incorrectly started the timer. Run
  // to completion to ensure the timer is expired.
  base::RunLoop().RunUntilIdle();

  EXPECT_FALSE(error_handler()->IsTimerRunningForTesting());
  EXPECT_FALSE(delegate()->captive_portal_checked());
  EXPECT_FALSE(delegate()->ssl_interstitial_shown());
  EXPECT_TRUE(delegate()->captive_portal_interstitial_shown());
  EXPECT_FALSE(delegate()->suggested_url_checked());

  // Check that the histogram for the captive portal cert was recorded.
  histograms.ExpectTotalCount(SSLErrorHandler::GetHistogramNameForTesting(), 3);
  histograms.ExpectBucketCount(SSLErrorHandler::GetHistogramNameForTesting(),
                               SSLErrorHandler::HANDLE_ALL, 1);
  histograms.ExpectBucketCount(
      SSLErrorHandler::GetHistogramNameForTesting(),
      SSLErrorHandler::SHOW_CAPTIVE_PORTAL_INTERSTITIAL_OVERRIDABLE, 1);
  histograms.ExpectBucketCount(SSLErrorHandler::GetHistogramNameForTesting(),
                               SSLErrorHandler::CAPTIVE_PORTAL_CERT_FOUND, 1);
}

// Tests that a certificate marked as a known captive portal certificate does
// not cause the captive portal interstitial to be shown, if the feature is
// disabled.
TEST_F(SSLErrorAssistantTest,
       TestNoCaptivePortalInterstitialWithFeatureDisabled) {
  SetCaptivePortalFeatureEnabled(false);

  // Default error for SSLErrorHandlerNameMismatchTest tests is name mismatch.
  TestNoCaptivePortalInterstitial();
}

// Tests that an error other than name mismatch does not cause a captive portal
// interstitial to be shown, even if the certificate is marked as a known
// captive portal certificate.
TEST_F(SSLErrorAssistantTest,
       TestNoCaptivePortalInterstitialWithAuthorityInvalidError) {
  SetCaptivePortalFeatureEnabled(true);

  ResetErrorHandlerForCaptivePortalTest(net::CERT_STATUS_AUTHORITY_INVALID);
  TestNoCaptivePortalInterstitial();
}

// Tests that an authority invalid error in addition to name mismatch error does
// not cause a captive portal interstitial to be shown, even if the certificate
// is marked as a known captive portal certificate. The resulting error is
// authority-invalid.
TEST_F(
    SSLErrorAssistantTest,
    TestNoCaptivePortalInterstitialWithNameMismatchAndAuthorityInvalidErrors) {
  SetCaptivePortalFeatureEnabled(true);

  const net::CertStatus cert_status =
      net::CERT_STATUS_COMMON_NAME_INVALID | net::CERT_STATUS_AUTHORITY_INVALID;
  // Sanity check that AUTHORITY_INVALID is seen as the net error.
  ASSERT_EQ(net::ERR_CERT_AUTHORITY_INVALID,
            net::MapCertStatusToNetError(cert_status));
  ResetErrorHandlerForCaptivePortalTest(cert_status);
  TestNoCaptivePortalInterstitial();
}

// Tests that another error in addition to name mismatch error does not cause a
// captive portal interstitial to be shown, even if the certificate is marked as
// a known captive portal certificate. Similar to
// NameMismatchAndAuthorityInvalid, except the resulting error is name mismatch.
TEST_F(SSLErrorAssistantTest,
       TestNoCaptivePortalInterstitialWithNameMismatchAndWeakKeyErrors) {
  SetCaptivePortalFeatureEnabled(true);

  const net::CertStatus cert_status =
      net::CERT_STATUS_COMMON_NAME_INVALID | net::CERT_STATUS_WEAK_KEY;
  // Sanity check that COMMON_NAME_INVALID is seen as the net error, since the
  // test is designed to verify that SSLErrorHandler notices other errors in the
  // CertStatus even when COMMON_NAME_INVALID is the net error.
  ASSERT_EQ(net::ERR_CERT_COMMON_NAME_INVALID,
            net::MapCertStatusToNetError(cert_status));
  ResetErrorHandlerForCaptivePortalTest(cert_status);
  TestNoCaptivePortalInterstitial();
}

#else

TEST_F(SSLErrorAssistantTest,
       TestGenericSSLInterstitialWhenCaptivePortalDisabledByBuild) {
  SetCaptivePortalFeatureEnabled(true);

  // Default error for SSLErrorHandlerNameMismatchTest tests is name mismatch,
  // but the feature is disabled by build so a generic SSL interstitial will be
  // displayed.
  base::HistogramTester histograms;

  EXPECT_FALSE(error_handler()->IsTimerRunningForTesting());
  EXPECT_EQ(1u, ssl_info().public_key_hashes.size());

  auto config_proto =
      base::MakeUnique<chrome_browser_ssl::SSLErrorAssistantConfig>();
  config_proto->add_captive_portal_cert()->set_sha256_hash(
      "sha256/aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa");
  config_proto->add_captive_portal_cert()->set_sha256_hash(
      ssl_info().public_key_hashes[0].ToString());
  config_proto->add_captive_portal_cert()->set_sha256_hash(
      "sha256/bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb");
  SSLErrorHandler::SetErrorAssistantProto(std::move(config_proto));

  error_handler()->StartHandlingError();

  EXPECT_FALSE(error_handler()->IsTimerRunningForTesting());
  EXPECT_FALSE(delegate()->captive_portal_checked());
  EXPECT_TRUE(delegate()->ssl_interstitial_shown());
  EXPECT_FALSE(delegate()->captive_portal_interstitial_shown());
  EXPECT_FALSE(delegate()->suggested_url_checked());

  base::RunLoop().RunUntilIdle();

  EXPECT_FALSE(error_handler()->IsTimerRunningForTesting());
  EXPECT_FALSE(delegate()->captive_portal_checked());
  EXPECT_TRUE(delegate()->ssl_interstitial_shown());
  EXPECT_FALSE(delegate()->captive_portal_interstitial_shown());
  EXPECT_FALSE(delegate()->suggested_url_checked());

  histograms.ExpectTotalCount(SSLErrorHandler::GetHistogramNameForTesting(), 2);
  histograms.ExpectBucketCount(SSLErrorHandler::GetHistogramNameForTesting(),
                               SSLErrorHandler::HANDLE_ALL, 1);
  histograms.ExpectBucketCount(
      SSLErrorHandler::GetHistogramNameForTesting(),
      SSLErrorHandler::SHOW_SSL_INTERSTITIAL_OVERRIDABLE, 1);
}

#endif  // BUILDFLAG(ENABLE_CAPTIVE_PORTAL_DETECTION)

TEST_F(SSLErrorAssistantTest, TestContentFilterTriggerOnMatchingCertificate) {
  base::HistogramTester histograms;

  ResetErrorHandler(std::string(), kMcAfeeCert,
                    net::CERT_STATUS_AUTHORITY_INVALID);
  delegate()->set_non_overridable_error();
  RunContentFilterTest();

  EXPECT_FALSE(delegate()->ssl_interstitial_shown());
  EXPECT_TRUE(delegate()->content_filter_interstitial_shown());
  EXPECT_FALSE(delegate()->suggested_url_checked());

  ResetErrorHandler(std::string(), kCyberoamCert,
                    net::CERT_STATUS_AUTHORITY_INVALID);
  delegate()->set_non_overridable_error();
  RunContentFilterTest();

  EXPECT_FALSE(delegate()->ssl_interstitial_shown());
  EXPECT_TRUE(delegate()->content_filter_interstitial_shown());
  EXPECT_FALSE(delegate()->suggested_url_checked());

  histograms.ExpectTotalCount(SSLErrorHandler::GetHistogramNameForTesting(), 6);
  histograms.ExpectBucketCount(SSLErrorHandler::GetHistogramNameForTesting(),
                               SSLErrorHandler::HANDLE_ALL, 2);
  histograms.ExpectBucketCount(
      SSLErrorHandler::GetHistogramNameForTesting(),
      SSLErrorHandler::SHOW_SSL_INTERSTITIAL_NONOVERRIDABLE, 0);
  histograms.ExpectBucketCount(
      SSLErrorHandler::GetHistogramNameForTesting(),
      SSLErrorHandler::SHOW_SSL_INTERSTITIAL_OVERRIDABLE, 0);
  histograms.ExpectBucketCount(SSLErrorHandler::GetHistogramNameForTesting(),
                               SSLErrorHandler::CONTENT_FILTER_CERT_FOUND, 2);
  histograms.ExpectBucketCount(
      SSLErrorHandler::GetHistogramNameForTesting(),
      SSLErrorHandler::SHOW_CONTENT_FILTER_INTERSTITIAL, 2);
}

TEST_F(SSLErrorAssistantTest,
       TestNoContentFilterTriggerOnMiscellaneousCertificate) {
  base::HistogramTester histograms;

  ResetErrorHandler(kOkayCertName, std::string(),
                    net::CERT_STATUS_AUTHORITY_INVALID);
  delegate()->set_non_overridable_error();
  RunContentFilterTest();

  EXPECT_TRUE(delegate()->ssl_interstitial_shown());
  EXPECT_FALSE(delegate()->content_filter_interstitial_shown());
  EXPECT_FALSE(delegate()->suggested_url_checked());

  histograms.ExpectTotalCount(SSLErrorHandler::GetHistogramNameForTesting(), 2);
  histograms.ExpectBucketCount(SSLErrorHandler::GetHistogramNameForTesting(),
                               SSLErrorHandler::HANDLE_ALL, 1);
  histograms.ExpectBucketCount(
      SSLErrorHandler::GetHistogramNameForTesting(),
      SSLErrorHandler::SHOW_SSL_INTERSTITIAL_NONOVERRIDABLE, 1);
  histograms.ExpectBucketCount(SSLErrorHandler::GetHistogramNameForTesting(),
                               SSLErrorHandler::CONTENT_FILTER_CERT_FOUND, 0);
  histograms.ExpectBucketCount(
      SSLErrorHandler::GetHistogramNameForTesting(),
      SSLErrorHandler::SHOW_CONTENT_FILTER_INTERSTITIAL, 0);
}

TEST_F(SSLErrorAssistantTest,
       TestNoContentFilterInterstitialWithoutAuthorityInvalidError) {
  base::HistogramTester histograms;

  ResetErrorHandler(std::string(), kMcAfeeCert,
                    net::CERT_STATUS_COMMON_NAME_INVALID);
  delegate()->set_non_overridable_error();
  RunContentFilterTest();

  EXPECT_TRUE(delegate()->ssl_interstitial_shown());
  EXPECT_FALSE(delegate()->content_filter_interstitial_shown());
  EXPECT_FALSE(delegate()->suggested_url_checked());

  ResetErrorHandler(std::string(), kCyberoamCert,
                    net::CERT_STATUS_COMMON_NAME_INVALID);
  delegate()->set_non_overridable_error();
  RunContentFilterTest();

  EXPECT_TRUE(delegate()->ssl_interstitial_shown());
  EXPECT_FALSE(delegate()->content_filter_interstitial_shown());
  EXPECT_FALSE(delegate()->suggested_url_checked());

  histograms.ExpectTotalCount(SSLErrorHandler::GetHistogramNameForTesting(), 4);
  histograms.ExpectBucketCount(SSLErrorHandler::GetHistogramNameForTesting(),
                               SSLErrorHandler::HANDLE_ALL, 2);
  histograms.ExpectBucketCount(
      SSLErrorHandler::GetHistogramNameForTesting(),
      SSLErrorHandler::SHOW_SSL_INTERSTITIAL_NONOVERRIDABLE, 2);
  histograms.ExpectBucketCount(SSLErrorHandler::GetHistogramNameForTesting(),
                               SSLErrorHandler::CONTENT_FILTER_CERT_FOUND, 0);
  histograms.ExpectBucketCount(
      SSLErrorHandler::GetHistogramNameForTesting(),
      SSLErrorHandler::SHOW_CONTENT_FILTER_INTERSTITIAL, 0);
}

TEST_F(SSLErrorAssistantTest, TestNoContentFilterInterstitialWithTwoErrors) {
  base::HistogramTester histograms;

  ResetErrorHandler(std::string(), kMcAfeeCert,
                    net::CERT_STATUS_AUTHORITY_INVALID |
                        net::CERT_STATUS_COMMON_NAME_INVALID);
  delegate()->set_non_overridable_error();
  RunContentFilterTest();

  EXPECT_TRUE(delegate()->ssl_interstitial_shown());
  EXPECT_FALSE(delegate()->content_filter_interstitial_shown());
  EXPECT_FALSE(delegate()->suggested_url_checked());

  ResetErrorHandler(std::string(), kCyberoamCert,
                    net::CERT_STATUS_COMMON_NAME_INVALID);
  delegate()->set_non_overridable_error();
  RunContentFilterTest();

  EXPECT_TRUE(delegate()->ssl_interstitial_shown());
  EXPECT_FALSE(delegate()->content_filter_interstitial_shown());
  EXPECT_FALSE(delegate()->suggested_url_checked());

  histograms.ExpectTotalCount(SSLErrorHandler::GetHistogramNameForTesting(), 4);
  histograms.ExpectBucketCount(SSLErrorHandler::GetHistogramNameForTesting(),
                               SSLErrorHandler::HANDLE_ALL, 2);
  histograms.ExpectBucketCount(
      SSLErrorHandler::GetHistogramNameForTesting(),
      SSLErrorHandler::SHOW_SSL_INTERSTITIAL_NONOVERRIDABLE, 2);
  histograms.ExpectBucketCount(SSLErrorHandler::GetHistogramNameForTesting(),
                               SSLErrorHandler::CONTENT_FILTER_CERT_FOUND, 0);
  histograms.ExpectBucketCount(
      SSLErrorHandler::GetHistogramNameForTesting(),
      SSLErrorHandler::SHOW_CONTENT_FILTER_INTERSTITIAL, 0);
}

TEST_F(SSLErrorAssistantTest, TestNoContentFilterInterstitialIfOverridable) {
  base::HistogramTester histograms;

  // The errors thrown in tests are overridable by default, so we don't
  // have to change anything here.
  ResetErrorHandler(std::string(), kMcAfeeCert,
                    net::CERT_STATUS_AUTHORITY_INVALID);
  RunContentFilterTest();

  EXPECT_TRUE(delegate()->ssl_interstitial_shown());
  EXPECT_FALSE(delegate()->content_filter_interstitial_shown());
  EXPECT_FALSE(delegate()->suggested_url_checked());

  histograms.ExpectTotalCount(SSLErrorHandler::GetHistogramNameForTesting(), 2);
  histograms.ExpectBucketCount(SSLErrorHandler::GetHistogramNameForTesting(),
                               SSLErrorHandler::HANDLE_ALL, 1);
  histograms.ExpectBucketCount(
      SSLErrorHandler::GetHistogramNameForTesting(),
      SSLErrorHandler::SHOW_SSL_INTERSTITIAL_NONOVERRIDABLE, 0);
  histograms.ExpectBucketCount(
      SSLErrorHandler::GetHistogramNameForTesting(),
      SSLErrorHandler::SHOW_SSL_INTERSTITIAL_OVERRIDABLE, 1);
  histograms.ExpectBucketCount(SSLErrorHandler::GetHistogramNameForTesting(),
                               SSLErrorHandler::CONTENT_FILTER_CERT_FOUND, 0);
  histograms.ExpectBucketCount(
      SSLErrorHandler::GetHistogramNameForTesting(),
      SSLErrorHandler::SHOW_CONTENT_FILTER_INTERSTITIAL, 0);
}
