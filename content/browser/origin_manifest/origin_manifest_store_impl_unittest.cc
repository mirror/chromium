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
  // This is black magic, voodoo, call it what you want
  // https://cs.chromium.org/chromium/src/base/test/scoped_task_environment.h?l=26
  base::test::ScopedTaskEnvironment scoped_task_environment_;
};

// Note that we do not initialize the persistent store here since that
// is subject to testing the persistent store itself.

TEST_F(OriginManifestStoreImplTest, QueryOriginManifest) {
  OriginManifestStoreImpl store(nullptr);
  mojom::OriginManifestPtr om = mojom::OriginManifest::New();
  om->origin = "https://a.com";
  om->version = "version1";
  url::Origin origin(GURL(om->origin));

  EXPECT_TRUE(store.Get(origin).Equals(nullptr));

  // Add origin manifest
  base::RunLoop run_loop;
  store.Add(om.Clone(), run_loop.QuitClosure());
  run_loop.Run();

  // get with same origin -> the equivalent OM
  mojom::OriginManifestPtr om2 = store.Get(origin);
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
                              mojom::CORSPreflightPtr corspreflights) {
  EXPECT_TRUE(corspreflights.Equals(nullptr));
  std::move(quit_closure).Run();
}

void CheckHasCORSPreflights(base::Closure quit_closure,
                            mojom::CORSPreflightPtr corspreflights) {
  EXPECT_FALSE(corspreflights.Equals(nullptr));

  EXPECT_EQ(corspreflights->nocredentials->origins.size(), 1ul);
  EXPECT_TRUE(corspreflights->nocredentials->origins[0] == "https://a.com");

  EXPECT_EQ(corspreflights->withcredentials->origins.size(), 1ul);
  EXPECT_TRUE(corspreflights->withcredentials->origins[0] == "https://b.com");

  std::move(quit_closure).Run();
}

}  // namespace

TEST_F(OriginManifestStoreImplTest, QueryCORSPreflight) {
  OriginManifestStoreImpl store(nullptr);
  mojom::OriginManifestPtr om = mojom::OriginManifest::New();
  om->origin = "https://a.com";
  std::vector<std::string> nocredentials;
  nocredentials.push_back("https://a.com");
  std::vector<std::string> withcredentials;
  withcredentials.push_back("https://b.com");
  om->corspreflights = mojom::CORSPreflight::New(
      mojom::CORSNoCredentials::New(nocredentials),
      mojom::CORSWithCredentials::New(withcredentials));
  url::Origin origin(GURL(om->origin));

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
    std::vector<mojom::ContentSecurityPolicyPtr> csps) {
  ASSERT_EQ(csps.size(), 0ul);
  std::move(quit_closure).Run();
}

void CheckHasContentSecurityPolicies(
    base::Closure quit_closure,
    std::vector<mojom::ContentSecurityPolicyPtr> csps) {
  ASSERT_EQ(csps.size(), 1ul);

  EXPECT_TRUE(csps[0]->policy == "default-src 'none'");
  EXPECT_EQ(csps[0]->disposition, mojom::Disposition::REPORT_ONLY);
  EXPECT_TRUE(csps[0]->allowOverride);

  std::move(quit_closure).Run();
}

}  // namespace

TEST_F(OriginManifestStoreImplTest, QueryContentSecurityPolicies) {
  OriginManifestStoreImpl store(nullptr);
  mojom::OriginManifestPtr om = mojom::OriginManifest::New();
  om->origin = "https://a.com";
  om->csps.push_back(mojom::ContentSecurityPolicy::New(
      "default-src 'none'", mojom::Disposition::REPORT_ONLY, true));
  url::Origin origin(GURL(om->origin));

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
