// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_SYNC_TEST_MODEL_TYPE_SYNC_BRIDGE_TEST_TEMPLATE_H_
#define COMPONENTS_SYNC_TEST_MODEL_TYPE_SYNC_BRIDGE_TEST_TEMPLATE_H_

#include <type_traits>

#include "base/message_loop/message_loop.h"
#include "base/strings/stringprintf.h"
#include "components/sync/model/data_batch.h"
#include "components/sync/model/model_type_sync_bridge.h"
#include "components/sync/model/recording_model_type_change_processor.h"
#include "components/sync/model/sync_metadata_store.h"
#include "components/sync/protocol/proto_visitors.h"
#include "components/sync/protocol/sync.pb.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace sync_pb {

class ProtoPrinter {
 public:
  ProtoPrinter(std::ostream* os, int indentation)
      : os_(os), indentation_(indentation), indent_str_(indentation * 2, ' ') {}

  template <class P>
  void VisitBytes(const P& parent_proto,
                  const char* field_name,
                  const std::string& field) {
    std::string value;
    for (char c : field) {
      base::StringAppendF(&value, "\\u%04X ", c);
    }
    *os_ << indent_str_ << field_name << ": " << value << "\n";
  }

  // Types derived from MessageLite (i.e. protos).
  template <class P, class F>
  typename std::enable_if<
      std::is_base_of<google::protobuf::MessageLite, F>::value>::type
  Visit(const P&, const char* field_name, const F& field) {
    *os_ << indent_str_ << field_name << " {\n";
    ProtoPrinter visitor(os_, indentation_ + 1);
    VisitProtoFields(visitor, field);
    *os_ << indent_str_ << "}\n";
  }

  // RepeatedPtrField.
  template <class P, class F>
  void Visit(const P& parent_proto,
             const char* field_name,
             const google::protobuf::RepeatedPtrField<F>& fields) {
    for (const auto& field : fields) {
      Visit(parent_proto, field_name, field);
    }
  }

  // RepeatedField.
  template <class P, class F>
  void Visit(const P& parent_proto,
             const char* field_name,
             const google::protobuf::RepeatedField<F>& fields) {
    for (const auto& field : fields) {
      Visit(parent_proto, field_name, field);
    }
  }

  template <class P, class E>
  void VisitEnum(const P& parent_proto, const char* field_name, E field) {
    Visit(parent_proto, field_name, field);
  }

  // std::string.
  template <class P>
  void Visit(const P&, const char* field_name, const std::string& field) {
    *os_ << indent_str_ << field_name << ": \"" << field << "\"\n";
  }

  // Default implementation.
  template <class P, class F>
  void Visit(const P&, const char* field_name, F field) {
    *os_ << indent_str_ << field_name << ": " << field << "\n";
  }

 private:
  std::ostream* os_;
  const int indentation_;
  const std::string indent_str_;
};

// TODO(mastiz): I haven't managed to use a function template to support all
// sync protobug types, so we might need to list them all here for
// human-readable error messages.
void PrintTo(const TypedUrlSpecifics& proto, ::std::ostream* os) {
  *os << "{\n";
  ProtoPrinter visitor(os, /*indentation=*/1);
  ::syncer::VisitProtoFields(visitor, proto);
  *os << "}";
}

}  // namespace sync_pb

namespace syncer {

MATCHER_P(EqualsProto, expected, "") {
  // Unfortunately, we only have MessageLite protocol buffers in Chrome, so
  // comparing via DebugString() or MessageDifferencer is not working.
  // So we either need to compare field-by-field (maintenance heavy) or
  // compare the binary version (unusable diagnostic). Deciding for the latter.
  std::string expected_serialized, actual_serialized;
  expected.SerializeToString(&expected_serialized);
  arg.SerializeToString(&actual_serialized);
  if (expected_serialized == actual_serialized) {
    return true;
  }
  if (result_listener->stream() != nullptr) {
    *result_listener << "Expected:\n";
    PrintTo(expected, result_listener->stream());
    *result_listener << "\nActual:\n";
    PrintTo(arg, result_listener->stream());
  }
  return false;
}

template <typename N, typename S>
struct ModelTypeSyncBridgeTester {
  using NATIVE_T = N;
  using SPECIFICS = S;

  // Equality verification (e.g. for a Put()):
  // static bool URLsEqual(const NATIVE_T& row, const SPECIFICS& specifics);
  // static bool NativesEqual(const NATIVE_T& lhs, const NATIVE_T& rhs);

  // Perhaps alternatively (too gmock jargon? let's prototype how this
  // implementation would look like) This would lead to much nices failure
  // messages (say, transition type mismatch). NOT SURE IF THIS IS ACTUALLY
  // NEEDED (not so far) Matcher<SPECIFICS>
  // NativeModelTester::GetEqualsSpecificsMatcher(const NATIVE_T&
  // expected_value); Matcher<NATIVE>
  // NativeModelTester::GetEqualsNativeMatcher(const NATIVE_T& expected_value);

  // static std::string GetStorageKey(const NATIVE_T& row);// {
  //  return row.id();
  //}

  static EntityDataPtr SpecificsToEntity(const SPECIFICS& specifics) {
    EntityData data;
    data.client_tag_hash = "ignored";
    *data.specifics.mutable_typed_url() = specifics;
    return data.PassToPtr();
  }

  static const SPECIFICS& SelectSpecifics(
      const sync_pb::EntitySpecifics& entity_specifics) {
    return entity_specifics.typed_url();
  }

  static testing::Matcher<EntityDataPtr> HasSpecifics(
      const SPECIFICS& expected) {
    return testing::Property(
        &EntityDataPtr::value,
        testing::Field(
            &EntityData::specifics,
            testing::ResultOf(&SelectSpecifics, EqualsProto(expected))));
  }

  struct LocalEntityData {
    std::string storage_key;
    NATIVE_T native;
    SPECIFICS specifics;
  };

  virtual ModelTypeSyncBridge* bridge() = 0;

  // Creation of entities that are also stored locally. On return, the entity
  // is present in the local/native model and all notifications have been
  // dispatched.
  virtual LocalEntityData CreateAndStoreNativeTestEntity1() = 0;
  virtual LocalEntityData CreateAndStoreNativeTestEntity2() = 0;
  LocalEntityData MutateAndStoreTestEntity1() {
    // TODO
    return LocalEntityData();
  }

  // Creation of entities that are remote only.
  struct RemoteEntityData {
    // No storage key exists for entities without a local storage.
    // NATIVE_T native;
    SPECIFICS specifics;
  };
  RemoteEntityData CreateRemoteOnlyTestEntity1() {
    // TODO
    return RemoteEntityData();
  }

  // Starts sync for the bridge with |initial_data| as the initial sync data.
  void StartSyncing(const std::vector<SPECIFICS>& initial_data) {
    EntityChangeList entity_change_list;
    for (const auto& specifics : initial_data) {
      EntityDataPtr entity_data = SpecificsToEntity(specifics);
      std::string storage_key;
      if (bridge()->SupportsGetStorageKey()) {
        storage_key = bridge()->GetStorageKey(entity_data.value());
      }
      entity_change_list.push_back(
          EntityChange::CreateAdd(storage_key, entity_data));
    }

    const auto error = bridge()->MergeSyncData(
        bridge()->CreateMetadataChangeList(), entity_change_list);
    EXPECT_FALSE(error);
  }
};

template <typename TESTER>
class ModelTypeSyncBridgeTest : public testing::Test {
 public:
  using SPECIFICS = typename TESTER::SPECIFICS;
  using NATIVE_T = typename TESTER::NATIVE_T;

  ModelTypeSyncBridgeTest()
      : tester_(
            RecordingModelTypeChangeProcessor::FactoryForBridgeTest(&processor_,
                                                                    false)) {}
  ~ModelTypeSyncBridgeTest() override {}

  TESTER* tester() { return &tester_; }
  ModelTypeSyncBridge* bridge() { return tester_.bridge(); }
  RecordingModelTypeChangeProcessor* processor() { return processor_; };

  std::map<std::string, SPECIFICS> GetAllDataFromBridge() {
    std::unique_ptr<DataBatch> got_batch;
    base::RunLoop loop;
    // TODO(mastiz): ReportError() should also quit the loop.
    bridge()->GetAllData(base::Bind(
        [](base::RunLoop* loop, std::unique_ptr<DataBatch>* got_batch,
           std::unique_ptr<DataBatch> batch) {
          *got_batch = std::move(batch);
          loop->Quit();
        },
        &loop, &got_batch));

    EXPECT_NE(nullptr, got_batch);

    std::map<std::string, SPECIFICS> storage_key_to_specifics;
    if (got_batch != nullptr) {
      while (got_batch->HasNext()) {
        const KeyAndData& pair = got_batch->Next();
        storage_key_to_specifics[pair.first] =
            TESTER::SelectSpecifics(pair.second->specifics);
      }
    }
    return storage_key_to_specifics;
  }

  SPECIFICS GetDataFromBridge(const std::string& storage_key) {
    std::unique_ptr<DataBatch> got_batch;
    base::RunLoop loop;
    // TODO(mastiz): ReportError() should also quit the loop.
    bridge()->GetData(
        {storage_key},
        base::Bind(
            [](base::RunLoop* loop, std::unique_ptr<DataBatch>* got_batch,
               std::unique_ptr<DataBatch> batch) {
              *got_batch = std::move(batch);
              loop->Quit();
            },
            &loop, &got_batch));

    loop.Run();
    EXPECT_NE(nullptr, got_batch);

    SPECIFICS specifics;
    if (got_batch != nullptr) {
      EXPECT_TRUE(got_batch->HasNext());
      const KeyAndData& pair = got_batch->Next();
      specifics = TESTER::SelectSpecifics(pair.second->specifics);
      EXPECT_FALSE(got_batch->HasNext());
    }
    return specifics;
  }

 private:
  // A non-owning pointer to the processor given to the bridge. Will be null
  // before being given to the bridge, to make ownership easier.
  RecordingModelTypeChangeProcessor* processor_ = nullptr;
  TESTER tester_;
};

TYPED_TEST_CASE_P(ModelTypeSyncBridgeTest);

TYPED_TEST_P(ModelTypeSyncBridgeTest, GetAllDataWithEmptyModel) {
  EXPECT_THAT(this->GetAllDataFromBridge(), testing::IsEmpty());
}

// Add two typed urls locally and verify bridge can get them from GetAllData.
// TODO / DONOTSUBMIT: Add dedicated test for the filtering of expired visits.
TYPED_TEST_P(ModelTypeSyncBridgeTest, GetAllData) {
  // Add two urls to backend.
  const auto entity1 = this->tester()->CreateAndStoreNativeTestEntity1();
  const auto entity2 = this->tester()->CreateAndStoreNativeTestEntity2();

  EXPECT_THAT(
      this->GetAllDataFromBridge(),
      testing::UnorderedElementsAre(
          testing::Pair(entity1.storage_key, EqualsProto(entity1.specifics)),
          testing::Pair(entity2.storage_key, EqualsProto(entity2.specifics))));
}

// Add two typed urls locally and verify bridge can get them from GetData.
TYPED_TEST_P(ModelTypeSyncBridgeTest, GetData) {
  // Add two urls to backend.
  const auto entity1 = this->tester()->CreateAndStoreNativeTestEntity1();
  const auto entity2 = this->tester()->CreateAndStoreNativeTestEntity2();

  EXPECT_THAT(this->GetDataFromBridge(entity1.storage_key),
              EqualsProto(entity1.specifics));
  EXPECT_THAT(this->GetDataFromBridge(entity2.storage_key),
              EqualsProto(entity2.specifics));
}

// Add a typed url locally and one to sync with the same data. Starting sync
// should result in no changes.
TYPED_TEST_P(ModelTypeSyncBridgeTest, MergeUrlNoChange) {
  const auto entity1 = this->tester()->CreateAndStoreNativeTestEntity1();

  // Create the same data in sync.
  this->tester()->StartSyncing({entity1.specifics});

  EXPECT_THAT(this->processor()->put_multimap(), testing::IsEmpty());

  std::string storage_key;
  if (this->bridge()->SupportsGetStorageKey()) {
    ASSERT_EQ(entity1.storage_key,
              this->bridge()->GetStorageKey(
                  TypeParam::SpecificsToEntity(entity1.specifics).value()));
    // For bridges that do support SupportsGetStorageKey(), no call to
    // UpdateStorageKey() is expected.
    ASSERT_THAT(this->processor()->update_multimap(), testing::IsEmpty());
  } else {
    // For bridges that do not support SupportsGetStorageKey(), a call to
    // UpdateStorageKey() is expected.
    ASSERT_THAT(
        this->processor()->update_multimap(),
        testing::ElementsAre(testing::Pair(
            entity1.storage_key, TypeParam::HasSpecifics(entity1.specifics))));
    storage_key = this->processor()->update_multimap().begin()->first;
  }

  // Check that the local cache was is still correct.
  EXPECT_THAT(this->GetAllDataFromBridge(),
              testing::ElementsAre(testing::Pair(
                  entity1.storage_key, EqualsProto(entity1.specifics))));
}

// Starting sync with no sync data should just push the local url to sync.
TYPED_TEST_P(ModelTypeSyncBridgeTest, MergeUrlEmptySync) {
  const auto entity1 = this->tester()->CreateAndStoreNativeTestEntity1();

  this->tester()->StartSyncing({});

  // Check that the local cache was is still correct.
  EXPECT_THAT(this->GetAllDataFromBridge(),
              testing::ElementsAre(testing::Pair(
                  entity1.storage_key, EqualsProto(entity1.specifics))));

  // Check that the server was updated correctly.
  EXPECT_THAT(
      this->processor()->put_multimap(),
      testing::ElementsAre(testing::Pair(
          entity1.storage_key, TypeParam::HasSpecifics(entity1.specifics))));
}

// Starting sync with no local data should just push the synced url into the
// backend.
TYPED_TEST_P(ModelTypeSyncBridgeTest, MergeUrlEmptyLocal) {
  // Create the sync data.
  const auto entity1 = this->tester()->CreateRemoteOnlyTestEntity1();

  this->tester()->StartSyncing({entity1.specifics});
  EXPECT_EQ(0u, this->processor()->put_multimap().size());

  std::string storage_key;
  if (this->bridge()->SupportsGetStorageKey()) {
    storage_key = this->bridge()->GetStorageKey(
        TypeParam::SpecificsToEntity(entity1.specifics).value());
    // For bridges that do support SupportsGetStorageKey(), no call to
    // UpdateStorageKey() is expected.
    ASSERT_THAT(storage_key, testing::Not(testing::IsEmpty()));
    ASSERT_THAT(this->processor()->update_multimap(), testing::IsEmpty());
  } else {
    // For bridges that do not support SupportsGetStorageKey(), a call to
    // UpdateStorageKey() is expected.
    ASSERT_THAT(this->processor()->update_multimap(),
                testing::ElementsAre(
                    testing::Pair(testing::Not(testing::IsEmpty()),
                                  TypeParam::HasSpecifics(entity1.specifics))));
    storage_key = this->processor()->update_multimap().begin()->first;
  }

  // Check that the backend was updated correctly.
  EXPECT_THAT(this->GetAllDataFromBridge(),
              testing::ElementsAre(
                  testing::Pair(storage_key, EqualsProto(entity1.specifics))));
}

// Starting sync with both local and sync have same typed URL, but different
// visit. After merge, both local and sync should have two same visits.
TYPED_TEST_P(ModelTypeSyncBridgeTest, SimpleMerge) {
  // TODO / DONOTSUBIT: This one seems hard to generalize.
}

// Create a local typed URL with one TYPED visit after sync has started. Check
// that sync is sent an ADD change for the new URL.
TYPED_TEST_P(ModelTypeSyncBridgeTest, AddLocalTypedUrl) {
  this->tester()->StartSyncing({});

  // Create a local entity that is not already in sync. Check that sync is sent
  // an ADD change for the existing URL.
  const auto entity1 = this->tester()->CreateAndStoreNativeTestEntity1();

  EXPECT_THAT(
      this->processor()->put_multimap(),
      testing::ElementsAre(testing::Pair(
          entity1.storage_key, TypeParam::HasSpecifics(entity1.specifics))));
}

// Update a local typed URL that is already synced. Check that sync is sent an
// UPDATE for the existing url,
// TODO / DONOTSUBMIT: Add dedicate test for ...but RELOAD visits aren't synced.
TYPED_TEST_P(ModelTypeSyncBridgeTest, UpdateLocalTypedUrl) {
  const auto entity1 = this->tester()->CreateAndStoreNativeTestEntity1();

  // Create the same data in sync.
  this->tester()->StartSyncing({entity1.specifics});

  ASSERT_THAT(this->processor()->put_multimap(), testing::IsEmpty());

  // Mutate the local entity.
  const auto& mutated_entity1 = this->tester()->MutateAndStoreTestEntity1();
  ASSERT_THAT(mutated_entity1.specifics,
              testing::Not(EqualsProto(entity1.specifics)));

  EXPECT_THAT(this->processor()->put_multimap(),
              testing::ElementsAre(testing::Pair(
                  entity1.storage_key,
                  TypeParam::HasSpecifics(mutated_entity1.specifics))));
  EXPECT_THAT(
      this->GetAllDataFromBridge(),
      testing::ElementsAre(testing::Pair(
          entity1.storage_key, EqualsProto(mutated_entity1.specifics))));
}

// Create a remote typed URL and visit, then send to sync bridge after sync
// has started. Check that local DB is received the new URL and visit.
TYPED_TEST_P(ModelTypeSyncBridgeTest, AddUrlAndVisits) {
  // TODO(mastiz) / DONOTSUBMIT: Is this generalizable?
  /*
  const auto entity1 = this->tester()->CreateRemoteOnlyTestEntity1();

  StartSyncing(std::vector<TypedUrlSpecifics>());
  ApplyUrlAndVisitsChange(native1, EntityChange::ACTION_ADD);

  ASSERT_EQ(0U, this->processor()->put_multimap().size());
  ASSERT_EQ(1U, this->processor()->update_multimap().size());
  ASSERT_EQ(0U, this->processor()->untrack_set().size());

  // Verify processor receive correct upate storage key.
  const auto& it = this->processor()->update_multimap().begin();
  EXPECT_EQ(it->first, IntToStroageKey(1));
  EXPECT_TRUE(it->second->specifics.has_typed_url());
  EXPECT_EQ(it->second->specifics.typed_url().url(), kURL);
  EXPECT_EQ(it->second->specifics.typed_url().title(), kTitle);

  base::Time visit_time = base::Time::FromInternalValue(3);
  VisitVector all_visits;
  URLID url_id = fake_history_backend_->GetIdByUrl(GURL(kURL));
  ASSERT_NE(0, url_id);
  fake_history_backend_->GetVisitsForURL(url_id, &all_visits);
  EXPECT_EQ(1U, all_visits.size());
  EXPECT_EQ(visit_time, all_visits[0].visit_time);
  EXPECT_TRUE(ui::PageTransitionTypeIncludingQualifiersIs(
      all_visits[0].transition, visits[0].transition));
  URLRow url_row;
  EXPECT_TRUE(fake_history_backend_->GetURL(GURL(kURL), &url_row));
  EXPECT_EQ(kTitle, base::UTF16ToUTF8(url_row.title()));
  */
}

// Update a remote typed URL and create a new visit that is already synced, then
// send the update to sync bridge. Check that local DB is received an
// UPDATE for the existing url and new visit.
TYPED_TEST_P(ModelTypeSyncBridgeTest, UpdateUrlAndVisits) {
  // TODO(mastiz) / DONOTSUBMIT: Is this generalizable?
  /*
  StartSyncing(std::vector<TypedUrlSpecifics>());

  VisitVector visits = ApplyUrlAndVisitsChange(kURL, kTitle, 1, 3, false,
                                               EntityChange::ACTION_ADD);
  base::Time visit_time = base::Time::FromInternalValue(3);
  VisitVector all_visits;
  URLRow url_row;

  URLID url_id = fake_history_backend_->GetIdByUrl(GURL(kURL));
  ASSERT_NE(0, url_id);

  fake_history_backend_->GetVisitsForURL(url_id, &all_visits);

  EXPECT_EQ(1U, all_visits.size());
  EXPECT_EQ(visit_time, all_visits[0].visit_time);
  EXPECT_TRUE(ui::PageTransitionTypeIncludingQualifiersIs(
      all_visits[0].transition, visits[0].transition));
  EXPECT_TRUE(fake_history_backend_->GetURL(GURL(kURL), &url_row));
  EXPECT_EQ(kTitle, base::UTF16ToUTF8(url_row.title()));

  VisitVector new_visits = ApplyUrlAndVisitsChange(kURL, kTitle2, 2, 6, false,
                                                   EntityChange::ACTION_UPDATE);

  base::Time new_visit_time = base::Time::FromInternalValue(6);
  url_id = fake_history_backend_->GetIdByUrl(GURL(kURL));
  ASSERT_NE(0, url_id);
  fake_history_backend_->GetVisitsForURL(url_id, &all_visits);

  EXPECT_EQ(2U, all_visits.size());
  EXPECT_EQ(new_visit_time, all_visits.back().visit_time);
  EXPECT_TRUE(ui::PageTransitionTypeIncludingQualifiersIs(
      all_visits.back().transition, new_visits[0].transition));
  EXPECT_TRUE(fake_history_backend_->GetURL(GURL(kURL), &url_row));
  EXPECT_EQ(kTitle2, base::UTF16ToUTF8(url_row.title()));
  */
}

// Delete a typed urls which already synced. Check that local DB receives the
// DELETE changes.
TYPED_TEST_P(ModelTypeSyncBridgeTest, DeleteUrlAndVisits) {
  const auto entity1 = this->tester()->CreateRemoteOnlyTestEntity1();
  this->tester()->StartSyncing({entity1.specifics});
  ASSERT_THAT(this->processor()->put_multimap(), testing::IsEmpty());

  const std::map<std::string, typename TypeParam::SPECIFICS> initial_data =
      this->GetAllDataFromBridge();
  ASSERT_THAT(initial_data, testing::SizeIs(1));
  const std::string storage_key = initial_data.begin()->first;

  // Remote deletion.
  this->bridge()->ApplySyncChanges(this->bridge()->CreateMetadataChangeList(),
                                   {EntityChange::CreateDelete(storage_key)});

  ASSERT_THAT(this->GetAllDataFromBridge(), testing::IsEmpty());
  // TODO/DONOTSUBMIT: Should we in addition verify the removal using the
  // model's regular API? Using GetAllData() might use a different codepath
  // (e.g. additional filtering?). We might need to add yet another function to
  // the tester class.

  // Check TypedUrlSyncBridge did not receive update since the update is
  // trigered by it.
  EXPECT_THAT(this->processor()->put_multimap(), testing::IsEmpty());
}

REGISTER_TYPED_TEST_CASE_P(ModelTypeSyncBridgeTest,
                           GetAllDataWithEmptyModel,
                           GetAllData,
                           GetData,
                           MergeUrlNoChange,
                           MergeUrlEmptySync,
                           MergeUrlEmptyLocal,
                           SimpleMerge,
                           AddLocalTypedUrl,
                           UpdateLocalTypedUrl,
                           AddUrlAndVisits,
                           UpdateUrlAndVisits,
                           DeleteUrlAndVisits);

}  // namespace syncer

#endif  // COMPONENTS_SYNC_TEST_MODEL_TYPE_SYNC_BRIDGE_TEST_TEMPLATE_H_
