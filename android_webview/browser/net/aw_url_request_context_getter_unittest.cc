// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "android_webview/browser/net/aw_url_request_context_getter.h"

#include <memory>

#include "base/android/jni_android.h"
#include "base/files/scoped_temp_dir.h"
#include "base/memory/ref_counted.h"
#include "components/prefs/testing_pref_service.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "net/proxy/proxy_config_service.h"
#include "net/proxy/proxy_service.h"
#include "net/ssl/ssl_config.h"
#include "net/ssl/ssl_config_service.h"
#include "net/url_request/url_request_context.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {

class AwURLRequestContextGetterTest : public ::testing::Test {
 public:
  AwURLRequestContextGetterTest() = default;

 protected:
  void SetUp() override {
    ASSERT_TRUE(temp_dir_.CreateUniqueTempDir());
    env_ = base::android::AttachCurrentThread();
    ASSERT_TRUE(env_);

    pref_service_ = std::make_unique<TestingPrefServiceSimple>();
    getter_ = base::MakeRefCounted<android_webview::AwURLRequestContextGetter>(
        temp_dir_.GetPath(),
        net::ProxyService::CreateSystemProxyConfigService(
            content::BrowserThread::GetTaskRunnerForThread(
                content::BrowserThread::IO)),
        pref_service_.get());
  }

  content::TestBrowserThreadBundle thread_bundle_;
  base::ScopedTempDir temp_dir_;
  JNIEnv* env_;
  std::unique_ptr<PrefService> pref_service_;
  scoped_refptr<net::URLRequestContextGetter> getter_;
};

// Tests that constraints on trust for Symantec-issued certificates are not
// enforced for the AwURLRequestContext(Getter), as it should behave like
// the Android system.
TEST_F(AwURLRequestContextGetterTest, SymantecPoliciesExempted) {
  net::URLRequestContext* context = getter_->GetURLRequestContext();
  net::SSLConfigService* config_service = context->ssl_config_service();

  net::SSLConfig config;
  EXPECT_FALSE(config.symantec_enforcement_disabled);
  config_service->GetSSLConfig(&config);
  EXPECT_TRUE(config.symantec_enforcement_disabled);
}

}  // namespace
