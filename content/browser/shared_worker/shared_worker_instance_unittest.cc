// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/shared_worker/shared_worker_instance.h"

#include <memory>

#include "base/macros.h"
#include "base/strings/string16.h"
#include "base/strings/utf_string_conversions.h"
#include "content/browser/shared_worker/worker_storage_partition.h"
#include "content/public/browser/storage_partition.h"
#include "content/public/test/test_browser_context.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace content {

class SharedWorkerInstanceTest : public testing::Test {
 protected:
  SharedWorkerInstanceTest()
      : browser_context_(new TestBrowserContext()),
        partition_(new WorkerStoragePartition(
            BrowserContext::GetDefaultStoragePartition(browser_context_.get())
                ->GetURLRequestContext(),
            nullptr /* media_url_request_context */,
            nullptr /* appcache_service */,
            nullptr /* quota_manager */,
            nullptr /* filesystem_context */,
            nullptr /* database_tracker */,
            nullptr /* indexed_db_context */,
            nullptr /* service_worker_context */)),
        partition_id_(*partition_.get()) {}

  bool Matches(const SharedWorkerInstance& instance,
               const std::string& url,
               const base::StringPiece& name) {
    url::Origin constructor_origin;
    if (GURL(url).SchemeIs(url::kDataScheme))
      constructor_origin = url::Origin::Create(GURL("http://example.com/"));
    else
      constructor_origin = url::Origin::Create(GURL(url));
    return instance.Matches(GURL(url), name.as_string(), constructor_origin,
                            partition_id_,
                            browser_context_->GetResourceContext());
  }

  TestBrowserThreadBundle thread_bundle_;
  std::unique_ptr<TestBrowserContext> browser_context_;
  std::unique_ptr<WorkerStoragePartition> partition_;
  const WorkerStoragePartitionId partition_id_;

 private:
  DISALLOW_COPY_AND_ASSIGN(SharedWorkerInstanceTest);
};

TEST_F(SharedWorkerInstanceTest, MatchesTest) {
  // Hello!
  const std::string kDataURL("data:text/javascript;base64,Ly8gSGVsbG8h");

  SharedWorkerInstance instance1(
      GURL("http://example.com/w.js"), std::string(),
      url::Origin::Create(GURL("http://example.com/")), std::string(),
      blink::kWebContentSecurityPolicyTypeReport, blink::kWebAddressSpacePublic,
      browser_context_->GetResourceContext(), partition_id_,
      blink::mojom::SharedWorkerCreationContextType::kNonsecure,
      base::UnguessableToken::Create());
  EXPECT_TRUE(Matches(instance1, "http://example.com/w.js", ""));
  EXPECT_FALSE(Matches(instance1, "http://example.com/w2.js", ""));
  EXPECT_FALSE(Matches(instance1, "http://example.net/w.js", ""));
  EXPECT_FALSE(Matches(instance1, "http://example.net/w2.js", ""));
  EXPECT_FALSE(Matches(instance1, "http://example.com/w.js", "name"));
  EXPECT_FALSE(Matches(instance1, "http://example.com/w2.js", "name"));
  EXPECT_FALSE(Matches(instance1, "http://example.net/w.js", "name"));
  EXPECT_FALSE(Matches(instance1, "http://example.net/w2.js", "name"));
  EXPECT_FALSE(Matches(instance1, kDataURL, ""));
  EXPECT_FALSE(Matches(instance1, kDataURL, "name"));

  SharedWorkerInstance instance2(
      GURL("http://example.com/w.js"), "name",
      url::Origin::Create(GURL("http://example.com/")), std::string(),
      blink::kWebContentSecurityPolicyTypeReport, blink::kWebAddressSpacePublic,
      browser_context_->GetResourceContext(), partition_id_,
      blink::mojom::SharedWorkerCreationContextType::kNonsecure,
      base::UnguessableToken::Create());
  EXPECT_FALSE(Matches(instance2, "http://example.com/w.js", ""));
  EXPECT_FALSE(Matches(instance2, "http://example.com/w2.js", ""));
  EXPECT_FALSE(Matches(instance2, "http://example.net/w.js", ""));
  EXPECT_FALSE(Matches(instance2, "http://example.net/w2.js", ""));
  EXPECT_TRUE(Matches(instance2, "http://example.com/w.js", "name"));
  EXPECT_FALSE(Matches(instance2, "http://example.com/w2.js", "name"));
  EXPECT_FALSE(Matches(instance2, "http://example.net/w.js", "name"));
  EXPECT_FALSE(Matches(instance2, "http://example.net/w2.js", "name"));
  EXPECT_FALSE(Matches(instance2, "http://example.com/w.js", "name2"));
  EXPECT_FALSE(Matches(instance2, "http://example.com/w2.js", "name2"));
  EXPECT_FALSE(Matches(instance2, "http://example.net/w.js", "name2"));
  EXPECT_FALSE(Matches(instance2, "http://example.net/w2.js", "name2"));
  EXPECT_FALSE(Matches(instance2, kDataURL, ""));
  EXPECT_FALSE(Matches(instance2, kDataURL, "name"));

  SharedWorkerInstance instance3(
      GURL(kDataURL), std::string(),
      url::Origin::Create(GURL("http://example.com/")), std::string(),
      blink::kWebContentSecurityPolicyTypeReport, blink::kWebAddressSpacePublic,
      browser_context_->GetResourceContext(), partition_id_,
      blink::mojom::SharedWorkerCreationContextType::kNonsecure,
      base::UnguessableToken::Create());
  EXPECT_FALSE(Matches(instance3, "http://example.com/w.js", ""));
  EXPECT_FALSE(Matches(instance3, "http://example.com/w2.js", ""));
  EXPECT_FALSE(Matches(instance3, "http://example.net/w.js", ""));
  EXPECT_FALSE(Matches(instance3, "http://example.net/w2.js", ""));
  EXPECT_FALSE(Matches(instance3, "http://example.com/w.js", "name"));
  EXPECT_FALSE(Matches(instance3, "http://example.com/w2.js", "name"));
  EXPECT_FALSE(Matches(instance3, "http://example.net/w.js", "name"));
  EXPECT_FALSE(Matches(instance3, "http://example.net/w2.js", "name"));
  EXPECT_FALSE(Matches(instance3, "http://example.com/w.js", "name2"));
  EXPECT_FALSE(Matches(instance3, "http://example.com/w2.js", "name2"));
  EXPECT_FALSE(Matches(instance3, "http://example.net/w.js", "name2"));
  EXPECT_FALSE(Matches(instance3, "http://example.net/w2.js", "name2"));
  // The data: URL should match because the instance has the same data: URL,
  // name, and constructor origin.
  EXPECT_TRUE(Matches(instance3, kDataURL, ""));
  EXPECT_FALSE(Matches(instance3, kDataURL, "name"));

  SharedWorkerInstance instance4(
      GURL(kDataURL), "name", url::Origin::Create(GURL("http://example.com/")),
      std::string(), blink::kWebContentSecurityPolicyTypeReport,
      blink::kWebAddressSpacePublic, browser_context_->GetResourceContext(),
      partition_id_, blink::mojom::SharedWorkerCreationContextType::kNonsecure,
      base::UnguessableToken::Create());
  EXPECT_FALSE(Matches(instance4, "http://example.com/w.js", ""));
  EXPECT_FALSE(Matches(instance4, "http://example.com/w2.js", ""));
  EXPECT_FALSE(Matches(instance4, "http://example.net/w.js", ""));
  EXPECT_FALSE(Matches(instance4, "http://example.net/w2.js", ""));
  EXPECT_FALSE(Matches(instance4, "http://example.com/w.js", "name"));
  EXPECT_FALSE(Matches(instance4, "http://example.com/w2.js", "name"));
  EXPECT_FALSE(Matches(instance4, "http://example.net/w.js", "name"));
  EXPECT_FALSE(Matches(instance4, "http://example.net/w2.js", "name"));
  EXPECT_FALSE(Matches(instance4, "http://example.com/w.js", "name2"));
  EXPECT_FALSE(Matches(instance4, "http://example.com/w2.js", "name2"));
  EXPECT_FALSE(Matches(instance4, "http://example.net/w.js", "name2"));
  EXPECT_FALSE(Matches(instance4, "http://example.net/w2.js", "name2"));
  EXPECT_FALSE(Matches(instance4, kDataURL, ""));
  // The data: URL should match because the instance has the same data: URL,
  // name, and constructor origin.
  EXPECT_TRUE(Matches(instance4, kDataURL, "name"));

  SharedWorkerInstance instance5(
      GURL(kDataURL), std::string(),
      url::Origin::Create(GURL("http://example.net/")), std::string(),
      blink::kWebContentSecurityPolicyTypeReport, blink::kWebAddressSpacePublic,
      browser_context_->GetResourceContext(), partition_id_,
      blink::mojom::SharedWorkerCreationContextType::kNonsecure,
      base::UnguessableToken::Create());
  EXPECT_FALSE(Matches(instance5, "http://example.com/w.js", ""));
  EXPECT_FALSE(Matches(instance5, "http://example.com/w2.js", ""));
  EXPECT_FALSE(Matches(instance5, "http://example.net/w.js", ""));
  EXPECT_FALSE(Matches(instance5, "http://example.net/w2.js", ""));
  EXPECT_FALSE(Matches(instance5, "http://example.com/w.js", "name"));
  EXPECT_FALSE(Matches(instance5, "http://example.com/w2.js", "name"));
  EXPECT_FALSE(Matches(instance5, "http://example.net/w.js", "name"));
  EXPECT_FALSE(Matches(instance5, "http://example.net/w2.js", "name"));
  EXPECT_FALSE(Matches(instance5, "http://example.com/w.js", "name2"));
  EXPECT_FALSE(Matches(instance5, "http://example.com/w2.js", "name2"));
  EXPECT_FALSE(Matches(instance5, "http://example.net/w.js", "name2"));
  EXPECT_FALSE(Matches(instance5, "http://example.net/w2.js", "name2"));
  // The data: URL should not match because the instance has a different
  // constructor origin.
  EXPECT_FALSE(Matches(instance5, kDataURL, ""));
  EXPECT_FALSE(Matches(instance5, kDataURL, "name"));
}

TEST_F(SharedWorkerInstanceTest, AddressSpace) {
  for (int i = 0; i < static_cast<int>(blink::kWebAddressSpaceLast); i++) {
    SharedWorkerInstance instance(
        GURL("http://example.com/w.js"), "name",
        url::Origin::Create(GURL("http://example.com/")), std::string(),
        blink::kWebContentSecurityPolicyTypeReport,
        static_cast<blink::WebAddressSpace>(i),
        browser_context_->GetResourceContext(), partition_id_,
        blink::mojom::SharedWorkerCreationContextType::kNonsecure,
        base::UnguessableToken::Create());
    EXPECT_EQ(static_cast<blink::WebAddressSpace>(i),
              instance.creation_address_space());
  }
}

}  // namespace content
