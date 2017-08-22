// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/loader/modulescript/ModuleScriptCreationParams.h"
#include "core/testing/DummyPageHolder.h"
#include "core/workers/WorkletModuleResponsesMap.h"
#include "platform/loader/testing/FetchTestingPlatformSupport.h"
#include "platform/loader/testing/MockFetchContext.h"
#include "platform/testing/TestingPlatformSupport.h"
#include "platform/testing/URLTestHelpers.h"
#include "platform/testing/UnitTestHelpers.h"
#include "platform/weborigin/KURL.h"
#include "platform/wtf/Functional.h"
#include "platform/wtf/Optional.h"
#include "public/platform/WebURLLoaderMockFactory.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace blink {

namespace {

class ClientImpl final : public GarbageCollectedFinalized<ClientImpl>,
                         public WorkletModuleResponsesMap::Client {
  USING_GARBAGE_COLLECTED_MIXIN(ClientImpl);

 public:
  enum class Result { kInitial, kOK, kFailed };

  void OnRead(const ModuleScriptCreationParams& params) override {
    ASSERT_EQ(Result::kInitial, result_);
    result_ = Result::kOK;
    params_.emplace(params);
  }

  void OnFailed() override {
    ASSERT_EQ(Result::kInitial, result_);
    result_ = Result::kFailed;
  }

  Result GetResult() const { return result_; }
  WTF::Optional<ModuleScriptCreationParams> GetParams() const {
    return params_;
  }

 private:
  Result result_ = Result::kInitial;
  WTF::Optional<ModuleScriptCreationParams> params_;
};

}  // namespace

class WorkletModuleResponsesMapTest : public ::testing::Test {
 public:
  WorkletModuleResponsesMapTest() = default;

  void SetUp() override {
    platform_->AdvanceClockSeconds(1.);  // For non-zero DocumentParserTimings
    dummy_page_holder_ = DummyPageHolder::Create();
    auto context =
        MockFetchContext::Create(MockFetchContext::kShouldLoadNewResource);
    fetcher_ = ResourceFetcher::Create(context);
    map_ = new WorkletModuleResponsesMap(fetcher_);
  }

 protected:
  ScopedTestingPlatformSupport<FetchTestingPlatformSupport> platform_;
  std::unique_ptr<DummyPageHolder> dummy_page_holder_;
  Persistent<ResourceFetcher> fetcher_;
  Persistent<WorkletModuleResponsesMap> map_;
};

TEST_F(WorkletModuleResponsesMapTest, Basic) {
  const KURL kUrl(kParsedURLString, "https://example.com/module.js");
  URLTestHelpers::RegisterMockedURLLoad(
      kUrl, testing::CoreTestDataPath("module.js"), "text/javascript");

  HeapVector<Member<ClientImpl>> clients;

  // An initial read call initiates a fetch request.
  clients.push_back(new ClientImpl);
  map_->ReadOrCreateEntry(FetchParameters(ResourceRequest(kUrl)), clients[0]);
  EXPECT_EQ(ClientImpl::Result::kInitial, clients[0]->GetResult());
  EXPECT_FALSE(clients[0]->GetParams().has_value());

  // The entry is now being fetched. Following read calls should wait for the
  // completion.
  clients.push_back(new ClientImpl);
  map_->ReadOrCreateEntry(FetchParameters(ResourceRequest(kUrl)), clients[1]);
  EXPECT_EQ(ClientImpl::Result::kInitial, clients[1]->GetResult());

  clients.push_back(new ClientImpl);
  map_->ReadOrCreateEntry(FetchParameters(ResourceRequest(kUrl)), clients[2]);
  EXPECT_EQ(ClientImpl::Result::kInitial, clients[2]->GetResult());

  // Serve the fetch request. This should notify the waiting clients.
  platform_->GetURLLoaderMockFactory()->ServeAsynchronousRequests();

  for (auto client : clients) {
    EXPECT_TRUE(client->GetParams().has_value());
    EXPECT_EQ(ClientImpl::Result::kOK, client->GetResult());
  }
}

TEST_F(WorkletModuleResponsesMapTest, Failure) {
  const KURL kUrl(kParsedURLString, "https://example.com/foo.js");

  // An initial read call creates a placeholder entry and asks the client to
  // fetch a module script.
  ClientImpl* client1 = new ClientImpl;
  map_->ReadOrCreateEntry(FetchParameters(ResourceRequest(kUrl)), client1);
  EXPECT_EQ(ClientImpl::Result::kInitial, client1->GetResult());
  EXPECT_FALSE(client1->GetParams().has_value());

  // The entry is now being fetched. Following read calls should wait for the
  // completion.
  ClientImpl* client2 = new ClientImpl;
  map_->ReadOrCreateEntry(FetchParameters(ResourceRequest(kUrl)), client2);
  EXPECT_EQ(ClientImpl::Result::kInitial, client2->GetResult());

  ClientImpl* client3 = new ClientImpl;
  map_->ReadOrCreateEntry(FetchParameters(ResourceRequest(kUrl)), client3);
  EXPECT_EQ(ClientImpl::Result::kInitial, client3->GetResult());

  // An invalidation call should notify the waiting clients.
  map_->InvalidateEntry(kUrl);
  EXPECT_EQ(ClientImpl::Result::kFailed, client2->GetResult());
  EXPECT_FALSE(client2->GetParams().has_value());
  EXPECT_EQ(ClientImpl::Result::kFailed, client3->GetResult());
  EXPECT_FALSE(client3->GetParams().has_value());
}

TEST_F(WorkletModuleResponsesMapTest, Isolation) {
  const KURL kUrl1(kParsedURLString, "https://example.com/foo.js");
  const KURL kUrl2(kParsedURLString, "https://example.com/bar.js");

  // An initial read call for |kUrl1| creates a placeholder entry and asks the
  // client to fetch a module script.
  ClientImpl* client1 = new ClientImpl;
  map_->ReadOrCreateEntry(FetchParameters(ResourceRequest(kUrl1)), client1);
  EXPECT_EQ(ClientImpl::Result::kInitial, client1->GetResult());
  EXPECT_FALSE(client1->GetParams().has_value());

  // The entry is now being fetched. Following read calls for |kUrl1| should
  // wait for the completion.
  ClientImpl* client2 = new ClientImpl;
  map_->ReadOrCreateEntry(FetchParameters(ResourceRequest(kUrl1)), client2);
  EXPECT_EQ(ClientImpl::Result::kInitial, client2->GetResult());

  // An initial read call for |kUrl2| also creates a placeholder entry and asks
  // the client to fetch a module script.
  ClientImpl* client3 = new ClientImpl;
  map_->ReadOrCreateEntry(FetchParameters(ResourceRequest(kUrl2)), client3);
  EXPECT_EQ(ClientImpl::Result::kInitial, client3->GetResult());
  EXPECT_FALSE(client3->GetParams().has_value());

  // The entry is now being fetched. Following read calls for |kUrl2| should
  // wait for the completion.
  ClientImpl* client4 = new ClientImpl;
  map_->ReadOrCreateEntry(FetchParameters(ResourceRequest(kUrl2)), client4);
  EXPECT_EQ(ClientImpl::Result::kInitial, client4->GetResult());

  // The read call for |kUrl2| should not affect the other entry for |kUrl1|.
  EXPECT_EQ(ClientImpl::Result::kInitial, client2->GetResult());

  // An update call for |kUrl2| should notify the waiting clients for |kUrl2|.
  ModuleScriptCreationParams params(kUrl2, "// dummy script",
                                    WebURLRequest::kFetchCredentialsModeOmit,
                                    kNotSharableCrossOrigin);
  map_->UpdateEntry(kUrl2, params);
  EXPECT_EQ(ClientImpl::Result::kOK, client4->GetResult());
  EXPECT_TRUE(client4->GetParams().has_value());

  // ... but should not notify the waiting clients for |kUrl1|.
  EXPECT_EQ(ClientImpl::Result::kInitial, client2->GetResult());
}

TEST_F(WorkletModuleResponsesMapTest, InvalidURL) {
  const KURL kEmptyURL;
  ASSERT_TRUE(kEmptyURL.IsEmpty());
  ClientImpl* client1 = new ClientImpl;
  map_->ReadOrCreateEntry(FetchParameters(ResourceRequest(kEmptyURL)), client1);
  EXPECT_EQ(ClientImpl::Result::kFailed, client1->GetResult());
  EXPECT_FALSE(client1->GetParams().has_value());

  const KURL kNullURL = NullURL();
  ASSERT_TRUE(kNullURL.IsNull());
  ClientImpl* client2 = new ClientImpl;
  map_->ReadOrCreateEntry(FetchParameters(ResourceRequest(kNullURL)), client2);
  EXPECT_EQ(ClientImpl::Result::kFailed, client2->GetResult());
  EXPECT_FALSE(client2->GetParams().has_value());

  const KURL kInvalidURL(kParsedURLString, String());
  ASSERT_FALSE(kInvalidURL.IsValid());
  ClientImpl* client3 = new ClientImpl;
  map_->ReadOrCreateEntry(FetchParameters(ResourceRequest(kInvalidURL)), client3);
  EXPECT_EQ(ClientImpl::Result::kFailed, client3->GetResult());
  EXPECT_FALSE(client3->GetParams().has_value());
}

TEST_F(WorkletModuleResponsesMapTest, Dispose) {
  const KURL kUrl1(kParsedURLString, "https://example.com/foo.js");
  const KURL kUrl2(kParsedURLString, "https://example.com/bar.js");

  // An initial read call for |kUrl1| creates a placeholder entry and asks the
  // client to fetch a module script.
  ClientImpl* client1 = new ClientImpl;
  map_->ReadOrCreateEntry(FetchParameters(ResourceRequest(kUrl1)), client1);
  EXPECT_EQ(ClientImpl::Result::kInitial, client1->GetResult());
  EXPECT_FALSE(client1->GetParams().has_value());

  // The entry is now being fetched. Following read calls for |kUrl1| should
  // wait for the completion.
  ClientImpl* client2 = new ClientImpl;
  map_->ReadOrCreateEntry(FetchParameters(ResourceRequest(kUrl1)), client2);
  EXPECT_EQ(ClientImpl::Result::kInitial, client2->GetResult());

  // An initial read call for |kUrl2| also creates a placeholder entry and asks
  // the client to fetch a module script.
  ClientImpl* client3 = new ClientImpl;
  map_->ReadOrCreateEntry(FetchParameters(ResourceRequest(kUrl2)), client3);
  EXPECT_EQ(ClientImpl::Result::kInitial, client3->GetResult());
  EXPECT_FALSE(client3->GetParams().has_value());

  // The entry is now being fetched. Following read calls for |kUrl2| should
  // wait for the completion.
  ClientImpl* client4 = new ClientImpl;
  map_->ReadOrCreateEntry(FetchParameters(ResourceRequest(kUrl2)), client4);
  EXPECT_EQ(ClientImpl::Result::kInitial, client4->GetResult());

  // Dispose() should notify to all waiting clients.
  map_->Dispose();
  EXPECT_EQ(ClientImpl::Result::kFailed, client2->GetResult());
  EXPECT_FALSE(client2->GetParams().has_value());
  EXPECT_EQ(ClientImpl::Result::kFailed, client4->GetResult());
  EXPECT_FALSE(client4->GetParams().has_value());
}

}  // namespace blink
