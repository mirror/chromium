// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/base_switches.h"
#include "base/command_line.h"
#include "build/build_config.h"
#include "chrome/browser/extensions/extension_apitest.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/common/pref_names.h"
#include "components/prefs/pref_service.h"
#include "net/dns/mock_host_resolver.h"

namespace extensions {

class HistoryApiTest : public ExtensionApiTest {
 public:
  void SetUpOnMainThread() override {
    ExtensionApiTest::SetUpOnMainThread();

    host_resolver()->AddRule("www.a.com", "127.0.0.1");
    host_resolver()->AddRule("www.b.com", "127.0.0.1");
  }
};

// Full text search indexing sometimes exceeds a timeout. (http://crbug/119505)
IN_PROC_BROWSER_TEST_F(HistoryApiTest, DISABLED_MiscSearch) {
  ASSERT_TRUE(StartEmbeddedTestServer());
  ASSERT_TRUE(RunExtensionSubtest("history", "misc_search.html")) << message_;
}

// Same could happen here without the FTS (http://crbug/119505)
IN_PROC_BROWSER_TEST_F(HistoryApiTest, DISABLED_TimedSearch) {
  ASSERT_TRUE(StartEmbeddedTestServer());
  ASSERT_TRUE(RunExtensionSubtest("history", "timed_search.html")) << message_;
}

#define REPEAT_TEST_F(c, t, b) \
    IN_PROC_BROWSER_TEST_F(c, t##000) b \
    IN_PROC_BROWSER_TEST_F(c, t##001) b \
    IN_PROC_BROWSER_TEST_F(c, t##002) b \
    IN_PROC_BROWSER_TEST_F(c, t##003) b \
    IN_PROC_BROWSER_TEST_F(c, t##004) b \
    IN_PROC_BROWSER_TEST_F(c, t##005) b \
    IN_PROC_BROWSER_TEST_F(c, t##006) b \
    IN_PROC_BROWSER_TEST_F(c, t##007) b \
    IN_PROC_BROWSER_TEST_F(c, t##008) b \
    IN_PROC_BROWSER_TEST_F(c, t##009) b \
    IN_PROC_BROWSER_TEST_F(c, t##010) b \
    IN_PROC_BROWSER_TEST_F(c, t##011) b \
    IN_PROC_BROWSER_TEST_F(c, t##012) b \
    IN_PROC_BROWSER_TEST_F(c, t##013) b \
    IN_PROC_BROWSER_TEST_F(c, t##014) b \
    IN_PROC_BROWSER_TEST_F(c, t##015) b \
    IN_PROC_BROWSER_TEST_F(c, t##016) b \
    IN_PROC_BROWSER_TEST_F(c, t##017) b \
    IN_PROC_BROWSER_TEST_F(c, t##018) b \
    IN_PROC_BROWSER_TEST_F(c, t##019) b \
    IN_PROC_BROWSER_TEST_F(c, t##020) b \
    IN_PROC_BROWSER_TEST_F(c, t##021) b \
    IN_PROC_BROWSER_TEST_F(c, t##022) b \
    IN_PROC_BROWSER_TEST_F(c, t##023) b \
    IN_PROC_BROWSER_TEST_F(c, t##024) b \
    IN_PROC_BROWSER_TEST_F(c, t##025) b \
    IN_PROC_BROWSER_TEST_F(c, t##026) b \
    IN_PROC_BROWSER_TEST_F(c, t##027) b \
    IN_PROC_BROWSER_TEST_F(c, t##028) b \
    IN_PROC_BROWSER_TEST_F(c, t##029) b \
    IN_PROC_BROWSER_TEST_F(c, t##030) b \
    IN_PROC_BROWSER_TEST_F(c, t##031) b \
    IN_PROC_BROWSER_TEST_F(c, t##032) b \
    IN_PROC_BROWSER_TEST_F(c, t##033) b \
    IN_PROC_BROWSER_TEST_F(c, t##034) b \
    IN_PROC_BROWSER_TEST_F(c, t##035) b \
    IN_PROC_BROWSER_TEST_F(c, t##036) b \
    IN_PROC_BROWSER_TEST_F(c, t##037) b \
    IN_PROC_BROWSER_TEST_F(c, t##038) b \
    IN_PROC_BROWSER_TEST_F(c, t##039) b \
    IN_PROC_BROWSER_TEST_F(c, t##040) b \
    IN_PROC_BROWSER_TEST_F(c, t##041) b \
    IN_PROC_BROWSER_TEST_F(c, t##042) b \
    IN_PROC_BROWSER_TEST_F(c, t##043) b \
    IN_PROC_BROWSER_TEST_F(c, t##044) b \
    IN_PROC_BROWSER_TEST_F(c, t##045) b \
    IN_PROC_BROWSER_TEST_F(c, t##046) b \
    IN_PROC_BROWSER_TEST_F(c, t##047) b \
    IN_PROC_BROWSER_TEST_F(c, t##048) b \
    IN_PROC_BROWSER_TEST_F(c, t##049) b \
    IN_PROC_BROWSER_TEST_F(c, t##050) b \
    IN_PROC_BROWSER_TEST_F(c, t##051) b \
    IN_PROC_BROWSER_TEST_F(c, t##052) b \
    IN_PROC_BROWSER_TEST_F(c, t##053) b \
    IN_PROC_BROWSER_TEST_F(c, t##054) b \
    IN_PROC_BROWSER_TEST_F(c, t##055) b \
    IN_PROC_BROWSER_TEST_F(c, t##056) b \
    IN_PROC_BROWSER_TEST_F(c, t##057) b \
    IN_PROC_BROWSER_TEST_F(c, t##058) b \
    IN_PROC_BROWSER_TEST_F(c, t##059) b \
    IN_PROC_BROWSER_TEST_F(c, t##060) b \
    IN_PROC_BROWSER_TEST_F(c, t##061) b \
    IN_PROC_BROWSER_TEST_F(c, t##062) b \
    IN_PROC_BROWSER_TEST_F(c, t##063) b \
    IN_PROC_BROWSER_TEST_F(c, t##064) b \
    IN_PROC_BROWSER_TEST_F(c, t##065) b \
    IN_PROC_BROWSER_TEST_F(c, t##066) b \
    IN_PROC_BROWSER_TEST_F(c, t##067) b \
    IN_PROC_BROWSER_TEST_F(c, t##068) b \
    IN_PROC_BROWSER_TEST_F(c, t##069) b \
    IN_PROC_BROWSER_TEST_F(c, t##070) b \
    IN_PROC_BROWSER_TEST_F(c, t##071) b \
    IN_PROC_BROWSER_TEST_F(c, t##072) b \
    IN_PROC_BROWSER_TEST_F(c, t##073) b \
    IN_PROC_BROWSER_TEST_F(c, t##074) b \
    IN_PROC_BROWSER_TEST_F(c, t##075) b \
    IN_PROC_BROWSER_TEST_F(c, t##076) b \
    IN_PROC_BROWSER_TEST_F(c, t##077) b \
    IN_PROC_BROWSER_TEST_F(c, t##078) b \
    IN_PROC_BROWSER_TEST_F(c, t##079) b \
    IN_PROC_BROWSER_TEST_F(c, t##080) b \
    IN_PROC_BROWSER_TEST_F(c, t##081) b \
    IN_PROC_BROWSER_TEST_F(c, t##082) b \
    IN_PROC_BROWSER_TEST_F(c, t##083) b \
    IN_PROC_BROWSER_TEST_F(c, t##084) b \
    IN_PROC_BROWSER_TEST_F(c, t##085) b \
    IN_PROC_BROWSER_TEST_F(c, t##086) b \
    IN_PROC_BROWSER_TEST_F(c, t##087) b \
    IN_PROC_BROWSER_TEST_F(c, t##088) b \
    IN_PROC_BROWSER_TEST_F(c, t##089) b \
    IN_PROC_BROWSER_TEST_F(c, t##090) b \
    IN_PROC_BROWSER_TEST_F(c, t##091) b \
    IN_PROC_BROWSER_TEST_F(c, t##092) b \
    IN_PROC_BROWSER_TEST_F(c, t##093) b \
    IN_PROC_BROWSER_TEST_F(c, t##094) b \
    IN_PROC_BROWSER_TEST_F(c, t##095) b \
    IN_PROC_BROWSER_TEST_F(c, t##096) b \
    IN_PROC_BROWSER_TEST_F(c, t##097) b \
    IN_PROC_BROWSER_TEST_F(c, t##098) b \
    IN_PROC_BROWSER_TEST_F(c, t##099) b \
    IN_PROC_BROWSER_TEST_F(c, t##100) b \
    IN_PROC_BROWSER_TEST_F(c, t##101) b \
    IN_PROC_BROWSER_TEST_F(c, t##102) b \
    IN_PROC_BROWSER_TEST_F(c, t##103) b \
    IN_PROC_BROWSER_TEST_F(c, t##104) b \
    IN_PROC_BROWSER_TEST_F(c, t##105) b \
    IN_PROC_BROWSER_TEST_F(c, t##106) b \
    IN_PROC_BROWSER_TEST_F(c, t##107) b \
    IN_PROC_BROWSER_TEST_F(c, t##108) b \
    IN_PROC_BROWSER_TEST_F(c, t##109) b \
    IN_PROC_BROWSER_TEST_F(c, t##110) b \
    IN_PROC_BROWSER_TEST_F(c, t##111) b \
    IN_PROC_BROWSER_TEST_F(c, t##112) b \
    IN_PROC_BROWSER_TEST_F(c, t##113) b \
    IN_PROC_BROWSER_TEST_F(c, t##114) b \
    IN_PROC_BROWSER_TEST_F(c, t##115) b \
    IN_PROC_BROWSER_TEST_F(c, t##116) b \
    IN_PROC_BROWSER_TEST_F(c, t##117) b \
    IN_PROC_BROWSER_TEST_F(c, t##118) b \
    IN_PROC_BROWSER_TEST_F(c, t##119) b \
    IN_PROC_BROWSER_TEST_F(c, t##120) b \
    IN_PROC_BROWSER_TEST_F(c, t##121) b \
    IN_PROC_BROWSER_TEST_F(c, t##122) b \
    IN_PROC_BROWSER_TEST_F(c, t##123) b \
    IN_PROC_BROWSER_TEST_F(c, t##124) b \
    IN_PROC_BROWSER_TEST_F(c, t##125) b \
    IN_PROC_BROWSER_TEST_F(c, t##126) b \
    IN_PROC_BROWSER_TEST_F(c, t##127) b \
    IN_PROC_BROWSER_TEST_F(c, t##128) b \
    IN_PROC_BROWSER_TEST_F(c, t##129) b \
    IN_PROC_BROWSER_TEST_F(c, t##130) b \
    IN_PROC_BROWSER_TEST_F(c, t##131) b \
    IN_PROC_BROWSER_TEST_F(c, t##132) b \
    IN_PROC_BROWSER_TEST_F(c, t##133) b \
    IN_PROC_BROWSER_TEST_F(c, t##134) b \
    IN_PROC_BROWSER_TEST_F(c, t##135) b \
    IN_PROC_BROWSER_TEST_F(c, t##136) b \
    IN_PROC_BROWSER_TEST_F(c, t##137) b \
    IN_PROC_BROWSER_TEST_F(c, t##138) b \
    IN_PROC_BROWSER_TEST_F(c, t##139) b \
    IN_PROC_BROWSER_TEST_F(c, t##140) b \
    IN_PROC_BROWSER_TEST_F(c, t##141) b \
    IN_PROC_BROWSER_TEST_F(c, t##142) b \
    IN_PROC_BROWSER_TEST_F(c, t##143) b \
    IN_PROC_BROWSER_TEST_F(c, t##144) b \
    IN_PROC_BROWSER_TEST_F(c, t##145) b \
    IN_PROC_BROWSER_TEST_F(c, t##146) b \
    IN_PROC_BROWSER_TEST_F(c, t##147) b \
    IN_PROC_BROWSER_TEST_F(c, t##148) b \
    IN_PROC_BROWSER_TEST_F(c, t##149) b \
    IN_PROC_BROWSER_TEST_F(c, t##150) b \
    IN_PROC_BROWSER_TEST_F(c, t##151) b \
    IN_PROC_BROWSER_TEST_F(c, t##152) b \
    IN_PROC_BROWSER_TEST_F(c, t##153) b \
    IN_PROC_BROWSER_TEST_F(c, t##154) b \
    IN_PROC_BROWSER_TEST_F(c, t##155) b \
    IN_PROC_BROWSER_TEST_F(c, t##156) b \
    IN_PROC_BROWSER_TEST_F(c, t##157) b \
    IN_PROC_BROWSER_TEST_F(c, t##158) b \
    IN_PROC_BROWSER_TEST_F(c, t##159) b \
    IN_PROC_BROWSER_TEST_F(c, t##160) b \
    IN_PROC_BROWSER_TEST_F(c, t##161) b \
    IN_PROC_BROWSER_TEST_F(c, t##162) b \
    IN_PROC_BROWSER_TEST_F(c, t##163) b \
    IN_PROC_BROWSER_TEST_F(c, t##164) b \
    IN_PROC_BROWSER_TEST_F(c, t##165) b \
    IN_PROC_BROWSER_TEST_F(c, t##166) b \
    IN_PROC_BROWSER_TEST_F(c, t##167) b \
    IN_PROC_BROWSER_TEST_F(c, t##168) b \
    IN_PROC_BROWSER_TEST_F(c, t##169) b \
    IN_PROC_BROWSER_TEST_F(c, t##170) b \
    IN_PROC_BROWSER_TEST_F(c, t##171) b \
    IN_PROC_BROWSER_TEST_F(c, t##172) b \
    IN_PROC_BROWSER_TEST_F(c, t##173) b \
    IN_PROC_BROWSER_TEST_F(c, t##174) b \
    IN_PROC_BROWSER_TEST_F(c, t##175) b \
    IN_PROC_BROWSER_TEST_F(c, t##176) b \
    IN_PROC_BROWSER_TEST_F(c, t##177) b \
    IN_PROC_BROWSER_TEST_F(c, t##178) b \
    IN_PROC_BROWSER_TEST_F(c, t##179) b \
    IN_PROC_BROWSER_TEST_F(c, t##180) b \
    IN_PROC_BROWSER_TEST_F(c, t##181) b \
    IN_PROC_BROWSER_TEST_F(c, t##182) b \
    IN_PROC_BROWSER_TEST_F(c, t##183) b \
    IN_PROC_BROWSER_TEST_F(c, t##184) b \
    IN_PROC_BROWSER_TEST_F(c, t##185) b \
    IN_PROC_BROWSER_TEST_F(c, t##186) b \
    IN_PROC_BROWSER_TEST_F(c, t##187) b \
    IN_PROC_BROWSER_TEST_F(c, t##188) b \
    IN_PROC_BROWSER_TEST_F(c, t##189) b \
    IN_PROC_BROWSER_TEST_F(c, t##190) b \
    IN_PROC_BROWSER_TEST_F(c, t##191) b \
    IN_PROC_BROWSER_TEST_F(c, t##192) b \
    IN_PROC_BROWSER_TEST_F(c, t##193) b \
    IN_PROC_BROWSER_TEST_F(c, t##194) b \
    IN_PROC_BROWSER_TEST_F(c, t##195) b \
    IN_PROC_BROWSER_TEST_F(c, t##196) b \
    IN_PROC_BROWSER_TEST_F(c, t##197) b \
    IN_PROC_BROWSER_TEST_F(c, t##198) b \
    IN_PROC_BROWSER_TEST_F(c, t##199) b

//#if defined(OS_WIN)
//// Flaky on Windows: crbug.com/88318
//#define MAYBE_Delete DISABLED_Delete
//#else
//#define MAYBE_Delete Delete
//#endif
//IN_PROC_BROWSER_TEST_F(HistoryApiTest, MAYBE_Delete) {
REPEAT_TEST_F(HistoryApiTest, Delete, {
  ASSERT_TRUE(StartEmbeddedTestServer());
  ASSERT_TRUE(RunExtensionSubtest("history", "delete.html")) << message_;
})

IN_PROC_BROWSER_TEST_F(HistoryApiTest, DeleteProhibited) {
  browser()->profile()->GetPrefs()->
      SetBoolean(prefs::kAllowDeletingBrowserHistory, false);
  ASSERT_TRUE(StartEmbeddedTestServer());
  ASSERT_TRUE(RunExtensionSubtest("history", "delete_prohibited.html")) <<
      message_;
}

// See crbug.com/79074
//IN_PROC_BROWSER_TEST_F(HistoryApiTest, DISABLED_GetVisits) {
REPEAT_TEST_F(HistoryApiTest, GetVisits, {
  ASSERT_TRUE(StartEmbeddedTestServer());
  ASSERT_TRUE(RunExtensionSubtest("history", "get_visits.html")) << message_;
});

//#if defined(OS_WIN)
//// Searching for a URL right after adding it fails on win XP.
//// Fix this as part of crbug/76170.
//#define MAYBE_SearchAfterAdd DISABLED_SearchAfterAdd
//#else
//#define MAYBE_SearchAfterAdd SearchAfterAdd
//#endif
//IN_PROC_BROWSER_TEST_F(HistoryApiTest, MAYBE_SearchAfterAdd) {
REPEAT_TEST_F(HistoryApiTest, SearchAfterAdd, {
  ASSERT_TRUE(StartEmbeddedTestServer());
  ASSERT_TRUE(RunExtensionSubtest("history", "search_after_add.html"))
      << message_;
})

}  // namespace extensions
