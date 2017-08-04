// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/dom/ClassicPendingScript.h"

#include "core/dom/MockScriptElementBase.h"
#include "core/loader/resource/ScriptResource.h"
#include "platform/exported/WrappedResourceResponse.h"
#include "platform/loader/fetch/ResourceFetcher.h"
#include "platform/loader/fetch/ResourceLoader.h"
#include "platform/loader/testing/MockFetchContext.h"
#include "platform/testing/ScopedMockedURL.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace blink {

namespace {

class MockPendingScriptClient
    : public GarbageCollectedFinalized<MockPendingScriptClient>,
      public PendingScriptClient {
  USING_GARBAGE_COLLECTED_MIXIN(MockPendingScriptClient);

 public:
  static MockPendingScriptClient* Create() {
    return new ::testing::StrictMock<MockPendingScriptClient>();
  }

  virtual ~MockPendingScriptClient() {}
  MOCK_METHOD1(PendingScriptFinished, void(PendingScript*));

  DEFINE_INLINE_VIRTUAL_TRACE() { PendingScriptClient::Trace(visitor); }
};

class ClassicPendingScriptTest : public ::testing::Test {
 public:
  ClassicPendingScriptTest()
      : test_url_(kParsedURLString, "http://example.com/test.js"),
        fetcher_(ResourceFetcher::Create(MockFetchContext::Create(
            MockFetchContext::kShouldLoadNewResource))),
        scoped_mocked_url_load_(test_url_, "") {}

  ScriptResource* Fetch(const ResourceRequest& request) {
    FetchParameters fetch_params(request);
    return ScriptResource::Fetch(fetch_params, fetcher_);
  }

  void ReceiveResponse(Resource* resource, const char* data = "fox") {
    size_t length = strlen(data);
    ResourceResponse response(TestUrl(), "text/javascript", length,
                              g_null_atom);
    resource->Loader()->DidReceiveResponse(WrappedResourceResponse(response));
    resource->Loader()->DidReceiveData(data, length);
    resource->Loader()->DidFinishLoading(0.0, length, length, length);
  }

  const KURL& TestUrl() const { return test_url_; }

 private:
  const KURL test_url_;
  Persistent<ResourceFetcher> fetcher_;
  testing::ScopedMockedURLLoad scoped_mocked_url_load_;
};

TEST_F(ClassicPendingScriptTest, GetSource) {
  ScriptResource* resource = Fetch(ResourceRequest(TestUrl()));

  MockScriptElementBase* element = MockScriptElementBase::Create();
  ClassicPendingScript* pending_script =
      ClassicPendingScript::Create(element, resource);
  Persistent<MockPendingScriptClient> mock_client =
      MockPendingScriptClient::Create();
  pending_script->WatchForLoad(mock_client);

  EXPECT_FALSE(pending_script->IsReady());
  EXPECT_CALL(*mock_client, PendingScriptFinished(pending_script));
  EXPECT_CALL(*element, IntegrityAttributeValue())
      .WillOnce(::testing::Return(String()));

  ReceiveResponse(resource);

  EXPECT_TRUE(pending_script->IsReady());

  bool error_occurred = false;
  Script* script = pending_script->GetSource(KURL(), error_occurred);

  EXPECT_FALSE(error_occurred);
  ASSERT_TRUE(script);
  EXPECT_EQ(ScriptType::kClassic, script->GetScriptType());
  EXPECT_EQ("fox", script->InlineSourceTextForCSP());
}

// Following tests related to revalidation are for crbug.com/692856.
//
// GetSource() should return the original source even after revalidation starts.
TEST_F(ClassicPendingScriptTest, GetSourceAfterRevalidationStarts) {
  ScriptResource* resource = Fetch(ResourceRequest(TestUrl()));

  MockScriptElementBase* element = MockScriptElementBase::Create();
  ClassicPendingScript* pending_script =
      ClassicPendingScript::Create(element, resource);
  Persistent<MockPendingScriptClient> mock_client =
      MockPendingScriptClient::Create();
  pending_script->WatchForLoad(mock_client);

  EXPECT_FALSE(pending_script->IsReady());
  EXPECT_CALL(*mock_client, PendingScriptFinished(pending_script));
  EXPECT_CALL(*element, IntegrityAttributeValue())
      .WillOnce(::testing::Return(String()));

  ReceiveResponse(resource);

  EXPECT_TRUE(resource->IsLoaded());
  EXPECT_TRUE(pending_script->IsReady());

  // Simulates revalidation start.
  resource->SetRevalidatingRequest(ResourceRequest(TestUrl()));

  EXPECT_FALSE(resource->IsLoaded());
  EXPECT_TRUE(pending_script->IsReady());

  bool error_occurred = false;
  Script* script = pending_script->GetSource(KURL(), error_occurred);

  EXPECT_FALSE(error_occurred);
  ASSERT_TRUE(script);
  EXPECT_EQ(ScriptType::kClassic, script->GetScriptType());
  EXPECT_EQ("fox", script->InlineSourceTextForCSP());
}

// GetSource() should return the original source even after revalidation starts
// and revalidation response is received.
TEST_F(ClassicPendingScriptTest, GetSourceAfterRevalidationFailed) {
  ScriptResource* resource = Fetch(ResourceRequest(TestUrl()));

  MockScriptElementBase* element = MockScriptElementBase::Create();
  ClassicPendingScript* pending_script =
      ClassicPendingScript::Create(element, resource);
  Persistent<MockPendingScriptClient> mock_client =
      MockPendingScriptClient::Create();
  pending_script->WatchForLoad(mock_client);

  EXPECT_FALSE(pending_script->IsReady());
  EXPECT_CALL(*mock_client, PendingScriptFinished(pending_script));
  EXPECT_CALL(*element, IntegrityAttributeValue())
      .WillOnce(::testing::Return(String()));

  ReceiveResponse(resource);

  EXPECT_TRUE(resource->IsLoaded());
  EXPECT_TRUE(pending_script->IsReady());

  // Simulates revalidation start.
  resource->SetRevalidatingRequest(ResourceRequest(TestUrl()));

  EXPECT_FALSE(resource->IsLoaded());
  EXPECT_TRUE(pending_script->IsReady());

  // Simulates failed revalidation response received.
  ResourceResponse response(TestUrl(), "text/javascript", 7, g_null_atom);
  response.SetHTTPStatusCode(200);
  resource->ResponseReceived(response, nullptr);

  EXPECT_FALSE(resource->IsLoaded());
  EXPECT_TRUE(pending_script->IsReady());

  bool error_occurred = false;
  Script* script = pending_script->GetSource(KURL(), error_occurred);

  EXPECT_FALSE(error_occurred);
  ASSERT_TRUE(script);
  EXPECT_EQ(ScriptType::kClassic, script->GetScriptType());
  EXPECT_EQ("fox", script->InlineSourceTextForCSP());
}

// GetSource() should return the original source even after revalidation is
// finished.
TEST_F(ClassicPendingScriptTest, GetSourceAfterFailedRevalidationFinished) {
  ScriptResource* resource = Fetch(ResourceRequest(TestUrl()));

  MockScriptElementBase* element = MockScriptElementBase::Create();
  ClassicPendingScript* pending_script =
      ClassicPendingScript::Create(element, resource);
  Persistent<MockPendingScriptClient> mock_client =
      MockPendingScriptClient::Create();
  pending_script->WatchForLoad(mock_client);

  EXPECT_FALSE(pending_script->IsReady());
  EXPECT_CALL(*mock_client, PendingScriptFinished(pending_script));
  EXPECT_CALL(*element, IntegrityAttributeValue())
      .WillOnce(::testing::Return(String()));

  ReceiveResponse(resource);

  EXPECT_TRUE(resource->IsLoaded());
  EXPECT_TRUE(pending_script->IsReady());

  // Simulates revalidation start.
  resource->SetRevalidatingRequest(ResourceRequest(TestUrl()));

  EXPECT_FALSE(resource->IsLoaded());
  EXPECT_TRUE(pending_script->IsReady());

  // Simulates failed revalidation to finish.
  ResourceResponse response(TestUrl(), "text/javascript", 7, g_null_atom);
  response.SetHTTPStatusCode(200);
  resource->ResponseReceived(response, nullptr);
  // This new data shouldn't be returned from pending_script->GetSource().
  resource->AppendData("kitsune", 7);
  resource->Finish();

  EXPECT_TRUE(resource->IsLoaded());
  EXPECT_TRUE(pending_script->IsReady());

  bool error_occurred = false;
  Script* script = pending_script->GetSource(KURL(), error_occurred);

  EXPECT_FALSE(error_occurred);
  ASSERT_TRUE(script);
  EXPECT_EQ(ScriptType::kClassic, script->GetScriptType());
  EXPECT_EQ("fox", script->InlineSourceTextForCSP());
}

}  // namespace

}  // namespace blink
