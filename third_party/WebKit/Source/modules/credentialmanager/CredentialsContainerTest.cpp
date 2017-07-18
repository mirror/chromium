// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "modules/credentialmanager/CredentialsContainer.h"

#include <memory>

#include "bindings/core/v8/V8BindingForTesting.h"
#include "bindings/core/v8/V8GCController.h"
#include "core/dom/Document.h"
#include "modules/credentialmanager/CredentialManagerClient.h"
#include "modules/credentialmanager/CredentialRequestOptions.h"
#include "public/platform/WebCredentialMediationRequirement.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace blink {

using ::testing::_;
using ::testing::SaveArg;

namespace {

class MockCredentialManagerClient : public CredentialManagerClient {
 public:
  MockCredentialManagerClient() : CredentialManagerClient(nullptr) {}
  ~MockCredentialManagerClient() {}

  MOCK_METHOD4(DispatchGet,
               void(WebCredentialMediationRequirement,
                    bool,
                    const ::WTF::Vector<KURL>& federations,
                    RequestCallbacks*));
};

}  // namespace

// Make a call to CredentialsContainer::get(). Destroy v8::Context.
// Make sure that the renderer doesn't crash if got a credential.
TEST(CredentialsContainerTest, TestGetWithDocumentDestroyed) {
  CredentialsContainer* credential_container = CredentialsContainer::Create();
  std::unique_ptr<CredentialManagerClient::RequestCallbacks> get_callback;

  V8TestingScope scope;
  {
    // Set up.
    scope.GetDocument().SetSecurityOrigin(
        SecurityOrigin::CreateFromString("https://example.test"));
    ::testing::StrictMock<MockCredentialManagerClient> mock_client;
    CredentialManagerClient::ProvideToExecutionContext(&scope.GetDocument(),
                                                       &mock_client);

    // Request a credential.
    CredentialManagerClient::RequestCallbacks* callback = nullptr;
    EXPECT_CALL(mock_client, DispatchGet(_, _, _, _))
        .WillOnce(SaveArg<3>(&callback));
    credential_container->get(scope.GetScriptState(),
                              CredentialRequestOptions());

    ASSERT_TRUE(callback);
    get_callback.reset(callback);
  }

  // Garbage collect v8::Context.
  V8GCController::CollectAllGarbageForTesting(v8::Isolate::GetCurrent());

  // Invoking the callback shouldn't crash.
  get_callback->OnSuccess(nullptr);
}

}  // namespace blink
