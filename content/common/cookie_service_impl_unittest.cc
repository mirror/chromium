// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/common/cookie_service_impl.h"

#include <algorithm>

// TODO(rdsmith): Sort below :-}.
#include "base/message_loop/message_loop.h"
#include "base/run_loop.h"
#include "base/time/time.h"
#include "content/common/cookie.mojom.h"
#include "net/cookies/cookie_constants.h"
#include "net/cookies/cookie_monster.h"
#include "net/cookies/cookie_store.h"
#include "net/cookies/cookie_store_test_callbacks.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {

// Callback functions for use by SynchronousCookieServiceWrapper.
void GetCookiesCallback(base::RunLoop* run_loop,
                        std::vector<net::CanonicalCookie>* cookies_out,
                        const std::vector<net::CanonicalCookie>& cookies) {
  *cookies_out = cookies;
  run_loop->Quit();
}

void SetCookieCallback(base::RunLoop* run_loop, bool* result_out, bool result) {
  *result_out = result;
  run_loop->Quit();
}

void DeleteCookiesCallback(base::RunLoop* run_loop,
                           uint32_t* num_deleted_out,
                           uint32_t num_deleted) {
  *num_deleted_out = num_deleted;
  run_loop->Quit();
}

void RequestNotificationCallback(
    base::RunLoop* run_loop,
    content::mojom::CookieChangeNotificationRequest* request_out,
    content::mojom::CookieChangeNotificationRequest request) {
  *request_out = std::move(request);
  run_loop->Quit();
}

}  // namespace

namespace content {

// Wraps a CookieService in synchronous, blocking calls to make it easier to
// test.
class SynchronousCookieServiceWrapper {
 public:
  // Caller must guarantee that |*cookie_service| outlives the
  // SynchronousCookieServiceWrapper.
  explicit SynchronousCookieServiceWrapper(mojom::CookieService* cookie_service)
      : cookie_service_(cookie_service) {}
  ~SynchronousCookieServiceWrapper() {}

  // Use of Unretained pointers in the following is safe because they are
  // only outstanding for the life of the method call, and the
  // SynchronousCookieServiceWrapper object is presumed to be alive for the
  // life of all of it's method calls.

  std::vector<net::CanonicalCookie> GetAllCookies() {
    base::RunLoop run_loop;
    std::vector<net::CanonicalCookie> cookies;
    cookie_service_->GetAllCookies(
        base::BindOnce(&GetCookiesCallback, &run_loop, &cookies));
    run_loop.Run();
    return cookies;
  }

  std::vector<net::CanonicalCookie> GetCookieList(const GURL& url,
                                                  net::CookieOptions options) {
    base::RunLoop run_loop;
    std::vector<net::CanonicalCookie> cookies;
    cookie_service_->GetCookieList(
        url, options, base::BindOnce(&GetCookiesCallback, &run_loop, &cookies));

    run_loop.Run();
    return cookies;
  }

  bool SetCanonicalCookie(const net::CanonicalCookie& cookie,
                          bool secure_source,
                          bool modify_http_only) {
    base::RunLoop run_loop;
    bool result = false;
    cookie_service_->SetCanonicalCookie(
        cookie, secure_source, modify_http_only,
        base::BindOnce(&SetCookieCallback, &run_loop, &result));
    run_loop.Run();
    return result;
  }

  uint32_t DeleteCookies(mojom::CookieDeletionFilter filter) {
    base::RunLoop run_loop;
    uint32_t num_deleted = 0u;
    mojom::CookieDeletionFilterPtr filter_ptr =
        mojom::CookieDeletionFilter::New(filter);

    cookie_service_->DeleteCookies(
        std::move(filter_ptr),
        base::BindOnce(&DeleteCookiesCallback, &run_loop, &num_deleted));
    run_loop.Run();
    return num_deleted;
  }

  mojom::CookieChangeNotificationRequest RequestNotification(
      const GURL& url,
      std::string cookie_name) {
    base::RunLoop run_loop;
    content::mojom::CookieChangeNotificationRequest request;

    cookie_service_->RequestNotification(
        url, cookie_name,
        base::BindOnce(&RequestNotificationCallback, &run_loop, &request));
    run_loop.Run();
    return request;
  }

  // No need to wrap CloneInterface, since its use is pure async.
 private:
  mojom::CookieService* cookie_service_;

  DISALLOW_COPY_AND_ASSIGN(SynchronousCookieServiceWrapper);
};

class CookieServiceTest : public testing::Test {
 public:
  CookieServiceTest()
      : connection_error_seen_(false),
        cookie_monster_(nullptr, nullptr),
        cookie_service_(base::MakeUnique<CookieServiceImpl>(&cookie_monster_)),
        cookie_service_request_(mojo::MakeRequest(&cookie_service_ptr_)),
        service_wrapper_(cookie_service_ptr_.get()) {
    cookie_service_->AddRequest(std::move(cookie_service_request_));
    cookie_service_ptr_.set_connection_error_handler(base::BindOnce(
        &CookieServiceTest::OnConnectionError, base::Unretained(this)));
  }
  ~CookieServiceTest() override {}

  void SetUp() override {
    setup_time_ = base::Time::Now();

    // Set a couple of cookies for tests to play with.
    bool result;
    result = SetCanonicalCookie(
        net::CanonicalCookie("A", "B", "foo_host", "/", base::Time(),
                             base::Time(), base::Time(), false, false,
                             net::CookieSameSite::NO_RESTRICTION,
                             net::COOKIE_PRIORITY_MEDIUM),
        true, true);
    DCHECK(result);

    result = SetCanonicalCookie(
        net::CanonicalCookie("C", "D", "foo_host2", "/with/path", base::Time(),
                             base::Time(), base::Time(), false, false,
                             net::CookieSameSite::NO_RESTRICTION,
                             net::COOKIE_PRIORITY_MEDIUM),
        true, true);
    DCHECK(result);

    result = SetCanonicalCookie(
        net::CanonicalCookie("Secure", "E", "foo_host", "/with/path",
                             base::Time(), base::Time(), base::Time(), true,
                             false, net::CookieSameSite::NO_RESTRICTION,
                             net::COOKIE_PRIORITY_MEDIUM),
        true, true);
    DCHECK(result);

    result = SetCanonicalCookie(
        net::CanonicalCookie("HttpOnly", "F", "foo_host", "/with/path",
                             base::Time(), base::Time(), base::Time(), false,
                             true, net::CookieSameSite::NO_RESTRICTION,
                             net::COOKIE_PRIORITY_MEDIUM),
        true, true);
    DCHECK(result);
  }

  // Tear down the remote service.
  void NukeService() { cookie_service_.reset(); }

  // Set a canonical cookie directly into the store.
  bool SetCanonicalCookie(const net::CanonicalCookie& cookie,
                          bool secure_source,
                          bool can_modify_httponly) {
    net::ResultSavingCookieCallback<bool> callback;
    cookie_monster_.SetCanonicalCookieAsync(
        base::MakeUnique<net::CanonicalCookie>(cookie), secure_source,
        can_modify_httponly,
        base::Bind(&net::ResultSavingCookieCallback<bool>::Run,
                   base::Unretained(&callback)));
    callback.WaitUntilDone();
    return callback.result();
  }

  net::CookieStore* cookie_store() { return &cookie_monster_; }

  CookieServiceImpl* service_impl() const { return cookie_service_.get(); }

  // Return the cookie service at the client end of the mojo pipe.
  mojom::CookieService* cookie_service_client() {
    return cookie_service_ptr_.get();
  }

  // Synchronous wrapper
  SynchronousCookieServiceWrapper* service_wrapper() {
    return &service_wrapper_;
  }

  base::Time setup_time() { return setup_time_; }

  bool connection_error_seen() const { return connection_error_seen_; }

 private:
  void OnConnectionError() { connection_error_seen_ = true; }

  bool connection_error_seen_;

  base::MessageLoopForIO message_loop_;
  net::CookieMonster cookie_monster_;
  std::unique_ptr<content::CookieServiceImpl> cookie_service_;
  mojom::CookieServicePtr cookie_service_ptr_;
  mojom::CookieServiceRequest cookie_service_request_;
  SynchronousCookieServiceWrapper service_wrapper_;
  base::Time setup_time_;

  DISALLOW_COPY_AND_ASSIGN(CookieServiceTest);
};

bool CompareCanonicalCookies(const net::CanonicalCookie& c1,
                             const net::CanonicalCookie& c2) {
  return c1.FullCompare(c2);
}

// Test the GetAllCookies accessor.  Also tests that canonical
// cookies come out of the store unchanged.
TEST_F(CookieServiceTest, GetAllCookies) {
  base::Time now(base::Time::Now());

  std::vector<net::CanonicalCookie> cookies =
      service_wrapper()->GetAllCookies();

  ASSERT_EQ(4u, cookies.size());
  std::sort(cookies.begin(), cookies.end(), &CompareCanonicalCookies);

  EXPECT_EQ("A", cookies[0].Name());
  EXPECT_EQ("B", cookies[0].Value());
  EXPECT_EQ("foo_host", cookies[0].Domain());
  EXPECT_EQ("/", cookies[0].Path());
  EXPECT_LT(setup_time(), cookies[0].CreationDate());
  EXPECT_LT(cookies[0].CreationDate(), now);
  EXPECT_EQ(cookies[0].LastAccessDate(), base::Time());
  EXPECT_EQ(cookies[0].ExpiryDate(), base::Time());
  EXPECT_FALSE(cookies[0].IsPersistent());
  EXPECT_FALSE(cookies[0].IsSecure());
  EXPECT_FALSE(cookies[0].IsHttpOnly());
  EXPECT_EQ(net::CookieSameSite::NO_RESTRICTION, cookies[0].SameSite());
  EXPECT_EQ(net::COOKIE_PRIORITY_MEDIUM, cookies[0].Priority());

  EXPECT_EQ("C", cookies[1].Name());
  EXPECT_EQ("D", cookies[1].Value());
  EXPECT_EQ("foo_host2", cookies[1].Domain());
  EXPECT_EQ("/with/path", cookies[1].Path());
  EXPECT_LT(setup_time(), cookies[1].CreationDate());
  EXPECT_LT(cookies[1].CreationDate(), now);
  EXPECT_EQ(cookies[1].LastAccessDate(), base::Time());
  EXPECT_EQ(cookies[1].ExpiryDate(), base::Time());
  EXPECT_FALSE(cookies[1].IsPersistent());
  EXPECT_FALSE(cookies[1].IsSecure());
  EXPECT_FALSE(cookies[1].IsHttpOnly());
  EXPECT_EQ(net::CookieSameSite::NO_RESTRICTION, cookies[1].SameSite());
  EXPECT_EQ(net::COOKIE_PRIORITY_MEDIUM, cookies[1].Priority());

  EXPECT_EQ("HttpOnly", cookies[2].Name());
  EXPECT_EQ("F", cookies[2].Value());
  EXPECT_EQ("foo_host", cookies[2].Domain());
  EXPECT_EQ("/with/path", cookies[2].Path());
  EXPECT_LT(setup_time(), cookies[2].CreationDate());
  EXPECT_LT(cookies[2].CreationDate(), now);
  EXPECT_EQ(cookies[2].LastAccessDate(), base::Time());
  EXPECT_EQ(cookies[2].ExpiryDate(), base::Time());
  EXPECT_FALSE(cookies[2].IsPersistent());
  EXPECT_FALSE(cookies[2].IsSecure());
  EXPECT_TRUE(cookies[2].IsHttpOnly());
  EXPECT_EQ(net::CookieSameSite::NO_RESTRICTION, cookies[2].SameSite());
  EXPECT_EQ(net::COOKIE_PRIORITY_MEDIUM, cookies[2].Priority());

  EXPECT_EQ("Secure", cookies[3].Name());
  EXPECT_EQ("E", cookies[3].Value());
  EXPECT_EQ("foo_host", cookies[3].Domain());
  EXPECT_EQ("/with/path", cookies[3].Path());
  EXPECT_LT(setup_time(), cookies[3].CreationDate());
  EXPECT_LT(cookies[3].CreationDate(), now);
  EXPECT_EQ(cookies[3].LastAccessDate(), base::Time());
  EXPECT_EQ(cookies[3].ExpiryDate(), base::Time());
  EXPECT_FALSE(cookies[3].IsPersistent());
  EXPECT_TRUE(cookies[3].IsSecure());
  EXPECT_FALSE(cookies[3].IsHttpOnly());
  EXPECT_EQ(net::CookieSameSite::NO_RESTRICTION, cookies[3].SameSite());
  EXPECT_EQ(net::COOKIE_PRIORITY_MEDIUM, cookies[3].Priority());
}

TEST_F(CookieServiceTest, GetCookieList) {
  std::vector<net::CanonicalCookie> cookies = service_wrapper()->GetCookieList(
      GURL("https://foo_host/with/path"), net::CookieOptions());

  EXPECT_EQ(2u, cookies.size());
  std::sort(cookies.begin(), cookies.end(), &CompareCanonicalCookies);

  EXPECT_EQ("A", cookies[0].Name());
  EXPECT_EQ("B", cookies[0].Value());

  EXPECT_EQ("Secure", cookies[1].Name());
  EXPECT_EQ("E", cookies[1].Value());
}

TEST_F(CookieServiceTest, SetExtraCookie) {
  EXPECT_TRUE(service_wrapper()->SetCanonicalCookie(
      net::CanonicalCookie("X", "Y", "new_host", "/", base::Time(),
                           base::Time(), base::Time(), false, false,
                           net::CookieSameSite::NO_RESTRICTION,
                           net::COOKIE_PRIORITY_MEDIUM),
      false, false));

  std::vector<net::CanonicalCookie> cookies =
      service_wrapper()->GetAllCookies();

  ASSERT_EQ(5u, cookies.size());
  std::sort(cookies.begin(), cookies.end(), &CompareCanonicalCookies);

  EXPECT_EQ("A", cookies[0].Name());
  EXPECT_EQ("B", cookies[0].Value());

  EXPECT_EQ("C", cookies[1].Name());
  EXPECT_EQ("D", cookies[1].Value());

  EXPECT_EQ("HttpOnly", cookies[2].Name());
  EXPECT_EQ("F", cookies[2].Value());

  EXPECT_EQ("Secure", cookies[3].Name());
  EXPECT_EQ("E", cookies[3].Value());

  EXPECT_EQ("X", cookies[4].Name());
  EXPECT_EQ("Y", cookies[4].Value());
}

TEST_F(CookieServiceTest, DeleteThroughSet) {
  base::Time yesterday = base::Time::Now() - base::TimeDelta::FromDays(1);
  EXPECT_TRUE(service_wrapper()->SetCanonicalCookie(
      net::CanonicalCookie("A", "E", "foo_host", "/", base::Time(), yesterday,
                           base::Time(), false, false,
                           net::CookieSameSite::NO_RESTRICTION,
                           net::COOKIE_PRIORITY_MEDIUM),
      false, false));

  std::vector<net::CanonicalCookie> cookies =
      service_wrapper()->GetAllCookies();

  ASSERT_EQ(3u, cookies.size());
  std::sort(cookies.begin(), cookies.end(), &CompareCanonicalCookies);

  EXPECT_EQ("C", cookies[0].Name());
  EXPECT_EQ("D", cookies[0].Value());

  EXPECT_EQ("HttpOnly", cookies[1].Name());
  EXPECT_EQ("F", cookies[1].Value());

  EXPECT_EQ("Secure", cookies[2].Name());
  EXPECT_EQ("E", cookies[2].Value());
}

TEST_F(CookieServiceTest, ConfirmSecureSetFails) {
  EXPECT_FALSE(service_wrapper()->SetCanonicalCookie(
      net::CanonicalCookie("N", "O", "foo_host", "/", base::Time(),
                           base::Time(), base::Time(), true, false,
                           net::CookieSameSite::NO_RESTRICTION,
                           net::COOKIE_PRIORITY_MEDIUM),
      false, false));
  std::vector<net::CanonicalCookie> cookies =
      service_wrapper()->GetAllCookies();

  ASSERT_EQ(4u, cookies.size());
}

TEST_F(CookieServiceTest, ConfirmHttpOnlySetFails) {
  EXPECT_FALSE(service_wrapper()->SetCanonicalCookie(
      net::CanonicalCookie("N", "O", "foo_host", "/", base::Time(),
                           base::Time(), base::Time(), false, true,
                           net::CookieSameSite::NO_RESTRICTION,
                           net::COOKIE_PRIORITY_MEDIUM),
      false, false));
  std::vector<net::CanonicalCookie> cookies =
      service_wrapper()->GetAllCookies();

  ASSERT_EQ(4u, cookies.size());
}

TEST_F(CookieServiceTest, ConfirmHttpOnlyOverwriteFails) {
  EXPECT_FALSE(service_wrapper()->SetCanonicalCookie(
      net::CanonicalCookie("HttpOnly", "Nope", "foo_host", "/with/path",
                           base::Time(), base::Time(), base::Time(), false,
                           true, net::CookieSameSite::NO_RESTRICTION,
                           net::COOKIE_PRIORITY_MEDIUM),
      false, false));

  std::vector<net::CanonicalCookie> cookies =
      service_wrapper()->GetAllCookies();
  ASSERT_EQ(4u, cookies.size());
  std::sort(cookies.begin(), cookies.end(), &CompareCanonicalCookies);
  EXPECT_EQ("HttpOnly", cookies[2].Name());
  EXPECT_EQ("F", cookies[2].Value());
}

TEST_F(CookieServiceTest, DeleteEverything) {
  mojom::CookieDeletionFilter filter;
  EXPECT_EQ(4u, service_wrapper()->DeleteCookies(filter));

  std::vector<net::CanonicalCookie> cookies =
      service_wrapper()->GetAllCookies();
  ASSERT_EQ(0u, cookies.size());
}

TEST_F(CookieServiceTest, DeleteByTime) {
  mojom::CookieDeletionFilter filter;
  EXPECT_EQ(4u, service_wrapper()->DeleteCookies(filter));
  base::Time now(base::Time::Now());

  // Create three cookies and delete the middle one.
  EXPECT_TRUE(SetCanonicalCookie(
      net::CanonicalCookie(
          "A1", "val", "foo_host", "/", now - base::TimeDelta::FromMinutes(60),
          base::Time(), base::Time(), false, false,
          net::CookieSameSite::NO_RESTRICTION, net::COOKIE_PRIORITY_MEDIUM),
      true, true));

  EXPECT_TRUE(SetCanonicalCookie(
      net::CanonicalCookie(
          "A2", "val", "foo_host", "/", now - base::TimeDelta::FromMinutes(120),
          base::Time(), base::Time(), false, false,
          net::CookieSameSite::NO_RESTRICTION, net::COOKIE_PRIORITY_MEDIUM),
      true, true));

  EXPECT_TRUE(SetCanonicalCookie(
      net::CanonicalCookie(
          "A3", "val", "foo_host", "/", now - base::TimeDelta::FromMinutes(180),
          base::Time(), base::Time(), false, false,
          net::CookieSameSite::NO_RESTRICTION, net::COOKIE_PRIORITY_MEDIUM),
      true, true));

  filter.created_after_time = now - base::TimeDelta::FromMinutes(150);
  filter.created_before_time = now - base::TimeDelta::FromMinutes(90);
  EXPECT_EQ(1u, service_wrapper()->DeleteCookies(filter));
  std::vector<net::CanonicalCookie> cookies =
      service_wrapper()->GetAllCookies();
  ASSERT_EQ(2u, cookies.size());
  std::sort(cookies.begin(), cookies.end(), &CompareCanonicalCookies);
  EXPECT_EQ("A1", cookies[0].Name());
  EXPECT_EQ("A3", cookies[1].Name());
}

TEST_F(CookieServiceTest, DeleteByBlacklist) {
  mojom::CookieDeletionFilter filter;
  EXPECT_EQ(4u, service_wrapper()->DeleteCookies(filter));

  // Create three cookies and delete the middle one.
  EXPECT_TRUE(SetCanonicalCookie(
      net::CanonicalCookie("A1", "val", "foo_host1", "/", base::Time(),
                           base::Time(), base::Time(), false, false,
                           net::CookieSameSite::NO_RESTRICTION,
                           net::COOKIE_PRIORITY_MEDIUM),
      true, true));

  EXPECT_TRUE(SetCanonicalCookie(
      net::CanonicalCookie("A2", "val", "foo_host2", "/", base::Time(),
                           base::Time(), base::Time(), false, false,
                           net::CookieSameSite::NO_RESTRICTION,
                           net::COOKIE_PRIORITY_MEDIUM),
      true, true));

  EXPECT_TRUE(SetCanonicalCookie(
      net::CanonicalCookie("A3", "val", "foo_host3", "/", base::Time(),
                           base::Time(), base::Time(), false, false,
                           net::CookieSameSite::NO_RESTRICTION,
                           net::COOKIE_PRIORITY_MEDIUM),
      true, true));

  filter.domain_blacklist = std::vector<std::string>();
  filter.domain_blacklist->push_back("foo_host2");
  EXPECT_EQ(1u, service_wrapper()->DeleteCookies(filter));
  std::vector<net::CanonicalCookie> cookies =
      service_wrapper()->GetAllCookies();
  ASSERT_EQ(2u, cookies.size());
  std::sort(cookies.begin(), cookies.end(), &CompareCanonicalCookies);
  EXPECT_EQ("A1", cookies[0].Name());
  EXPECT_EQ("A3", cookies[1].Name());
}

TEST_F(CookieServiceTest, DeleteByWhitelist) {
  mojom::CookieDeletionFilter filter;
  EXPECT_EQ(4u, service_wrapper()->DeleteCookies(filter));

  // Create three cookies and delete the middle one.
  EXPECT_TRUE(SetCanonicalCookie(
      net::CanonicalCookie("A1", "val", "foo_host1", "/", base::Time(),
                           base::Time(), base::Time(), false, false,
                           net::CookieSameSite::NO_RESTRICTION,
                           net::COOKIE_PRIORITY_MEDIUM),
      true, true));

  EXPECT_TRUE(SetCanonicalCookie(
      net::CanonicalCookie("A2", "val", "foo_host2", "/", base::Time(),
                           base::Time(), base::Time(), false, false,
                           net::CookieSameSite::NO_RESTRICTION,
                           net::COOKIE_PRIORITY_MEDIUM),
      true, true));

  EXPECT_TRUE(SetCanonicalCookie(
      net::CanonicalCookie("A3", "val", "foo_host3", "/", base::Time(),
                           base::Time(), base::Time(), false, false,
                           net::CookieSameSite::NO_RESTRICTION,
                           net::COOKIE_PRIORITY_MEDIUM),
      true, true));

  filter.domain_whitelist = std::vector<std::string>();
  filter.domain_whitelist->push_back("foo_host1");
  filter.domain_whitelist->push_back("foo_host3");
  EXPECT_EQ(1u, service_wrapper()->DeleteCookies(filter));
  std::vector<net::CanonicalCookie> cookies =
      service_wrapper()->GetAllCookies();
  ASSERT_EQ(2u, cookies.size());
  std::sort(cookies.begin(), cookies.end(), &CompareCanonicalCookies);
  EXPECT_EQ("A1", cookies[0].Name());
  EXPECT_EQ("A3", cookies[1].Name());
}

TEST_F(CookieServiceTest, DeleteBySessionStatus) {
  mojom::CookieDeletionFilter filter;
  EXPECT_EQ(4u, service_wrapper()->DeleteCookies(filter));
  base::Time now(base::Time::Now());

  // Create three cookies and delete the middle one.
  EXPECT_TRUE(SetCanonicalCookie(
      net::CanonicalCookie("A1", "val", "foo_host", "/", base::Time(),
                           base::Time(), base::Time(), false, false,
                           net::CookieSameSite::NO_RESTRICTION,
                           net::COOKIE_PRIORITY_MEDIUM),
      true, true));

  EXPECT_TRUE(SetCanonicalCookie(
      net::CanonicalCookie("A2", "val", "foo_host", "/", base::Time(),
                           now + base::TimeDelta::FromDays(1), base::Time(),
                           false, false, net::CookieSameSite::NO_RESTRICTION,
                           net::COOKIE_PRIORITY_MEDIUM),
      true, true));

  EXPECT_TRUE(SetCanonicalCookie(
      net::CanonicalCookie("A3", "val", "foo_host", "/", base::Time(),
                           base::Time(), base::Time(), false, false,
                           net::CookieSameSite::NO_RESTRICTION,
                           net::COOKIE_PRIORITY_MEDIUM),
      true, true));

  filter.session_control = mojom::CookieDeletionSessionControl::PERSISTENT;
  EXPECT_EQ(1u, service_wrapper()->DeleteCookies(filter));
  std::vector<net::CanonicalCookie> cookies =
      service_wrapper()->GetAllCookies();
  ASSERT_EQ(2u, cookies.size());
  std::sort(cookies.begin(), cookies.end(), &CompareCanonicalCookies);
  EXPECT_EQ("A1", cookies[0].Name());
  EXPECT_EQ("A3", cookies[1].Name());
}

TEST_F(CookieServiceTest, DeleteByAll) {
  mojom::CookieDeletionFilter filter;
  EXPECT_EQ(4u, service_wrapper()->DeleteCookies(filter));
  base::Time now(base::Time::Now());

  // Add a lot of cookies, only one of which will be deleted by the filter.
  // Filter will be:
  //    * Between two and four days ago.
  //    * Blacklisting no.com and nope.com
  //    * Whitelisting no.com and yes.com (whitelist wins on no.com
  //      because of ANDing)
  //    * Persistent cookies.
  // The archetypal cookie (which will be deleted) will satisfy all of
  // these filters (2 days old, nope.com, persistent).
  // Each of the other four cookies will vary in a way that will take it out
  // of being deleted by one of the filter.

  filter.created_after_time = now - base::TimeDelta::FromDays(4);
  filter.created_before_time = now - base::TimeDelta::FromDays(2);
  filter.domain_blacklist = std::vector<std::string>();
  filter.domain_blacklist->push_back("no.com");
  filter.domain_blacklist->push_back("nope.com");
  filter.domain_whitelist = std::vector<std::string>();
  filter.domain_whitelist->push_back("no.com");
  filter.domain_whitelist->push_back("yes.com");
  filter.session_control = mojom::CookieDeletionSessionControl::PERSISTENT;

  // Architectypal cookie:
  EXPECT_TRUE(SetCanonicalCookie(
      net::CanonicalCookie(
          "A1", "val", "nope.com", "/", now - base::TimeDelta::FromDays(3),
          now + base::TimeDelta::FromDays(3), base::Time(), false, false,
          net::CookieSameSite::NO_RESTRICTION, net::COOKIE_PRIORITY_MEDIUM),
      true, true));

  // Too old cookie.
  EXPECT_TRUE(SetCanonicalCookie(
      net::CanonicalCookie(
          "A2", "val", "nope.com", "/", now - base::TimeDelta::FromDays(5),
          now + base::TimeDelta::FromDays(3), base::Time(), false, false,
          net::CookieSameSite::NO_RESTRICTION, net::COOKIE_PRIORITY_MEDIUM),
      true, true));

  // Too young cookie.
  EXPECT_TRUE(SetCanonicalCookie(
      net::CanonicalCookie(
          "A3", "val", "nope.com", "/", now - base::TimeDelta::FromDays(1),
          now + base::TimeDelta::FromDays(3), base::Time(), false, false,
          net::CookieSameSite::NO_RESTRICTION, net::COOKIE_PRIORITY_MEDIUM),
      true, true));

  // Not in blacklist.
  EXPECT_TRUE(SetCanonicalCookie(
      net::CanonicalCookie(
          "A4", "val", "other.com", "/", now - base::TimeDelta::FromDays(3),
          now + base::TimeDelta::FromDays(3), base::Time(), false, false,
          net::CookieSameSite::NO_RESTRICTION, net::COOKIE_PRIORITY_MEDIUM),
      true, true));

  // In whitelist
  EXPECT_TRUE(SetCanonicalCookie(
      net::CanonicalCookie(
          "A5", "val", "no.com", "/", now - base::TimeDelta::FromDays(3),
          now + base::TimeDelta::FromDays(3), base::Time(), false, false,
          net::CookieSameSite::NO_RESTRICTION, net::COOKIE_PRIORITY_MEDIUM),
      true, true));

  // Session
  EXPECT_TRUE(SetCanonicalCookie(
      net::CanonicalCookie(
          "A6", "val", "nope.com", "/", now - base::TimeDelta::FromDays(3),
          base::Time(), base::Time(), false, false,
          net::CookieSameSite::NO_RESTRICTION, net::COOKIE_PRIORITY_MEDIUM),
      true, true));

  EXPECT_EQ(1u, service_wrapper()->DeleteCookies(filter));
  std::vector<net::CanonicalCookie> cookies =
      service_wrapper()->GetAllCookies();
  ASSERT_EQ(5u, cookies.size());
  std::sort(cookies.begin(), cookies.end(), &CompareCanonicalCookies);
  EXPECT_EQ("A2", cookies[0].Name());
  EXPECT_EQ("A3", cookies[1].Name());
  EXPECT_EQ("A4", cookies[2].Name());
  EXPECT_EQ("A5", cookies[3].Name());
  EXPECT_EQ("A6", cookies[4].Name());
}

// Plan for testing RequestNotification
//      * Create a class which implements CookieChangeNotification and
//        stores the results.  Include a waiter on the class.
//      * Create an instance of the class and bind it to a
//        CookieChangeNotificationPipe.
//      * Send that pipe across the interface
//      * Set a cookie; confirm you get the notification.
//      * Delete a cookie; confirm you get the notification

class CookieChangeNotificationImpl : public mojom::CookieChangeNotification {
 public:
  struct Notification {
    Notification(const net::CanonicalCookie& cookie_in,
                 mojom::CookieChangeCause cause_in) {
      cookie = cookie_in;
      cause = cause_in;
    }

    net::CanonicalCookie cookie;
    mojom::CookieChangeCause cause;
  };

  CookieChangeNotificationImpl(mojom::CookieChangeNotificationRequest request)
      : run_loop_(nullptr), binding_(this, std::move(request)) {}

  void WaitForNextNotification() {
    base::RunLoop loop;
    run_loop_ = &loop;
    loop.Run();
    run_loop_ = nullptr;
  }

  // Adds existing notifications to passed in vector.
  void GetCurrentNotifications(std::vector<Notification>* notifications) {
    notifications->insert(notifications->end(), notifications_.begin(),
                          notifications_.end());
    notifications_.clear();
  }

  // mojom::CookieChangesNotification
  void OnCookieChanged(const net::CanonicalCookie& cookie,
                       mojom::CookieChangeCause cause) override {
    notifications_.push_back(Notification(cookie, cause));
    if (run_loop_)
      run_loop_->Quit();
  }

 private:
  std::vector<Notification> notifications_;

  // Loop to signal on receiving a notification if not null.
  base::RunLoop* run_loop_;

  mojo::Binding<mojom::CookieChangeNotification> binding_;
};

TEST_F(CookieServiceTest, Notification) {
  GURL notification_url("http://www.testing.com/pathele");
  std::string notification_name("Cookie_Name");
  mojom::CookieChangeNotificationRequest request =
      service_wrapper()->RequestNotification(notification_url,
                                             notification_name);

  CookieChangeNotificationImpl notification_impl(std::move(request));

  std::vector<CookieChangeNotificationImpl::Notification> notifications;
  notification_impl.GetCurrentNotifications(&notifications);
  EXPECT_EQ(0u, notifications.size());
  notifications.clear();

  service_wrapper()->SetCanonicalCookie(
      net::CanonicalCookie("DifferentName", "val", notification_url.host(), "/",
                           base::Time(), base::Time(), base::Time(), false,
                           false, net::CookieSameSite::NO_RESTRICTION,
                           net::COOKIE_PRIORITY_MEDIUM),
      true, true);
  base::RunLoop().RunUntilIdle();
  notification_impl.GetCurrentNotifications(&notifications);
  EXPECT_EQ(0u, notifications.size());
  notifications.clear();

  service_wrapper()->SetCanonicalCookie(
      net::CanonicalCookie(notification_name, "val", "www.other.host", "/",
                           base::Time(), base::Time(), base::Time(), false,
                           false, net::CookieSameSite::NO_RESTRICTION,
                           net::COOKIE_PRIORITY_MEDIUM),
      true, true);

  base::RunLoop().RunUntilIdle();
  notification_impl.GetCurrentNotifications(&notifications);
  EXPECT_EQ(0u, notifications.size());
  notifications.clear();

  // Insert a cookie that matches.
  service_wrapper()->SetCanonicalCookie(
      net::CanonicalCookie(notification_name, "val", notification_url.host(),
                           "/", base::Time(), base::Time(), base::Time(), false,
                           false, net::CookieSameSite::NO_RESTRICTION,
                           net::COOKIE_PRIORITY_MEDIUM),
      true, true);

  // Expect asynchrony
  notification_impl.GetCurrentNotifications(&notifications);
  EXPECT_EQ(0u, notifications.size());
  notifications.clear();

  // Expect notification
  notification_impl.WaitForNextNotification();
  notification_impl.GetCurrentNotifications(&notifications);
  EXPECT_EQ(1u, notifications.size());
  EXPECT_EQ(notification_name, notifications[0].cookie.Name());
  EXPECT_EQ(notification_url.host(), notifications[0].cookie.Domain());
  EXPECT_EQ(mojom::CookieChangeCause::INSERTED, notifications[0].cause);
  notifications.clear();

  // Delete both the above cookies.
  mojom::CookieDeletionFilter filter;
  filter.domain_blacklist = std::vector<std::string>();
  filter.domain_blacklist->push_back(notification_url.host());
  EXPECT_EQ(2u, service_wrapper()->DeleteCookies(filter));

  // The notification may already have arrived, or it may arrive in the future.
  notification_impl.GetCurrentNotifications(&notifications);
  while (notifications.size() < 1u) {
    notification_impl.WaitForNextNotification();
    notification_impl.GetCurrentNotifications(&notifications);
  }
  ASSERT_EQ(1u, notifications.size());
  EXPECT_EQ(notification_name, notifications[0].cookie.Name());
  EXPECT_EQ(notification_url.host(), notifications[0].cookie.Domain());
  EXPECT_EQ(mojom::CookieChangeCause::EXPLICIT, notifications[0].cause);
}

// Confirm we get a connection error notification if the service dies.
TEST_F(CookieServiceTest, NotificationServiceDestruct) {
  EXPECT_FALSE(connection_error_seen());
  NukeService();
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(connection_error_seen());
}

// Test service cloning.  Also confirm that a service notice if a client
// dies.
TEST_F(CookieServiceTest, Cloning) {
  EXPECT_EQ(1u, service_impl()->GetNumberClientsBoundForTesting());

  // Clone the interface.
  std::unique_ptr<mojom::CookieServicePtr> new_ptr(
      base::MakeUnique<mojom::CookieServicePtr>());
  mojom::CookieServiceRequest new_request(mojo::MakeRequest(new_ptr.get()));
  cookie_service_client()->CloneInterface(std::move(new_request));

  SynchronousCookieServiceWrapper new_wrapper(new_ptr->get());

  // Set a cookie on the new interface and make sure it's visible on the
  // old one.
  EXPECT_TRUE(new_wrapper.SetCanonicalCookie(
      net::CanonicalCookie("X", "Y", "www.other.host", "/", base::Time(),
                           base::Time(), base::Time(), false, false,
                           net::CookieSameSite::NO_RESTRICTION,
                           net::COOKIE_PRIORITY_MEDIUM),
      true, true));

  std::vector<net::CanonicalCookie> cookies = service_wrapper()->GetCookieList(
      GURL("http://www.other.host/"), net::CookieOptions());
  ASSERT_EQ(1u, cookies.size());
  EXPECT_EQ("X", cookies[0].Name());
  EXPECT_EQ("Y", cookies[0].Value());

  // After a synchronous round trip through the new client pointer, it
  // should be reflected in the bindings seen on the server.
  EXPECT_EQ(2u, service_impl()->GetNumberClientsBoundForTesting());

  new_ptr.reset();
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(1u, service_impl()->GetNumberClientsBoundForTesting());
}

}  // namespace content
