// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "modules/presentation/PresentationConnection.h"

#include <memory>

#include "bindings/core/v8/V8BindingForTesting.h"
#include "core/frame/LocalDOMWindow.h"
#include "core/frame/LocalFrame.h"
#include "modules/presentation/PresentationConnectionList.h"
#include "modules/presentation/PresentationController.h"
#include "modules/presentation/PresentationReceiver.h"
#include "modules/presentation/PresentationRequest.h"
#include "mojo/public/cpp/bindings/binding_set.h"
#include "platform/testing/URLTestHelpers.h"
#include "platform/testing/UnitTestHelpers.h"
#include "public/platform/modules/presentation/WebPresentationConnectionCallbacks.h"
#include "public/platform/modules/presentation/WebPresentationConnectionProxy.h"
#include "public/platform/modules/presentation/presentation.mojom-blink.h"
#include "services/service_manager/public/cpp/interface_provider.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "v8/include/v8.h"

namespace blink {

class MockPresentationConnectionProxy : public WebPresentationConnectionProxy {
 public:
  MOCK_CONST_METHOD0(Close, void());
  MOCK_METHOD1(NotifyTargetConnection, void(WebPresentationConnectionState));
  MOCK_METHOD1(SendTextMessage, void(const WebString& message));
  MOCK_METHOD2(SendBinaryMessage, void(const uint8_t* data, size_t length));
};

class MockPresentationService : public mojom::blink::PresentationService {
 public:
  explicit MockPresentationService(LocalFrame& frame) {
    service_manager::InterfaceProvider::TestApi test_api(
        &frame.GetInterfaceProvider());
    test_api.SetBinderForName(
        mojom::blink::PresentationService::Name_,
        ConvertToBaseCallback(WTF::Bind(&MockPresentationService::BindRequest,
                                        WTF::Unretained(this))));
  }

  void BindRequest(mojo::ScopedMessagePipeHandle handle) {
    binding_set_.AddBinding(
        this, mojom::blink::PresentationServiceRequest(std::move(handle)));
  }

  size_t NumBindings() const { return binding_set_.size(); }

  void SetClient(mojom::blink::PresentationServiceClientPtr client) override {}
  MOCK_METHOD1(SetDefaultPresentationUrls,
               void(const WTF::Vector<KURL>& presentation_urls));
  MOCK_METHOD1(ListenForScreenAvailability, void(const KURL& availability_url));
  MOCK_METHOD1(StopListeningForScreenAvailability,
               void(const KURL& availability_url));

  // TODO(crbug.com/729950): Use MOCK_METHOD directly once GMock gets the
  // move-only type support.
  void StartPresentation(const WTF::Vector<KURL>& presentation_urls,
                         StartPresentationCallback callback) {
    StartPresentationInternal(presentation_urls, callback);
  }
  MOCK_METHOD2(StartPresentationInternal,
               void(const WTF::Vector<KURL>& presentation_urls,
                    const StartPresentationCallback& callback));

  void ReconnectPresentation(const WTF::Vector<KURL>& presentation_urls,
                             const WTF::String& presentation_id,
                             ReconnectPresentationCallback callback) {
    ReconnectPresentationInternal(presentation_urls, presentation_id, callback);
  }
  MOCK_METHOD3(ReconnectPresentationInternal,
               void(const WTF::Vector<KURL>& presentation_urls,
                    const WTF::String& presentation_id,
                    const ReconnectPresentationCallback& callback));

  void SetPresentationConnection(
      mojom::blink::PresentationInfoPtr presentation_info,
      mojom::blink::PresentationConnectionPtr controller_conn_ptr,
      mojom::blink::PresentationConnectionRequest receiver_conn_request)
      override {
    SetPresentationConnection(presentation_info.get(),
                              controller_conn_ptr.get());
  }
  MOCK_METHOD2(SetPresentationConnection,
               void(mojom::blink::PresentationInfo* presentation_info,
                    mojom::blink::PresentationConnection* connection));

  MOCK_METHOD2(CloseConnection,
               void(const KURL& presentation_url,
                    const WTF::String& presentation_id));
  MOCK_METHOD2(Terminate,
               void(const KURL& presentation_url,
                    const WTF::String& presentation_id));

 private:
  mojo::BindingSet<PresentationService> binding_set_;
};

// TODO(crbug.com/xxxxx): Add more unit tests for PresentationConnection.

TEST(PresentationConnectionTest, CloseReceiverConnection) {
  V8TestingScope scope;
  WebPresentationInfo info(
      WebURL(blink::URLTestHelpers::ToKURL("https://google.com")), "presId");
  auto* receiver = new PresentationReceiver(&scope.GetFrame(), nullptr);
  auto* connection = PresentationConnection::Take(receiver, info);
  auto* proxy_ptr = new MockPresentationConnectionProxy();
  std::unique_ptr<WebPresentationConnectionProxy> proxy(proxy_ptr);
  connection->BindProxy(std::move(proxy));

  EXPECT_CALL(*proxy_ptr, Close());
  connection->close();
}

TEST(PresentationConnectionTest, TerminateReceiverConnection) {
  V8TestingScope scope;
  WebPresentationInfo info(
      WebURL(blink::URLTestHelpers::ToKURL("https://google.com")), "presId");
  auto* receiver = new PresentationReceiver(&scope.GetFrame(), nullptr);
  auto* connection = PresentationConnection::Take(receiver, info);
  auto* proxy_ptr = new MockPresentationConnectionProxy();
  std::unique_ptr<WebPresentationConnectionProxy> proxy(proxy_ptr);
  connection->BindProxy(std::move(proxy));

  connection->DidChangeState(WebPresentationConnectionState::kConnected, false);

  connection->terminate();
  EXPECT_TRUE(scope.GetFrame().DomWindow()->closed());
}

TEST(PresentationConnectionTest, CloseControllerConnection) {
  V8TestingScope scope;
  KURL url(blink::URLTestHelpers::ToKURL("https://google.com"));
  String id("presId");
  WebPresentationInfo info(WebURL(url), id);
  MockPresentationService presentation_service(scope.GetFrame());
  auto* controller = PresentationController::Create(scope.GetFrame(), nullptr);
  auto* request = PresentationRequest::Create(
      scope.GetExecutionContext(), Vector<String>({"https://google.com"}),
      scope.GetExceptionState());
  auto* connection = PresentationConnection::Take(controller, info, request);
  auto* proxy_ptr = new MockPresentationConnectionProxy();
  std::unique_ptr<WebPresentationConnectionProxy> proxy(proxy_ptr);
  connection->BindProxy(std::move(proxy));

  testing::RunPendingTasks();
  EXPECT_EQ(1u, presentation_service.NumBindings());

  EXPECT_CALL(*proxy_ptr, Close());
  EXPECT_CALL(presentation_service, CloseConnection(url, id));
  connection->close();
  testing::RunPendingTasks();
}

TEST(PresentationConnectionTest, TerminateControllerConnection) {
  V8TestingScope scope;
  KURL url(blink::URLTestHelpers::ToKURL("https://google.com"));
  String id("presId");
  WebPresentationInfo info(WebURL(url), id);
  MockPresentationService presentation_service(scope.GetFrame());
  auto* controller = PresentationController::Create(scope.GetFrame(), nullptr);
  auto* request = PresentationRequest::Create(
      scope.GetExecutionContext(), Vector<String>({"https://google.com"}),
      scope.GetExceptionState());
  auto* connection = PresentationConnection::Take(controller, info, request);
  auto* proxy_ptr = new MockPresentationConnectionProxy();
  std::unique_ptr<WebPresentationConnectionProxy> proxy(proxy_ptr);
  connection->BindProxy(std::move(proxy));

  testing::RunPendingTasks();
  EXPECT_EQ(1u, presentation_service.NumBindings());

  connection->DidChangeState(WebPresentationConnectionState::kConnected, false);

  EXPECT_CALL(presentation_service, Terminate(url, id));
  connection->terminate();

  testing::RunPendingTasks();
}

}  // namespace blink
