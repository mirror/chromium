// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "modules/credentialmanager/CredentialsContainer.h"

#include <memory>
#include <utility>

#include "bindings/core/v8/V8BindingForTesting.h"
#include "bindings/core/v8/V8GCController.h"
#include "core/dom/Document.h"
#include "core/frame/FrameTestHelpers.h"
#include "core/frame/LocalFrame.h"
#include "modules/credentialmanager/CredentialManagerProxy.h"
#include "modules/credentialmanager/CredentialRequestOptions.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "platform/testing/UnitTestHelpers.h"
#include "platform/weborigin/SecurityOrigin.h"
#include "platform/wtf/Functional.h"
#include "services/service_manager/public/cpp/interface_provider.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/WebKit/public/platform/modules/credentialmanager/credential_manager.mojom-blink.h"

namespace blink {

namespace {

class MockCredentialManager
    : public ::password_manager::mojom::blink::CredentialManager {
  WTF_MAKE_NONCOPYABLE(MockCredentialManager);

 public:
  MockCredentialManager() : binding_(this) {}
  ~MockCredentialManager() {}

  void Bind(
      ::password_manager::mojom::blink::CredentialManagerRequest request) {
    binding_.Bind(std::move(request));
  }

  void WaitForConnectionError() {
    if (!binding_.is_bound())
      return;

    binding_.set_connection_error_handler(WTF::Bind(&testing::ExitRunLoop));
    testing::EnterRunLoop();
  }

  void WaitForCallToGet() {
    if (get_callback_)
      return;

    testing::EnterRunLoop();
  }

  void InvokeGetCallback() {
    EXPECT_TRUE(binding_.is_bound());

    auto info = password_manager::mojom::blink::CredentialInfo::New();
    info->type = password_manager::mojom::blink::CredentialType::EMPTY;
    info->federation = SecurityOrigin::CreateUnique();
    std::move(get_callback_)
        .Run(password_manager::mojom::blink::CredentialManagerError::SUCCESS,
             std::move(info));
  }

 protected:
  virtual void Store(
      password_manager::mojom::blink::CredentialInfoPtr credential,
      StoreCallback callback) {}
  virtual void PreventSilentAccess(PreventSilentAccessCallback callback) {}
  virtual void Get(
      password_manager::mojom::blink::CredentialMediationRequirement mediation,
      bool include_passwords,
      const WTF::Vector<::blink::KURL>& federations,
      GetCallback callback) {
    get_callback_ = std::move(callback);
    testing::ExitRunLoop();
  }

 private:
  mojo::Binding<::password_manager::mojom::blink::CredentialManager> binding_;

  GetCallback get_callback_;
};

class CredentialManagerTestingContext {
  STACK_ALLOCATED();

 public:
  CredentialManagerTestingContext(
      MockCredentialManager* mock_credential_manager) {
    dummy_context_.GetDocument().SetSecurityOrigin(
        SecurityOrigin::CreateFromString("https://example.test"));
    dummy_context_.GetDocument().SetSecureContextStateForTesting(
        SecureContextState::kSecure);
    service_manager::InterfaceProvider::TestApi test_api(
        &dummy_context_.GetFrame().GetInterfaceProvider());
    test_api.SetBinderForName(
        ::password_manager::mojom::blink::CredentialManager::Name_,
        WTF::BindRepeating(
            [](MockCredentialManager* credential_manager,
               mojo::ScopedMessagePipeHandle handle) {
              credential_manager->Bind(
                  ::password_manager::mojom::blink::CredentialManagerRequest(
                      std::move(handle)));
            },
            WTF::Unretained(mock_credential_manager)));
  }

  Document* document() { return &dummy_context_.GetDocument(); }
  LocalFrame* frame() { return &dummy_context_.GetFrame(); }
  ScriptState* script_state() { return dummy_context_.GetScriptState(); }

 private:
  V8TestingScope dummy_context_;
};

}  // namespace

// The completion callbacks for pending mojom::CredentialManager calls each own
// a persistent handle to a ScriptPromiseResolver instance. Ensure that if the
// document is destored while a call is pending, it can still be freed up.
TEST(CredentialsContainerTest, PendingGetRequest_NoGCCycles) {
  MockCredentialManager credential_manager;
  WeakPersistent<Document> weak_document;

  {
    CredentialManagerTestingContext context(&credential_manager);
    weak_document = context.document();
    CredentialsContainer::Create()->get(context.script_state(),
                                        CredentialRequestOptions());
    credential_manager.WaitForCallToGet();
  }

  V8GCController::CollectAllGarbageForTesting(v8::Isolate::GetCurrent());
  ThreadState::Current()->CollectAllGarbage();

  ASSERT_EQ(nullptr, weak_document.Get());

  credential_manager.InvokeGetCallback();
  credential_manager.WaitForConnectionError();
}

TEST(CredentialsContainerTest,
     PendingGetRequest_NoCrashOnResponseAfterDocumentShutdown) {
  MockCredentialManager credential_manager;
  CredentialManagerTestingContext context(&credential_manager);

  auto promise = CredentialsContainer::Create()->get(
      context.script_state(), CredentialRequestOptions());
  credential_manager.WaitForCallToGet();

  context.document()->Shutdown();

  credential_manager.InvokeGetCallback();
  EXPECT_EQ(v8::Promise::kPending,
            promise.V8Value().As<v8::Promise>()->State());
}

}  // namespace blink
