// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <string>

#include "base/files/file_path.h"
#include "base/memory/ptr_util.h"
#include "base/path_service.h"
#include "build/build_config.h"
#include "components/prefs/testing_pref_service.h"
#include "components/update_client/persisted_data.h"
#include "components/update_client/protocol_builder.h"
#include "components/update_client/updater_state.h"
#include "extensions/features/features.h"
#include "testing/gtest/include/gtest/gtest.h"

using std::string;

namespace update_client {

TEST(UpdateClientUtilsTest, BuildProtocolRequest_ProdIdVersion) {
  // Verifies that |prod_id| and |version| are serialized.
  const string request = BuildProtocolRequest("some_prod_id", "1.0", "", "", "",
                                              "", "", "", nullptr);
  EXPECT_NE(string::npos, request.find(" version=\"some_prod_id-1.0\" "));
}

TEST(UpdateClientUtilsTest, BuildProtocolRequest_DownloadPreference) {
  // Verifies that an empty |download_preference| is not serialized.
  const string request_no_dlpref =
      BuildProtocolRequest("", "", "", "", "", "", "", "", nullptr);
  EXPECT_EQ(string::npos, request_no_dlpref.find(" dlpref="));

  // Verifies that |download_preference| is serialized.
  const string request_with_dlpref =
      BuildProtocolRequest("", "", "", "", "", "some pref", "", "", nullptr);
  EXPECT_NE(string::npos, request_with_dlpref.find(" dlpref=\"some pref\""));
}

TEST(UpdateClientUtilsTest, BuildProtocolRequestUpdaterStateAttributes) {
  // When no updater state is provided, then check that the elements and
  // attributes related to the updater state are not serialized.
  std::string request =
      BuildProtocolRequest("", "", "", "", "", "", "", "", nullptr).c_str();
  EXPECT_EQ(std::string::npos, request.find(" domainjoined"));
  EXPECT_EQ(std::string::npos, request.find("<updater"));

  UpdaterState::Attributes attributes;
  attributes["domainjoined"] = "1";
  attributes["name"] = "Omaha";
  attributes["version"] = "1.2.3.4";
  attributes["laststarted"] = "1";
  attributes["lastchecked"] = "2";
  attributes["autoupdatecheckenabled"] = "0";
  attributes["updatepolicy"] = "-1";
  request = BuildProtocolRequest(
      "", "", "", "", "", "", "", "",
      base::MakeUnique<UpdaterState::Attributes>(attributes));
  EXPECT_NE(std::string::npos, request.find(" domainjoined=\"1\""));
  const std::string updater_element =
      "<updater autoupdatecheckenabled=\"0\" "
      "lastchecked=\"2\" laststarted=\"1\" name=\"Omaha\" "
      "updatepolicy=\"-1\" version=\"1.2.3.4\"/>";
#if defined(GOOGLE_CHROME_BUILD)
  EXPECT_NE(std::string::npos, request.find(updater_element));
#else
  EXPECT_EQ(std::string::npos, request.find(updater_element));
#endif  // GOOGLE_CHROME_BUILD
}

TEST(UpdateClientUtilsTest, BuildUpdateCheckPingElement) {
  std::unique_ptr<TestingPrefServiceSimple> pref(
      new TestingPrefServiceSimple());
  PersistedData::RegisterPrefs(pref->registry());
  std::unique_ptr<PersistedData> metadata(new PersistedData(pref.get()));
  std::string component_id("someappid");

  std::string ping = BuildUpdateCheckPingElement(metadata.get(), component_id);

#if BUILDFLAG(ENABLE_EXTENSIONS)
  EXPECT_EQ("<ping rd=\"-1\" r=\"-1\" ping_freshness=\"\"/>", ping);

  // Fall back on r if rd is not available.
  metadata->SetLastRollCallTime(
      component_id, base::Time::Now() - base::TimeDelta::FromDays(2));
  ping = BuildUpdateCheckPingElement(metadata.get(), component_id);
  EXPECT_EQ("<ping rd=\"-2\" r=\"2\" ping_freshness=\"\"/>", ping);

  metadata->SetDateLastRollCall({component_id}, 123);
  ping = BuildUpdateCheckPingElement(metadata.get(), component_id);
  EXPECT_NE(std::string::npos,
            ping.find("<ping rd=\"123\" r=\"2\" ping_freshness="));

  metadata->SetActiveBit(component_id);
  ping = BuildUpdateCheckPingElement(metadata.get(), component_id);
  EXPECT_NE(
      std::string::npos,
      ping.find("<ping active=\"1\" ad=\"-1\" a=\"-1\" rd=\"123\" r=\"2\" "
                "ping_freshness="));

  metadata->SetLastActiveTime(component_id,
                              base::Time::Now() - base::TimeDelta::FromDays(4));
  ping = BuildUpdateCheckPingElement(metadata.get(), component_id);
  EXPECT_NE(std::string::npos,
            ping.find("<ping active=\"1\" ad=\"-2\" a=\"4\" rd=\"123\" r=\"2\" "
                      "ping_freshness="));

  metadata->SetDateLastRollCall({component_id}, 234);
  metadata->SetDateLastActive({component_id}, 200);
  ping = BuildUpdateCheckPingElement(metadata.get(), component_id);
  EXPECT_NE(
      std::string::npos,
      ping.find("<ping active=\"1\" ad=\"200\" a=\"4\" rd=\"234\" r=\"2\" "
                "ping_freshness="));

  metadata->ClearActiveBit({component_id});
  metadata->SetDateLastRollCall({component_id}, 345);
  ping = BuildUpdateCheckPingElement(metadata.get(), component_id);
  EXPECT_NE(std::string::npos,
            ping.find("<ping rd=\"345\" r=\"2\" ping_freshness="));
#else
  EXPECT_EQ("<ping rd=\"-2\" ping_freshness=\"\"/>", ping);

  metadata->SetDateLastRollCall({component_id}, 123);
  ping = BuildUpdateCheckPingElement(metadata.get(), component_id);
  EXPECT_NE(std::string::npos, ping.find("<ping rd=\"123\" ping_freshness="));
#endif
}

}  // namespace update_client
