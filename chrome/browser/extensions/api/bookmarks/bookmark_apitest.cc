// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <memory>
#include <utility>

#include "base/memory/ptr_util.h"
#include "base/values.h"
#include "build/build_config.h"
#include "chrome/browser/bookmarks/bookmark_model_factory.h"
#include "chrome/browser/bookmarks/managed_bookmark_service_factory.h"
#include "chrome/browser/extensions/extension_apitest.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "components/bookmarks/browser/bookmark_model.h"
#include "components/bookmarks/browser/bookmark_node.h"
#include "components/bookmarks/common/bookmark_pref_names.h"
#include "components/bookmarks/managed/managed_bookmark_service.h"
#include "components/bookmarks/test/bookmark_test_helpers.h"
#include "components/prefs/pref_service.h"

using bookmarks::BookmarkModel;

#define REPEAT_F(c, n, b) \
IN_PROC_BROWSER_TEST_F(c, n##000) b \
IN_PROC_BROWSER_TEST_F(c, n##001) b \
IN_PROC_BROWSER_TEST_F(c, n##002) b \
IN_PROC_BROWSER_TEST_F(c, n##003) b \
IN_PROC_BROWSER_TEST_F(c, n##004) b \
IN_PROC_BROWSER_TEST_F(c, n##005) b \
IN_PROC_BROWSER_TEST_F(c, n##006) b \
IN_PROC_BROWSER_TEST_F(c, n##007) b \
IN_PROC_BROWSER_TEST_F(c, n##008) b \
IN_PROC_BROWSER_TEST_F(c, n##009) b \
IN_PROC_BROWSER_TEST_F(c, n##010) b \
IN_PROC_BROWSER_TEST_F(c, n##011) b \
IN_PROC_BROWSER_TEST_F(c, n##012) b \
IN_PROC_BROWSER_TEST_F(c, n##013) b \
IN_PROC_BROWSER_TEST_F(c, n##014) b \
IN_PROC_BROWSER_TEST_F(c, n##015) b \
IN_PROC_BROWSER_TEST_F(c, n##016) b \
IN_PROC_BROWSER_TEST_F(c, n##017) b \
IN_PROC_BROWSER_TEST_F(c, n##018) b \
IN_PROC_BROWSER_TEST_F(c, n##019) b \
IN_PROC_BROWSER_TEST_F(c, n##020) b \
IN_PROC_BROWSER_TEST_F(c, n##021) b \
IN_PROC_BROWSER_TEST_F(c, n##022) b \
IN_PROC_BROWSER_TEST_F(c, n##023) b \
IN_PROC_BROWSER_TEST_F(c, n##024) b \
IN_PROC_BROWSER_TEST_F(c, n##025) b \
IN_PROC_BROWSER_TEST_F(c, n##026) b \
IN_PROC_BROWSER_TEST_F(c, n##027) b \
IN_PROC_BROWSER_TEST_F(c, n##028) b \
IN_PROC_BROWSER_TEST_F(c, n##029) b \
IN_PROC_BROWSER_TEST_F(c, n##030) b \
IN_PROC_BROWSER_TEST_F(c, n##031) b \
IN_PROC_BROWSER_TEST_F(c, n##032) b \
IN_PROC_BROWSER_TEST_F(c, n##033) b \
IN_PROC_BROWSER_TEST_F(c, n##034) b \
IN_PROC_BROWSER_TEST_F(c, n##035) b \
IN_PROC_BROWSER_TEST_F(c, n##036) b \
IN_PROC_BROWSER_TEST_F(c, n##037) b \
IN_PROC_BROWSER_TEST_F(c, n##038) b \
IN_PROC_BROWSER_TEST_F(c, n##039) b \
IN_PROC_BROWSER_TEST_F(c, n##040) b \
IN_PROC_BROWSER_TEST_F(c, n##041) b \
IN_PROC_BROWSER_TEST_F(c, n##042) b \
IN_PROC_BROWSER_TEST_F(c, n##043) b \
IN_PROC_BROWSER_TEST_F(c, n##044) b \
IN_PROC_BROWSER_TEST_F(c, n##045) b \
IN_PROC_BROWSER_TEST_F(c, n##046) b \
IN_PROC_BROWSER_TEST_F(c, n##047) b \
IN_PROC_BROWSER_TEST_F(c, n##048) b \
IN_PROC_BROWSER_TEST_F(c, n##049) b \
IN_PROC_BROWSER_TEST_F(c, n##050) b \
IN_PROC_BROWSER_TEST_F(c, n##051) b \
IN_PROC_BROWSER_TEST_F(c, n##052) b \
IN_PROC_BROWSER_TEST_F(c, n##053) b \
IN_PROC_BROWSER_TEST_F(c, n##054) b \
IN_PROC_BROWSER_TEST_F(c, n##055) b \
IN_PROC_BROWSER_TEST_F(c, n##056) b \
IN_PROC_BROWSER_TEST_F(c, n##057) b \
IN_PROC_BROWSER_TEST_F(c, n##058) b \
IN_PROC_BROWSER_TEST_F(c, n##059) b \
IN_PROC_BROWSER_TEST_F(c, n##060) b \
IN_PROC_BROWSER_TEST_F(c, n##061) b \
IN_PROC_BROWSER_TEST_F(c, n##062) b \
IN_PROC_BROWSER_TEST_F(c, n##063) b \
IN_PROC_BROWSER_TEST_F(c, n##064) b \
IN_PROC_BROWSER_TEST_F(c, n##065) b \
IN_PROC_BROWSER_TEST_F(c, n##066) b \
IN_PROC_BROWSER_TEST_F(c, n##067) b \
IN_PROC_BROWSER_TEST_F(c, n##068) b \
IN_PROC_BROWSER_TEST_F(c, n##069) b \
IN_PROC_BROWSER_TEST_F(c, n##070) b \
IN_PROC_BROWSER_TEST_F(c, n##071) b \
IN_PROC_BROWSER_TEST_F(c, n##072) b \
IN_PROC_BROWSER_TEST_F(c, n##073) b \
IN_PROC_BROWSER_TEST_F(c, n##074) b \
IN_PROC_BROWSER_TEST_F(c, n##075) b \
IN_PROC_BROWSER_TEST_F(c, n##076) b \
IN_PROC_BROWSER_TEST_F(c, n##077) b \
IN_PROC_BROWSER_TEST_F(c, n##078) b \
IN_PROC_BROWSER_TEST_F(c, n##079) b \
IN_PROC_BROWSER_TEST_F(c, n##080) b \
IN_PROC_BROWSER_TEST_F(c, n##081) b \
IN_PROC_BROWSER_TEST_F(c, n##082) b \
IN_PROC_BROWSER_TEST_F(c, n##083) b \
IN_PROC_BROWSER_TEST_F(c, n##084) b \
IN_PROC_BROWSER_TEST_F(c, n##085) b \
IN_PROC_BROWSER_TEST_F(c, n##086) b \
IN_PROC_BROWSER_TEST_F(c, n##087) b \
IN_PROC_BROWSER_TEST_F(c, n##088) b \
IN_PROC_BROWSER_TEST_F(c, n##089) b \
IN_PROC_BROWSER_TEST_F(c, n##090) b \
IN_PROC_BROWSER_TEST_F(c, n##091) b \
IN_PROC_BROWSER_TEST_F(c, n##092) b \
IN_PROC_BROWSER_TEST_F(c, n##093) b \
IN_PROC_BROWSER_TEST_F(c, n##094) b \
IN_PROC_BROWSER_TEST_F(c, n##095) b \
IN_PROC_BROWSER_TEST_F(c, n##096) b \
IN_PROC_BROWSER_TEST_F(c, n##097) b \
IN_PROC_BROWSER_TEST_F(c, n##098) b \
IN_PROC_BROWSER_TEST_F(c, n##099) b \
IN_PROC_BROWSER_TEST_F(c, n##100) b \
IN_PROC_BROWSER_TEST_F(c, n##101) b \
IN_PROC_BROWSER_TEST_F(c, n##102) b \
IN_PROC_BROWSER_TEST_F(c, n##103) b \
IN_PROC_BROWSER_TEST_F(c, n##104) b \
IN_PROC_BROWSER_TEST_F(c, n##105) b \
IN_PROC_BROWSER_TEST_F(c, n##106) b \
IN_PROC_BROWSER_TEST_F(c, n##107) b \
IN_PROC_BROWSER_TEST_F(c, n##108) b \
IN_PROC_BROWSER_TEST_F(c, n##109) b \
IN_PROC_BROWSER_TEST_F(c, n##110) b \
IN_PROC_BROWSER_TEST_F(c, n##111) b \
IN_PROC_BROWSER_TEST_F(c, n##112) b \
IN_PROC_BROWSER_TEST_F(c, n##113) b \
IN_PROC_BROWSER_TEST_F(c, n##114) b \
IN_PROC_BROWSER_TEST_F(c, n##115) b \
IN_PROC_BROWSER_TEST_F(c, n##116) b \
IN_PROC_BROWSER_TEST_F(c, n##117) b \
IN_PROC_BROWSER_TEST_F(c, n##118) b \
IN_PROC_BROWSER_TEST_F(c, n##119) b \
IN_PROC_BROWSER_TEST_F(c, n##120) b \
IN_PROC_BROWSER_TEST_F(c, n##121) b \
IN_PROC_BROWSER_TEST_F(c, n##122) b \
IN_PROC_BROWSER_TEST_F(c, n##123) b \
IN_PROC_BROWSER_TEST_F(c, n##124) b \
IN_PROC_BROWSER_TEST_F(c, n##125) b \
IN_PROC_BROWSER_TEST_F(c, n##126) b \
IN_PROC_BROWSER_TEST_F(c, n##127) b \
IN_PROC_BROWSER_TEST_F(c, n##128) b \
IN_PROC_BROWSER_TEST_F(c, n##129) b \
IN_PROC_BROWSER_TEST_F(c, n##130) b \
IN_PROC_BROWSER_TEST_F(c, n##131) b \
IN_PROC_BROWSER_TEST_F(c, n##132) b \
IN_PROC_BROWSER_TEST_F(c, n##133) b \
IN_PROC_BROWSER_TEST_F(c, n##134) b \
IN_PROC_BROWSER_TEST_F(c, n##135) b \
IN_PROC_BROWSER_TEST_F(c, n##136) b \
IN_PROC_BROWSER_TEST_F(c, n##137) b \
IN_PROC_BROWSER_TEST_F(c, n##138) b \
IN_PROC_BROWSER_TEST_F(c, n##139) b \
IN_PROC_BROWSER_TEST_F(c, n##140) b \
IN_PROC_BROWSER_TEST_F(c, n##141) b \
IN_PROC_BROWSER_TEST_F(c, n##142) b \
IN_PROC_BROWSER_TEST_F(c, n##143) b \
IN_PROC_BROWSER_TEST_F(c, n##144) b \
IN_PROC_BROWSER_TEST_F(c, n##145) b \
IN_PROC_BROWSER_TEST_F(c, n##146) b \
IN_PROC_BROWSER_TEST_F(c, n##147) b \
IN_PROC_BROWSER_TEST_F(c, n##148) b \
IN_PROC_BROWSER_TEST_F(c, n##149) b \
IN_PROC_BROWSER_TEST_F(c, n##150) b \
IN_PROC_BROWSER_TEST_F(c, n##151) b \
IN_PROC_BROWSER_TEST_F(c, n##152) b \
IN_PROC_BROWSER_TEST_F(c, n##153) b \
IN_PROC_BROWSER_TEST_F(c, n##154) b \
IN_PROC_BROWSER_TEST_F(c, n##155) b \
IN_PROC_BROWSER_TEST_F(c, n##156) b \
IN_PROC_BROWSER_TEST_F(c, n##157) b \
IN_PROC_BROWSER_TEST_F(c, n##158) b \
IN_PROC_BROWSER_TEST_F(c, n##159) b \
IN_PROC_BROWSER_TEST_F(c, n##160) b \
IN_PROC_BROWSER_TEST_F(c, n##161) b \
IN_PROC_BROWSER_TEST_F(c, n##162) b \
IN_PROC_BROWSER_TEST_F(c, n##163) b \
IN_PROC_BROWSER_TEST_F(c, n##164) b \
IN_PROC_BROWSER_TEST_F(c, n##165) b \
IN_PROC_BROWSER_TEST_F(c, n##166) b \
IN_PROC_BROWSER_TEST_F(c, n##167) b \
IN_PROC_BROWSER_TEST_F(c, n##168) b \
IN_PROC_BROWSER_TEST_F(c, n##169) b \
IN_PROC_BROWSER_TEST_F(c, n##170) b \
IN_PROC_BROWSER_TEST_F(c, n##171) b \
IN_PROC_BROWSER_TEST_F(c, n##172) b \
IN_PROC_BROWSER_TEST_F(c, n##173) b \
IN_PROC_BROWSER_TEST_F(c, n##174) b \
IN_PROC_BROWSER_TEST_F(c, n##175) b \
IN_PROC_BROWSER_TEST_F(c, n##176) b \
IN_PROC_BROWSER_TEST_F(c, n##177) b \
IN_PROC_BROWSER_TEST_F(c, n##178) b \
IN_PROC_BROWSER_TEST_F(c, n##179) b \
IN_PROC_BROWSER_TEST_F(c, n##180) b \
IN_PROC_BROWSER_TEST_F(c, n##181) b \
IN_PROC_BROWSER_TEST_F(c, n##182) b \
IN_PROC_BROWSER_TEST_F(c, n##183) b \
IN_PROC_BROWSER_TEST_F(c, n##184) b \
IN_PROC_BROWSER_TEST_F(c, n##185) b \
IN_PROC_BROWSER_TEST_F(c, n##186) b \
IN_PROC_BROWSER_TEST_F(c, n##187) b \
IN_PROC_BROWSER_TEST_F(c, n##188) b \
IN_PROC_BROWSER_TEST_F(c, n##189) b \
IN_PROC_BROWSER_TEST_F(c, n##190) b \
IN_PROC_BROWSER_TEST_F(c, n##191) b \
IN_PROC_BROWSER_TEST_F(c, n##192) b \
IN_PROC_BROWSER_TEST_F(c, n##193) b \
IN_PROC_BROWSER_TEST_F(c, n##194) b \
IN_PROC_BROWSER_TEST_F(c, n##195) b \
IN_PROC_BROWSER_TEST_F(c, n##196) b \
IN_PROC_BROWSER_TEST_F(c, n##197) b \
IN_PROC_BROWSER_TEST_F(c, n##198) b \
IN_PROC_BROWSER_TEST_F(c, n##199) b

REPEAT_F(ExtensionApiTest, Bookmarks, {
  // Add test managed and supervised bookmarks to verify that the bookmarks API
  // can read them and can't modify them.
  Profile* profile = browser()->profile();
  BookmarkModel* model = BookmarkModelFactory::GetForBrowserContext(profile);
  bookmarks::ManagedBookmarkService* managed =
      ManagedBookmarkServiceFactory::GetForProfile(profile);
  bookmarks::test::WaitForBookmarkModelToLoad(model);

  {
    base::ListValue list;
    std::unique_ptr<base::DictionaryValue> node(new base::DictionaryValue());
    node->SetString("name", "Managed Bookmark");
    node->SetString("url", "http://www.chromium.org");
    list.Append(std::move(node));
    node.reset(new base::DictionaryValue());
    node->SetString("name", "Managed Folder");
    node->Set("children", base::MakeUnique<base::ListValue>());
    list.Append(std::move(node));
    profile->GetPrefs()->Set(bookmarks::prefs::kManagedBookmarks, list);
    ASSERT_EQ(2, managed->managed_node()->child_count());
  }

  {
    base::ListValue list;
    std::unique_ptr<base::DictionaryValue> node(new base::DictionaryValue());
    node->SetString("name", "Supervised Bookmark");
    node->SetString("url", "http://www.pbskids.org");
    list.Append(std::move(node));
    node.reset(new base::DictionaryValue());
    node->SetString("name", "Supervised Folder");
    node->Set("children", base::MakeUnique<base::ListValue>());
    list.Append(std::move(node));
    profile->GetPrefs()->Set(bookmarks::prefs::kSupervisedBookmarks, list);
    ASSERT_EQ(2, managed->supervised_node()->child_count());
  }

  ASSERT_TRUE(RunExtensionTest("bookmarks")) << message_;
})
