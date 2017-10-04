// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/origin_manifest/origin_manifest_store_impl.h"
#include "base/run_loop.h"
#include "base/test/scoped_task_environment.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"
#include "url/origin.h"

namespace content {

class OriginManifestStoreImplTest : public testing::Test {
  base::test::ScopedTaskEnvironment scoped_task_environment_;
};

// Note that we do not initialize the persistent store here since that
// is subject to testing the persistent store itself.

TEST_F(OriginManifestStoreImplTest, QueryOriginManifest) {
  OriginManifestStoreImpl store;
  blink::mojom::OriginManifestPtr om = blink::mojom::OriginManifest::New();
  url::Origin origin(GURL("https://a.com"));
  om->origin = origin;
  om->version = "version1";

  EXPECT_TRUE(store.Get(origin).Equals(nullptr));

  // Add origin manifest
  base::RunLoop run_loop;
  store.Add(om.Clone(), run_loop.QuitClosure());
  run_loop.Run();

  // get with same origin -> the equivalent OM
  blink::mojom::OriginManifestPtr om2 = store.Get(origin);
  EXPECT_FALSE(om2.Equals(nullptr));
  EXPECT_TRUE(om->version == om2->version);

  // get with different origin -> nothing
  om2 = store.Get(url::Origin(GURL("https://b.com")));
  EXPECT_TRUE(om2.Equals(nullptr));

  // remove origin
  base::RunLoop run_loop2;
  store.Remove(origin, run_loop2.QuitClosure());
  run_loop2.Run();

  // get with same origin -> nothing
  om2 = store.Get(origin);
  EXPECT_TRUE(om2.Equals(nullptr));
}

namespace {

void CheckEmptyCORSPreflights(base::Closure quit_closure,
                              blink::mojom::CORSPreflightPtr corspreflights) {
  EXPECT_TRUE(corspreflights.Equals(nullptr));
  std::move(quit_closure).Run();
}

void CheckHasCORSPreflights(base::Closure quit_closure,
                            blink::mojom::CORSPreflightPtr corspreflights) {
  EXPECT_FALSE(corspreflights.Equals(nullptr));

  EXPECT_EQ(corspreflights->nocredentials->origins.size(), 1ul);
  EXPECT_TRUE(corspreflights->nocredentials->origins[0].IsSameOriginWith(
      url::Origin(GURL("https://a.com"))));

  EXPECT_EQ(corspreflights->withcredentials->origins.size(), 1ul);
  EXPECT_TRUE(corspreflights->withcredentials->origins[0].IsSameOriginWith(
      url::Origin(GURL("https://b.com"))));

  std::move(quit_closure).Run();
}

}  // namespace

TEST_F(OriginManifestStoreImplTest, QueryCORSPreflight) {
  OriginManifestStoreImpl store;
  blink::mojom::OriginManifestPtr om = blink::mojom::OriginManifest::New();
  url::Origin origin(GURL("https://a.com"));
  om->origin = origin;
  std::vector<url::Origin> nocredentials;
  nocredentials.push_back(url::Origin(GURL("https://a.com")));
  std::vector<url::Origin> withcredentials;
  withcredentials.push_back(url::Origin(GURL("https://b.com")));
  om->corspreflights = blink::mojom::CORSPreflight::New(
      blink::mojom::CORSOrigins::New(false, nocredentials),
      blink::mojom::CORSOrigins::New(false, withcredentials));

  // get on empty store -> nothing
  base::RunLoop run_loop;
  store.GetCORSPreflight(origin, base::BindOnce(&CheckEmptyCORSPreflights,
                                                run_loop.QuitClosure()));
  run_loop.Run();

  // Add origin manifest
  base::RunLoop run_loop2;
  store.Add(om.Clone(), run_loop2.QuitClosure());
  run_loop2.Run();

  // get with same origin -> the OM (don't test for object equality, but
  // equivalence)
  base::RunLoop run_loop3;
  store.GetCORSPreflight(
      origin, base::BindOnce(&CheckHasCORSPreflights, run_loop3.QuitClosure()));
  run_loop3.Run();

  // get with different origin -> nothing
  base::RunLoop run_loop4;
  store.GetCORSPreflight(
      url::Origin(GURL("https://b.com")),
      base::BindOnce(&CheckEmptyCORSPreflights, run_loop4.QuitClosure()));
  run_loop4.Run();

  // remove origin
  base::RunLoop run_loop5;
  store.Remove(origin, run_loop5.QuitClosure());
  run_loop5.Run();

  // get with same origin -> nothing
  base::RunLoop run_loop6;
  store.GetCORSPreflight(origin, base::BindOnce(&CheckEmptyCORSPreflights,
                                                run_loop6.QuitClosure()));
  run_loop6.Run();
}

namespace {

void CheckEmptyContentSecurityPolicies(
    base::Closure quit_closure,
    std::vector<blink::mojom::ContentSecurityPolicyPtr> csps) {
  ASSERT_EQ(csps.size(), 0ul);
  std::move(quit_closure).Run();
}

void CheckHasContentSecurityPolicies(
    base::Closure quit_closure,
    std::vector<blink::mojom::ContentSecurityPolicyPtr> csps) {
  ASSERT_EQ(csps.size(), 1ul);

  EXPECT_TRUE(csps[0]->policy == "default-src 'none'");
  EXPECT_EQ(
      csps[0]->disposition,
      blink::WebContentSecurityPolicyType::kWebContentSecurityPolicyTypeReport);
  EXPECT_TRUE(csps[0]->allowOverride);

  std::move(quit_closure).Run();
}

}  // namespace

TEST_F(OriginManifestStoreImplTest, QueryContentSecurityPolicies) {
  OriginManifestStoreImpl store;
  blink::mojom::OriginManifestPtr om = blink::mojom::OriginManifest::New();
  url::Origin origin(GURL("https://a.com"));
  om->origin = origin;
  om->csps.push_back(blink::mojom::ContentSecurityPolicy::New(
      "default-src 'none'",
      blink::WebContentSecurityPolicyType::kWebContentSecurityPolicyTypeReport,
      true));

  // get on empty store -> nothing
  base::RunLoop run_loop;
  store.GetContentSecurityPolicies(
      origin, base::BindOnce(&CheckEmptyContentSecurityPolicies,
                             run_loop.QuitClosure()));
  run_loop.Run();

  // Add origin manifest
  base::RunLoop run_loop2;
  store.Add(om.Clone(), run_loop2.QuitClosure());
  run_loop2.Run();

  // get with same origin -> the OM (don't test for object equality, but
  // equivalence)
  base::RunLoop run_loop3;
  store.GetContentSecurityPolicies(
      origin, base::BindOnce(&CheckHasContentSecurityPolicies,
                             run_loop3.QuitClosure()));
  run_loop3.Run();

  // get with different origin -> nothing
  base::RunLoop run_loop4;
  store.GetContentSecurityPolicies(
      url::Origin(GURL("https://b.com")),
      base::BindOnce(&CheckEmptyContentSecurityPolicies,
                     run_loop4.QuitClosure()));
  run_loop4.Run();

  // remove origin
  base::RunLoop run_loop5;
  store.Remove(origin, run_loop5.QuitClosure());
  run_loop5.Run();

  // get with same origin -> nothing
  base::RunLoop run_loop6;
  store.GetContentSecurityPolicies(
      origin, base::BindOnce(&CheckEmptyContentSecurityPolicies,
                             run_loop6.QuitClosure()));
  run_loop6.Run();
}

}  // namespace content
