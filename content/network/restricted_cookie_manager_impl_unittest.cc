// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/network/restricted_cookie_manager_impl.h"

#include <algorithm>

#include "base/message_loop/message_loop.h"
#include "base/run_loop.h"
#include "base/time/time.h"
#include "net/cookies/cookie_constants.h"
#include "net/cookies/cookie_monster.h"
#include "net/cookies/cookie_store.h"
#include "net/cookies/cookie_store_test_callbacks.h"
#include "services/network/public/interfaces/cookie_manager.mojom.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace content {

// Synchronous proxies to a wrapped RestrictedCookieManager's methods.
class RestrictedCookieManagerSync {
 public:
  // Caller must guarantee that |*cookie_service| outlives the
  // SynchronousCookieManager.
  explicit RestrictedCookieManagerSync(
      network::mojom::RestrictedCookieManager* cookie_service)
      : cookie_service_(cookie_service) {}
  ~RestrictedCookieManagerSync() {}

  std::vector<net::CanonicalCookie> GetAllForUrl(
      const GURL& url,
      const GURL& site_for_cookies,
      network::mojom::CookieManagerGetOptionsPtr options) {
    base::RunLoop run_loop;
    std::vector<net::CanonicalCookie> cookies;
    cookie_service_->GetAllForUrl(
        url, site_for_cookies, std::move(options),
        base::BindOnce(&RestrictedCookieManagerSync::GetAllForUrlCallback,
                       &run_loop, &cookies));
    run_loop.Run();
    return cookies;
  }

  bool SetCanonicalCookie(const net::CanonicalCookie& cookie,
                          const GURL& url,
                          const GURL& site_for_cookies) {
    base::RunLoop run_loop;
    bool result = false;
    cookie_service_->SetCanonicalCookie(
        cookie, url, site_for_cookies,
        base::BindOnce(&RestrictedCookieManagerSync::SetCanonicalCookieCallback,
                       &run_loop, &result));
    run_loop.Run();
    return result;
  }

 private:
  static void GetAllForUrlCallback(
      base::RunLoop* run_loop,
      std::vector<net::CanonicalCookie>* cookies_out,
      const std::vector<net::CanonicalCookie>& cookies) {
    *cookies_out = cookies;
    run_loop->Quit();
  }

  static void SetCanonicalCookieCallback(base::RunLoop* run_loop,
                                         bool* result_out,
                                         bool result) {
    *result_out = result;
    run_loop->Quit();
  }

  network::mojom::RestrictedCookieManager* cookie_service_;

  DISALLOW_COPY_AND_ASSIGN(RestrictedCookieManagerSync);
};

class RestrictedCookieManagerImplTest : public testing::Test {
 public:
  RestrictedCookieManagerImplTest()
      : cookie_monster_(nullptr, nullptr),
        cookie_service_(
            base::MakeUnique<RestrictedCookieManagerImpl>(&cookie_monster_,
                                                          0,
                                                          0)) {
    sync_service_wrapper_ = base::MakeUnique<RestrictedCookieManagerSync>(
        cookie_service_ptr_.get());
  }
  ~RestrictedCookieManagerImplTest() override {}

  void SetUp() override {
    // Set a couple of cookies for tests to play with.
    bool result;
    result = SetCanonicalCookie(
        net::CanonicalCookie(
            "answer", "42", "example.com", "/", base::Time(), base::Time(),
            base::Time(), /* secure = */ false, /* httponly = */ false,
            net::CookieSameSite::NO_RESTRICTION, net::COOKIE_PRIORITY_DEFAULT),
        true, true);
    DCHECK(result);
  }

  // Set a canonical cookie directly into the store.
  bool SetCanonicalCookie(const net::CanonicalCookie& cookie,
                          bool secure_source,
                          bool can_modify_httponly) {
    net::ResultSavingCookieCallback<bool> callback;
    cookie_monster_.SetCanonicalCookieAsync(
        base::MakeUnique<net::CanonicalCookie>(cookie), secure_source,
        can_modify_httponly,
        base::BindOnce(&net::ResultSavingCookieCallback<bool>::Run,
                       base::Unretained(&callback)));
    callback.WaitUntilDone();
    return callback.result();
  }

  net::CookieStore* cookie_store() { return &cookie_monster_; }

  RestrictedCookieManagerSync* sync_service() {
    return sync_service_wrapper_.get();
  }

 private:
  base::MessageLoopForIO message_loop_;
  net::CookieMonster cookie_monster_;
  std::unique_ptr<content::RestrictedCookieManagerImpl> cookie_service_;
  network::mojom::RestrictedCookieManagerPtr cookie_service_ptr_;
  std::unique_ptr<RestrictedCookieManagerSync> sync_service_wrapper_;
};

namespace {

bool CompareCanonicalCookies(const net::CanonicalCookie& c1,
                             const net::CanonicalCookie& c2) {
  return c1.FullCompare(c2);
}

}  // anonymous namespace

TEST_F(RestrictedCookieManagerImplTest, GetAllForUrl) {
  auto options = network::mojom::CookieManagerGetOptions::New();
  options->name = "";
  options->match_type = network::mojom::CookieMatchType::STARTS_WITH;
  std::vector<net::CanonicalCookie> cookies = sync_service()->GetAllForUrl(
      GURL("http://example.com/test/"), GURL("http://example.com"),
      std::move(options));

  ASSERT_EQ(1u, cookies.size());

  EXPECT_EQ("answer", cookies[0].Name());
  EXPECT_EQ("42", cookies[0].Value());
}

TEST_F(RestrictedCookieManagerImplTest, SetCanonicalCookie) {
  EXPECT_TRUE(sync_service()->SetCanonicalCookie(
      net::CanonicalCookie(
          "foo", "bar", "example.com", "/", base::Time(), base::Time(),
          base::Time(), /* secure = */ false, /* httponly = */ false,
          net::CookieSameSite::NO_RESTRICTION, net::COOKIE_PRIORITY_DEFAULT),
      GURL("http://example.com/test/"), GURL("http://example.com")));

  auto options = network::mojom::CookieManagerGetOptions::New();
  options->name = "";
  options->match_type = network::mojom::CookieMatchType::STARTS_WITH;
  std::vector<net::CanonicalCookie> cookies = sync_service()->GetAllForUrl(
      GURL("http://example.com/test/"), GURL("http://example.com"),
      std::move(options));

  ASSERT_EQ(2u, cookies.size());
  std::sort(cookies.begin(), cookies.end(), &CompareCanonicalCookies);

  EXPECT_EQ("answer", cookies[0].Name());
  EXPECT_EQ("42", cookies[0].Value());

  EXPECT_EQ("foo", cookies[1].Name());
  EXPECT_EQ("bar", cookies[1].Value());
}

}  // namespace content
