// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/renderer/gin_port.h"

#include "base/bind.h"
#include "base/optional.h"
#include "base/strings/stringprintf.h"
#include "extensions/common/api/messaging/message.h"
#include "extensions/common/api/messaging/port_id.h"
#include "extensions/renderer/bindings/api_binding_test.h"
#include "extensions/renderer/bindings/api_binding_test_util.h"
#include "extensions/renderer/bindings/api_event_handler.h"
#include "gin/data_object_builder.h"
#include "gin/handle.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "third_party/WebKit/public/web/WebScopedUserGesture.h"

namespace extensions {

namespace {

void DoNothingOnEventListenersChanged(const std::string& event_name,
                                      binding::EventListenersChanged change,
                                      const base::DictionaryValue* value,
                                      bool was_manual,
                                      v8::Local<v8::Context> context) {}

// Stub delegate for testing.
class TestPortDelegate : public GinPort::Delegate {
 public:
  TestPortDelegate() {}
  ~TestPortDelegate() override {}

  void PostMessageToPort(const PortId& port_id,
                         std::unique_ptr<Message> message) override {
    last_port_id_ = port_id;
    last_message_ = std::move(message);
  }
  MOCK_METHOD1(ClosePort, void(const PortId&));

  void ResetLastMessage() {
    last_port_id_.reset();
    last_message_.reset();
  }

  const base::Optional<PortId>& last_port_id() const { return last_port_id_; }
  const Message* last_message() const { return last_message_.get(); }

 private:
  base::Optional<PortId> last_port_id_;
  std::unique_ptr<Message> last_message_;

  DISALLOW_COPY_AND_ASSIGN(TestPortDelegate);
};

class GinPortTest : public APIBindingTest {
 public:
  GinPortTest() {}
  ~GinPortTest() override {}

  void SetUp() override {
    APIBindingTest::SetUp();
    event_handler_ = std::make_unique<APIEventHandler>(
        base::Bind(&RunFunctionOnGlobalAndIgnoreResult),
        base::Bind(&RunFunctionOnGlobalAndReturnHandle),
        base::Bind(&DoNothingOnEventListenersChanged), nullptr);
    delegate_ = std::make_unique<TestPortDelegate>();
  }

  void TearDown() override {
    APIBindingTest::TearDown();
    event_handler_.reset();
  }

  void OnWillDisposeContext(v8::Local<v8::Context> context) override {
    event_handler_->InvalidateContext(context);
  }

  APIEventHandler* event_handler() { return event_handler_.get(); }
  TestPortDelegate* delegate() { return delegate_.get(); }

 private:
  std::unique_ptr<APIEventHandler> event_handler_;
  std::unique_ptr<TestPortDelegate> delegate_;

  DISALLOW_COPY_AND_ASSIGN(GinPortTest);
};

}  // namespace

// Tests getting the port's name.
TEST_F(GinPortTest, TestGetName) {
  v8::HandleScope handle_scope(isolate());
  v8::Local<v8::Context> context = MainContext();

  PortId port_id(base::UnguessableToken::Create(), 0, true);
  std::string name = "port name";
  gin::Handle<GinPort> port = gin::CreateHandle(
      isolate(), new GinPort(port_id, name, event_handler(), delegate()));

  v8::Local<v8::Object> port_obj = port.ToV8().As<v8::Object>();

  EXPECT_EQ(base::StringPrintf("\"%s\"", name.c_str()),
            GetStringPropertyFromObject(port_obj, context, "name"));
}

// Tests dispatching a message through the port to JS listeners.
TEST_F(GinPortTest, TestDispatchMessage) {
  v8::HandleScope handle_scope(isolate());
  v8::Local<v8::Context> context = MainContext();

  PortId port_id(base::UnguessableToken::Create(), 0, true);
  std::string name = "port name";
  gin::Handle<GinPort> port = gin::CreateHandle(
      isolate(), new GinPort(port_id, name, event_handler(), delegate()));

  v8::Local<v8::Object> port_obj = port.ToV8().As<v8::Object>();

  const char kTestFunction[] =
      R"((function(port) {
           this.onMessagePortValid = false;
           this.messageValid = false;
           port.onMessage.addListener((message, listenerPort) => {
             this.onMessagePortValid = listenerPort === port;
             let stringifiedMessage = JSON.stringify(message);
             this.messageValid =
                 stringifiedMessage === '{"foo":42}' || stringifiedMessage;
           });
      }))";
  v8::Local<v8::Function> test_function =
      FunctionFromString(context, kTestFunction);
  v8::Local<v8::Value> args[] = {port_obj};
  RunFunctionOnGlobal(test_function, context, arraysize(args), args);

  port->DispatchOnMessage(context, Message(R"({"foo":42})", false));

  EXPECT_EQ("true", GetStringPropertyFromObject(context->Global(), context,
                                                "messageValid"));
  EXPECT_EQ("true", GetStringPropertyFromObject(context->Global(), context,
                                                "onMessagePortValid"));
}

// Tests posting a message from JS.
TEST_F(GinPortTest, TestPostMessage) {
  v8::HandleScope handle_scope(isolate());
  v8::Local<v8::Context> context = MainContext();

  PortId port_id(base::UnguessableToken::Create(), 0, true);
  std::string name = "port name";
  gin::Handle<GinPort> port = gin::CreateHandle(
      isolate(), new GinPort(port_id, name, event_handler(), delegate()));

  v8::Local<v8::Object> port_obj = port.ToV8().As<v8::Object>();

  {
    // Simple message; should succeed.
    const char kFunction[] =
        "(function(port) { port.postMessage({data: [42]}); })";
    v8::Local<v8::Function> function = FunctionFromString(context, kFunction);
    v8::Local<v8::Value> args[] = {port_obj};
    RunFunction(function, context, arraysize(args), args);

    ASSERT_TRUE(delegate()->last_port_id());
    EXPECT_EQ(port_id, delegate()->last_port_id());
    ASSERT_TRUE(delegate()->last_message());
    EXPECT_EQ(R"({"data":[42]})", delegate()->last_message()->data);
    EXPECT_FALSE(delegate()->last_message()->user_gesture);
    delegate()->ResetLastMessage();
  }

  /*{
    // Simple non-object message; should succeed.
    const char kFunction[] =
        "(function(port) { port.postMessage('hello'); })";
    v8::Local<v8::Function> function = FunctionFromString(context, kFunction);
    v8::Local<v8::Value> args[] = {port_obj};
    RunFunction(function, context, arraysize(args), args);

    ASSERT_TRUE(delegate()->last_port_id());
    EXPECT_EQ(port_id, delegate()->last_port_id());
    ASSERT_TRUE(delegate()->last_message());
    EXPECT_EQ(R"("hello")", delegate()->last_message()->data);
    EXPECT_FALSE(delegate()->last_message()->user_gesture);
    delegate()->ResetLastMessage();
  }*/

  {
    // Simple message with user gesture; should succeed.
    const char kFunction[] =
        "(function(port) { port.postMessage({data: [42]}); })";
    v8::Local<v8::Function> function = FunctionFromString(context, kFunction);
    v8::Local<v8::Value> args[] = {port_obj};
    blink::WebScopedUserGesture user_gesture(nullptr);
    RunFunction(function, context, arraysize(args), args);

    ASSERT_TRUE(delegate()->last_port_id());
    EXPECT_EQ(port_id, delegate()->last_port_id());
    ASSERT_TRUE(delegate()->last_message());
    EXPECT_EQ(R"({"data":[42]})", delegate()->last_message()->data);
    EXPECT_TRUE(delegate()->last_message()->user_gesture);
    delegate()->ResetLastMessage();
  }

  {
    // Un-JSON-able object (self-referential). Should fail.
    const char kFunction[] =
        R"((function(port) {
             let message = {foo: 42};
             message.bar = message;
             port.postMessage(message);
           }))";
    v8::Local<v8::Function> function = FunctionFromString(context, kFunction);
    v8::Local<v8::Value> args[] = {port_obj};
    RunFunctionAndExpectError(
        function, context, arraysize(args), args,
        "Uncaught Error: Illegal argument to Port.postMessage");

    EXPECT_FALSE(delegate()->last_port_id());
    EXPECT_FALSE(delegate()->last_message());
    delegate()->ResetLastMessage();
  }

  {
    // Closed port. Should fail.
    EXPECT_CALL(*delegate(), ClosePort(port_id)).Times(1);
    port->Disconnect(context);
    ::testing::Mock::VerifyAndClearExpectations(delegate());
    EXPECT_TRUE(port->is_closed());
    const char kFunction[] =
        "(function(port) { port.postMessage({data: [42]}); })";
    v8::Local<v8::Function> function = FunctionFromString(context, kFunction);
    v8::Local<v8::Value> args[] = {port_obj};
    RunFunctionAndExpectError(
        function, context, arraysize(args), args,
        "Uncaught Error: Attempting to use a disconnected port object");

    EXPECT_FALSE(delegate()->last_port_id());
    EXPECT_FALSE(delegate()->last_message());
    delegate()->ResetLastMessage();
  }
}

TEST_F(GinPortTest, TestNativeDisconnect) {
  v8::HandleScope handle_scope(isolate());
  v8::Local<v8::Context> context = MainContext();

  PortId port_id(base::UnguessableToken::Create(), 0, true);
  std::string name = "port name";
  gin::Handle<GinPort> port = gin::CreateHandle(
      isolate(), new GinPort(port_id, name, event_handler(), delegate()));

  v8::Local<v8::Object> port_obj = port.ToV8().As<v8::Object>();

  const char kTestFunction[] =
      R"((function(port) {
           this.onDisconnectPortValid = false;
           port.onDisconnect.addListener(listenerPort => {
             this.onDisconnectPortValid = listenerPort === port;
           });
      }))";
  v8::Local<v8::Function> test_function =
      FunctionFromString(context, kTestFunction);
  v8::Local<v8::Value> args[] = {port_obj};
  RunFunctionOnGlobal(test_function, context, arraysize(args), args);

  EXPECT_CALL(*delegate(), ClosePort(port_id)).Times(1);
  port->Disconnect(context);
  ::testing::Mock::VerifyAndClearExpectations(delegate());
  EXPECT_EQ("true", GetStringPropertyFromObject(context->Global(), context,
                                                "onDisconnectPortValid"));
  EXPECT_TRUE(port->is_closed());
}

// Tests calling disconnect() from JS.
TEST_F(GinPortTest, TestJSDisconnect) {
  v8::HandleScope handle_scope(isolate());
  v8::Local<v8::Context> context = MainContext();

  PortId port_id(base::UnguessableToken::Create(), 0, true);
  std::string name = "port name";
  gin::Handle<GinPort> port = gin::CreateHandle(
      isolate(), new GinPort(port_id, name, event_handler(), delegate()));

  v8::Local<v8::Object> port_obj = port.ToV8().As<v8::Object>();

  {
    EXPECT_CALL(*delegate(), ClosePort(port_id)).Times(1);
    const char kFunction[] = "(function(port) { port.disconnect(); })";
    v8::Local<v8::Function> function = FunctionFromString(context, kFunction);
    v8::Local<v8::Value> args[] = {port_obj};
    RunFunction(function, context, arraysize(args), args);
    ::testing::Mock::VerifyAndClearExpectations(delegate());
    EXPECT_TRUE(port->is_closed());
  }
}

// Tests setting and getting the 'sender' property.
TEST_F(GinPortTest, TestSenderProperty) {
  v8::HandleScope handle_scope(isolate());
  v8::Local<v8::Context> context = MainContext();

  PortId port_id(base::UnguessableToken::Create(), 0, true);
  std::string name = "port name";
  gin::Handle<GinPort> port = gin::CreateHandle(
      isolate(), new GinPort(port_id, name, event_handler(), delegate()));

  v8::Local<v8::Object> port_obj = port.ToV8().As<v8::Object>();

  EXPECT_EQ("undefined",
            GetStringPropertyFromObject(port_obj, context, "sender"));

  port->SetSender(context,
                  gin::DataObjectBuilder(isolate()).Set("prop", 42).Build());

  EXPECT_EQ(R"({"prop":42})",
            GetStringPropertyFromObject(port_obj, context, "sender"));
}

}  // namespace extensions
