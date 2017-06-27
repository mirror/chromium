// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/loader/modulescript/ModuleScriptData.h"
#include "core/workers/WorkletModuleResponsesMap.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace blink {

namespace {

void ReadCallback(bool* was_called,
                  WorkletModuleResponsesMap::ReadStatus* status_out,
                  WTF::Optional<ModuleScriptData>* data_out,
                  WorkletModuleResponsesMap::ReadStatus status,
                  WTF::Optional<ModuleScriptData> data) {
  *was_called = true;
  *status_out = status;
  *data_out = data;
}

}  // namespace

TEST(WorkletModuleResponsesMapTest, Basic) {
  WorkletModuleResponsesMap* map = new WorkletModuleResponsesMap;
  KURL module_url(kParsedURLString, "https://example.com/foo.js");

  bool was_called1 = false;
  auto status1 = WorkletModuleResponsesMap::ReadStatus::kFailed;
  WTF::Optional<ModuleScriptData> actual_data1;

  // An initial read call creates an entry in the map and invokes the given
  // callback with an empty module script data.
  map->ReadOrCreateEntry(
      module_url,
      WTF::Bind(&ReadCallback, WTF::Unretained(&was_called1),
                WTF::Unretained(&status1), WTF::Unretained(&actual_data1)));
  EXPECT_TRUE(was_called1);
  EXPECT_EQ(WorkletModuleResponsesMap::ReadStatus::kNeedsFetching, status1);
  EXPECT_FALSE(actual_data1.has_value());

  // The entry in the map is now in the fetching state. Following read calls are
  // enqueued into the pending callback queue.
  bool was_called2 = false;
  auto status2 = WorkletModuleResponsesMap::ReadStatus::kFailed;
  WTF::Optional<ModuleScriptData> actual_data2;
  map->ReadOrCreateEntry(
      module_url,
      WTF::Bind(&ReadCallback, WTF::Unretained(&was_called2),
                WTF::Unretained(&status2), WTF::Unretained(&actual_data2)));
  EXPECT_FALSE(was_called2);
  EXPECT_EQ(WorkletModuleResponsesMap::ReadStatus::kFailed, status2);
  EXPECT_FALSE(actual_data2.has_value());

  bool was_called3 = false;
  auto status3 = WorkletModuleResponsesMap::ReadStatus::kFailed;
  WTF::Optional<ModuleScriptData> actual_data3;
  map->ReadOrCreateEntry(
      module_url,
      WTF::Bind(&ReadCallback, WTF::Unretained(&was_called3),
                WTF::Unretained(&status3), WTF::Unretained(&actual_data3)));
  EXPECT_FALSE(was_called3);
  EXPECT_EQ(WorkletModuleResponsesMap::ReadStatus::kFailed, status3);
  EXPECT_FALSE(actual_data3.has_value());

  // An update call invokes the pending read callbacks.
  ModuleScriptData data(module_url, "// dummy script",
                        WebURLRequest::kFetchCredentialsModeOmit,
                        kNotSharableCrossOrigin);
  map->UpdateEntry(module_url, data);
  EXPECT_TRUE(was_called2);
  EXPECT_EQ(WorkletModuleResponsesMap::ReadStatus::kOK, status2);
  EXPECT_TRUE(actual_data2.has_value());
  EXPECT_TRUE(was_called3);
  EXPECT_EQ(WorkletModuleResponsesMap::ReadStatus::kOK, status3);
  EXPECT_TRUE(actual_data3.has_value());
}

TEST(WorkletModuleResponsesMapTest, Failure) {
  WorkletModuleResponsesMap* map = new WorkletModuleResponsesMap;
  KURL module_url(kParsedURLString, "https://example.com/foo.js");

  bool was_called1 = false;
  auto status1 = WorkletModuleResponsesMap::ReadStatus::kFailed;
  WTF::Optional<ModuleScriptData> actual_data1;

  // An initial read call creates an entry in the map and invokes the given
  // callback with an empty module script data.
  map->ReadOrCreateEntry(
      module_url,
      WTF::Bind(&ReadCallback, WTF::Unretained(&was_called1),
                WTF::Unretained(&status1), WTF::Unretained(&actual_data1)));
  EXPECT_TRUE(was_called1);
  EXPECT_EQ(WorkletModuleResponsesMap::ReadStatus::kNeedsFetching, status1);
  EXPECT_FALSE(actual_data1.has_value());

  // The entry in the map is now in the fetching state. Following read calls are
  // enqueued into the pending callback queue.
  bool was_called2 = false;
  auto status2 = WorkletModuleResponsesMap::ReadStatus::kFailed;
  WTF::Optional<ModuleScriptData> actual_data2;
  map->ReadOrCreateEntry(
      module_url,
      WTF::Bind(&ReadCallback, WTF::Unretained(&was_called2),
                WTF::Unretained(&status2), WTF::Unretained(&actual_data2)));
  EXPECT_FALSE(was_called2);
  EXPECT_EQ(WorkletModuleResponsesMap::ReadStatus::kFailed, status2);
  EXPECT_FALSE(actual_data2.has_value());

  bool was_called3 = false;
  auto status3 = WorkletModuleResponsesMap::ReadStatus::kFailed;
  WTF::Optional<ModuleScriptData> actual_data3;
  map->ReadOrCreateEntry(
      module_url,
      WTF::Bind(&ReadCallback, WTF::Unretained(&was_called3),
                WTF::Unretained(&status3), WTF::Unretained(&actual_data3)));
  EXPECT_FALSE(was_called3);
  EXPECT_EQ(WorkletModuleResponsesMap::ReadStatus::kFailed, status3);
  EXPECT_FALSE(actual_data3.has_value());

  // An invalidation call fails the pending read callbacks.
  map->InvalidateEntry(module_url);
  EXPECT_TRUE(was_called2);
  EXPECT_EQ(WorkletModuleResponsesMap::ReadStatus::kFailed, status2);
  EXPECT_FALSE(actual_data2.has_value());
  EXPECT_TRUE(was_called3);
  EXPECT_EQ(WorkletModuleResponsesMap::ReadStatus::kFailed, status3);
  EXPECT_FALSE(actual_data3.has_value());
}

TEST(WorkletModuleResponsesMapTest, Isolation) {
  WorkletModuleResponsesMap* map = new WorkletModuleResponsesMap;
  KURL module_url1(kParsedURLString, "https://example.com/foo.js");
  KURL module_url2(kParsedURLString, "https://example.com/bar.js");

  bool was_called1 = false;
  auto status1 = WorkletModuleResponsesMap::ReadStatus::kFailed;
  WTF::Optional<ModuleScriptData> actual_data1;

  // An initial read call for |module_url1| creates an entry in the map and
  // invokes the given callback with an empty module script data.
  map->ReadOrCreateEntry(
      module_url1,
      WTF::Bind(&ReadCallback, WTF::Unretained(&was_called1),
                WTF::Unretained(&status1), WTF::Unretained(&actual_data1)));
  EXPECT_TRUE(was_called1);
  EXPECT_EQ(WorkletModuleResponsesMap::ReadStatus::kNeedsFetching, status1);
  EXPECT_FALSE(actual_data1.has_value());

  // The entry in the map is now in the fetching state. Following read calls for
  // |module_url1| are enqueued into the pending callback queue.
  bool was_called2 = false;
  auto status2 = WorkletModuleResponsesMap::ReadStatus::kFailed;
  WTF::Optional<ModuleScriptData> actual_data2;
  map->ReadOrCreateEntry(
      module_url1,
      WTF::Bind(&ReadCallback, WTF::Unretained(&was_called2),
                WTF::Unretained(&status2), WTF::Unretained(&actual_data2)));
  EXPECT_FALSE(was_called2);
  EXPECT_FALSE(actual_data2.has_value());

  // An initial read call for |module_url2| also creates an entry in the map.
  bool was_called3 = false;
  auto status3 = WorkletModuleResponsesMap::ReadStatus::kFailed;
  WTF::Optional<ModuleScriptData> actual_data3;
  map->ReadOrCreateEntry(
      module_url2,
      WTF::Bind(&ReadCallback, WTF::Unretained(&was_called3),
                WTF::Unretained(&status3), WTF::Unretained(&actual_data3)));
  EXPECT_TRUE(was_called3);
  EXPECT_EQ(WorkletModuleResponsesMap::ReadStatus::kNeedsFetching, status3);
  EXPECT_FALSE(actual_data3.has_value());

  // The entry in the map is now in the fetching state. Following read calls for
  // |module_url2| are enqueued into the pending callback queue.
  bool was_called4 = false;
  auto status4 = WorkletModuleResponsesMap::ReadStatus::kFailed;
  WTF::Optional<ModuleScriptData> actual_data4;
  map->ReadOrCreateEntry(
      module_url2,
      WTF::Bind(&ReadCallback, WTF::Unretained(&was_called4),
                WTF::Unretained(&status4), WTF::Unretained(&actual_data4)));
  EXPECT_FALSE(was_called4);
  EXPECT_FALSE(actual_data4.has_value());

  // The read call for |module_url2| call should not affect the other entry for
  // |module_url1|.
  EXPECT_FALSE(was_called2);
  EXPECT_FALSE(actual_data2.has_value());

  // An update call for |module_url2| invokes the pending read callbacks.
  ModuleScriptData data(module_url2, "// dummy script",
                        WebURLRequest::kFetchCredentialsModeOmit,
                        kNotSharableCrossOrigin);
  map->UpdateEntry(module_url2, data);
  EXPECT_TRUE(was_called4);
  EXPECT_EQ(WorkletModuleResponsesMap::ReadStatus::kOK, status4);
  EXPECT_TRUE(actual_data4.has_value());

  // The update call for |module_url2| should not affect the other entry for
  // |module_url1|.
  EXPECT_FALSE(was_called2);
  EXPECT_FALSE(actual_data2.has_value());
}

}  // namespace blink
