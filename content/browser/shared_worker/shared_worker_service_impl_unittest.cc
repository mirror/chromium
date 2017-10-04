// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/shared_worker/shared_worker_service_impl.h"

#include <stddef.h>

#include <map>
#include <memory>
#include <set>
#include <string>
#include <tuple>

#include "base/atomic_sequence_num.h"
#include "base/macros.h"
#include "base/memory/ptr_util.h"
#include "base/run_loop.h"
#include "base/strings/string16.h"
#include "base/strings/utf_string_conversions.h"
#include "base/synchronization/lock.h"
#include "content/browser/shared_worker/shared_worker_connector_impl.h"
#include "content/browser/shared_worker/worker_storage_partition.h"
#include "content/public/browser/storage_partition.h"
#include "content/public/test/mock_render_process_host.h"
#include "content/public/test/test_browser_context.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "content/public/test/test_utils.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/WebKit/common/message_port/message_port_channel.h"

using blink::MessagePortChannel;

namespace content {

class SharedWorkerServiceImplTest : public testing::Test {
 public:
  static void CreateSharedWorkerConnector(
      int process_id,
      int frame_id,
      ResourceContext* resource_context,
      WorkerStoragePartition partition,
      mojom::SharedWorkerConnectorRequest request) {
    SharedWorkerConnectorImpl::CreateInternal(
        process_id, frame_id, resource_context, partition, std::move(request));
  }

  static bool CheckReceivedFactoryRequest(
      mojom::SharedWorkerFactoryRequest* request) {
    if (s_factory_request_received_.empty())
      return false;
    *request = std::move(s_factory_request_received_.front());
    s_factory_request_received_.pop();
    return true;
  }

  static bool CheckNotReceivedFactoryRequest() {
    return s_factory_request_received_.empty();
  }

  static void BindSharedWorkerFactory(mojo::ScopedMessagePipeHandle handle) {
    s_factory_request_received_.push(
        mojom::SharedWorkerFactoryRequest(std::move(handle)));
  }

 protected:
  SharedWorkerServiceImplTest()
      : browser_thread_bundle_(TestBrowserThreadBundle::IO_MAINLOOP),
        browser_context_(new TestBrowserContext()),
        partition_(new WorkerStoragePartition(
            BrowserContext::GetDefaultStoragePartition(browser_context_.get())
                ->GetURLRequestContext(),
            nullptr /* media_url_request_context */,
            nullptr /* appcache_service */,
            nullptr /* quota_manager */,
            nullptr /* filesystem_context */,
            nullptr /* database_tracker */,
            nullptr /* indexed_db_context */,
            nullptr /* service_worker_context */)) {
  }

  void SetUp() override {}

  void TearDown() override {
    SharedWorkerServiceImpl::GetInstance()->ResetForTesting();
  }

  TestBrowserThreadBundle browser_thread_bundle_;
  std::unique_ptr<TestBrowserContext> browser_context_;
  std::unique_ptr<WorkerStoragePartition> partition_;
  static std::queue<mojom::SharedWorkerFactoryRequest>
      s_factory_request_received_;

 private:
  DISALLOW_COPY_AND_ASSIGN(SharedWorkerServiceImplTest);
};

// static
std::queue<mojom::SharedWorkerFactoryRequest>
    SharedWorkerServiceImplTest::s_factory_request_received_;

namespace {

static const int kRenderFrameRouteIDs[] = {300, 301, 302};

template <typename T>
static bool CheckEquality(const T& expected, const T& actual) {
  EXPECT_EQ(expected, actual);
  return expected == actual;
}

std::vector<uint8_t> StringPieceToVector(base::StringPiece s) {
  return std::vector<uint8_t>(s.begin(), s.end());
}

void BlockingReadFromMessagePort(MessagePortChannel port,
                                 std::vector<uint8_t>* message) {
  base::RunLoop run_loop;
  port.SetCallback(run_loop.QuitClosure());
  run_loop.Run();

  std::vector<MessagePortChannel> should_be_empty;
  EXPECT_TRUE(port.GetMessage(message, &should_be_empty));
  EXPECT_TRUE(should_be_empty.empty());
}

class MockSharedWorker : public mojom::SharedWorker {
 public:
  explicit MockSharedWorker(mojom::SharedWorkerRequest request)
      : binding_(this, std::move(request)) {}

  bool CheckReceivedConnect(int* connection_request_id,
                            MessagePortChannel* port) {
    if (connect_received_.empty())
      return false;
    if (connection_request_id)
      *connection_request_id = connect_received_.front().first;
    if (port)
      *port = connect_received_.front().second;
    connect_received_.pop();
    return true;
  }

  bool CheckNotReceivedConnect() { return connect_received_.empty(); }

  bool CheckReceivedTerminate() {
    if (!terminate_received_)
      return false;
    terminate_received_ = false;
    return true;
  }

 private:
  // mojom::SharedWorker methods:
  void Connect(int connection_request_id,
               mojo::ScopedMessagePipeHandle port) override {
    connect_received_.emplace(connection_request_id,
                              MessagePortChannel(std::move(port)));
  }
  void Terminate() override {
    // Allow duplicate events.
    terminate_received_ = true;
  }

  mojo::Binding<mojom::SharedWorker> binding_;
  std::queue<std::pair<int, MessagePortChannel>> connect_received_;
  bool terminate_received_ = false;
};

class MockSharedWorkerFactory : public mojom::SharedWorkerFactory {
 public:
  explicit MockSharedWorkerFactory(mojom::SharedWorkerFactoryRequest request)
      : binding_(this, std::move(request)) {}

  bool CheckReceivedCreateSharedWorker(
      const std::string& expected_url,
      const std::string& expected_name,
      blink::WebContentSecurityPolicyType expected_content_security_policy_type,
      mojom::SharedWorkerHostPtr* host,
      mojom::SharedWorkerRequest* request) {
    std::unique_ptr<CreateParams> create_params = std::move(create_params_);
    if (!create_params)
      return false;
    if (!CheckEquality(GURL(expected_url), create_params->info->url))
      return false;
    if (!CheckEquality(expected_name, create_params->info->name))
      return false;
    if (!CheckEquality(expected_content_security_policy_type,
                       create_params->info->content_security_policy_type))
      return false;
    *host = std::move(create_params->host);
    *request = std::move(create_params->request);
    return true;
  }

 private:
  // mojom::SharedWorkerFactory methods:
  void CreateSharedWorker(
      mojom::SharedWorkerInfoPtr info,
      bool pause_on_start,
      int32_t route_id,
      blink::mojom::WorkerContentSettingsProxyPtr content_settings,
      mojom::SharedWorkerHostPtr host,
      mojom::SharedWorkerRequest request) override {
    CHECK(!create_params_);
    create_params_ = std::make_unique<CreateParams>();
    create_params_->info = std::move(info);
    create_params_->pause_on_start = pause_on_start;
    create_params_->route_id = route_id;
    create_params_->content_settings = std::move(content_settings);
    create_params_->host = std::move(host);
    create_params_->request = std::move(request);
  }

  struct CreateParams {
    mojom::SharedWorkerInfoPtr info;
    bool pause_on_start;
    int32_t route_id;
    blink::mojom::WorkerContentSettingsProxyPtr content_settings;
    mojom::SharedWorkerHostPtr host;
    mojom::SharedWorkerRequest request;
  };

  mojo::Binding<mojom::SharedWorkerFactory> binding_;
  std::unique_ptr<CreateParams> create_params_;
};

class MockSharedWorkerClient : public mojom::SharedWorkerClient {
 public:
  MockSharedWorkerClient() : binding_(this) {}

  void Bind(mojom::SharedWorkerClientRequest request) {
    binding_.Bind(std::move(request));
  }

  void Close() { binding_.Close(); }

  bool CheckReceivedOnCreated() {
    if (!on_created_received_)
      return false;
    on_created_received_ = false;
    return true;
  }

  bool CheckReceivedOnConnected(
      std::set<blink::mojom::WebFeature> expected_used_features) {
    if (!on_connected_received_)
      return false;
    on_connected_received_ = false;
    if (!CheckEquality(expected_used_features, on_connected_features_))
      return false;
    return true;
  }

  bool CheckReceivedOnFeatureUsed(blink::mojom::WebFeature expected_feature) {
    if (!on_feature_used_received_)
      return false;
    on_feature_used_received_ = false;
    if (!CheckEquality(expected_feature, on_feature_used_feature_))
      return false;
    return true;
  }

  bool CheckNotReceivedOnFeatureUsed() { return !on_feature_used_received_; }

 private:
  // mojom::SharedWorkerClient methods:
  void OnCreated(blink::mojom::SharedWorkerCreationContextType
                     creation_context_type) override {
    CHECK(!on_created_received_);
    on_created_received_ = true;
  }
  void OnConnected(
      const std::vector<blink::mojom::WebFeature>& features_used) override {
    CHECK(!on_connected_received_);
    on_connected_received_ = true;
    for (auto feature : features_used)
      on_connected_features_.insert(feature);
  }
  void OnScriptLoadFailed() override { NOTREACHED(); }
  void OnFeatureUsed(blink::mojom::WebFeature feature) override {
    CHECK(!on_feature_used_received_);
    on_feature_used_received_ = true;
    on_feature_used_feature_ = feature;
  }

  mojo::Binding<mojom::SharedWorkerClient> binding_;
  bool on_created_received_ = false;
  bool on_connected_received_ = false;
  std::set<blink::mojom::WebFeature> on_connected_features_;
  bool on_feature_used_received_ = false;
  blink::mojom::WebFeature on_feature_used_feature_ =
      blink::mojom::WebFeature();
};

class MockRenderProcessHostWrapper {
 public:
  MockRenderProcessHostWrapper(BrowserContext* browser_context,
                               const WorkerStoragePartition& partition)
      : process_host_(browser_context), partition_(partition) {
    process_host_.OverrideBinderForTesting(
        mojom::SharedWorkerFactory::Name_,
        base::Bind(&SharedWorkerServiceImplTest::BindSharedWorkerFactory));
  }

  ~MockRenderProcessHostWrapper() {}

  void FastShutdownIfPossible() {
    process_host_.FastShutdownIfPossible(0, true);
  }

  void CreateSharedWorkerConnector(
      int frame_id,
      mojom::SharedWorkerConnectorRequest request) {
    SharedWorkerServiceImplTest::CreateSharedWorkerConnector(
        process_host_.GetID(), frame_id,
        process_host_.GetBrowserContext()->GetResourceContext(), partition_,
        std::move(request));
  }

  size_t GetKeepAliveRefCount() const {
    return process_host_.GetKeepAliveRefCount();
  }

 private:
  MockRenderProcessHost process_host_;
  WorkerStoragePartition partition_;
};

class MockSharedWorkerConnector {
 public:
  MockSharedWorkerConnector(MockRenderProcessHostWrapper* renderer_host)
      : renderer_host_(renderer_host) {}

  void Create(const std::string& url,
              const std::string& name,
              int render_frame_route_id,
              MockSharedWorkerClient* client) {
    mojom::SharedWorkerConnectorPtr connector;
    renderer_host_->CreateSharedWorkerConnector(render_frame_route_id,
                                                mojo::MakeRequest(&connector));

    mojom::SharedWorkerInfoPtr info(mojom::SharedWorkerInfo::New(
        GURL(url), name, std::string(),
        blink::kWebContentSecurityPolicyTypeReport,
        blink::kWebAddressSpacePublic,
        false /* data_saver_enabled */));

    mojo::MessagePipe message_pipe;
    local_port_ = MessagePortChannel(std::move(message_pipe.handle0));

    mojom::SharedWorkerClientPtr client_proxy;
    client->Bind(mojo::MakeRequest(&client_proxy));

    connector->Connect(std::move(info), std::move(client_proxy),
                       blink::mojom::SharedWorkerCreationContextType::kSecure,
                       std::move(message_pipe.handle1));
  }

  MessagePortChannel local_port() { return local_port_; }

 private:
  mojom::SharedWorkerClientRequest client_request_;
  MockRenderProcessHostWrapper* renderer_host_;
  MessagePortChannel local_port_;
};

}  // namespace

TEST_F(SharedWorkerServiceImplTest, BasicTest) {
  std::unique_ptr<MockRenderProcessHostWrapper> renderer_host(
      new MockRenderProcessHostWrapper(browser_context_.get(),
                                       *partition_.get()));
  std::unique_ptr<MockSharedWorkerConnector> connector(
      new MockSharedWorkerConnector(renderer_host.get()));
  MockSharedWorkerClient client;

  connector->Create("http://example.com/w.js", "name", kRenderFrameRouteIDs[0],
                    &client);
  RunAllPendingInMessageLoop();

  mojom::SharedWorkerFactoryRequest factory_request;
  EXPECT_TRUE(CheckReceivedFactoryRequest(&factory_request));
  MockSharedWorkerFactory factory(std::move(factory_request));
  RunAllPendingInMessageLoop();

  mojom::SharedWorkerHostPtr worker_host;
  mojom::SharedWorkerRequest worker_request;
  EXPECT_TRUE(factory.CheckReceivedCreateSharedWorker(
      "http://example.com/w.js", "name",
      blink::kWebContentSecurityPolicyTypeReport, &worker_host,
      &worker_request));
  MockSharedWorker worker(std::move(worker_request));
  RunAllPendingInMessageLoop();

  int connection_request_id;
  MessagePortChannel port;
  EXPECT_TRUE(worker.CheckReceivedConnect(&connection_request_id, &port));

  client.CheckReceivedOnCreated();

  // Simulate events the shared worker would send.
  worker_host->OnReadyForInspection();
  worker_host->OnScriptLoaded();
  worker_host->OnConnected(connection_request_id);

  RunAllPendingInMessageLoop();

  EXPECT_TRUE(
      client.CheckReceivedOnConnected(std::set<blink::mojom::WebFeature>()));

  // Verify that |port| corresponds to |connector->local_port()|.
  std::vector<uint8_t> expected_message(StringPieceToVector("test1"));
  connector->local_port().PostMessage(expected_message.data(),
                                      expected_message.size(),
                                      std::vector<MessagePortChannel>());
  std::vector<uint8_t> received_message;
  BlockingReadFromMessagePort(port, &received_message);
  EXPECT_EQ(expected_message, received_message);

  // Send feature from shared worker to host.
  auto feature1 = static_cast<blink::mojom::WebFeature>(124);
  worker_host->OnFeatureUsed(feature1);
  RunAllPendingInMessageLoop();
  EXPECT_TRUE(client.CheckReceivedOnFeatureUsed(feature1));

  // A message should be sent only one time per feature.
  worker_host->OnFeatureUsed(feature1);
  RunAllPendingInMessageLoop();
  EXPECT_TRUE(client.CheckNotReceivedOnFeatureUsed());

  // Send another feature.
  auto feature2 = static_cast<blink::mojom::WebFeature>(901);
  worker_host->OnFeatureUsed(feature2);
  RunAllPendingInMessageLoop();
  EXPECT_TRUE(client.CheckReceivedOnFeatureUsed(feature2));
}

TEST_F(SharedWorkerServiceImplTest, TwoRendererTest) {
  // The first renderer host.
  std::unique_ptr<MockRenderProcessHostWrapper> renderer_host0(
      new MockRenderProcessHostWrapper(browser_context_.get(),
                                       *partition_.get()));
  std::unique_ptr<MockSharedWorkerConnector> connector0(
      new MockSharedWorkerConnector(renderer_host0.get()));
  MockSharedWorkerClient client0;

  connector0->Create("http://example.com/w.js", "name", kRenderFrameRouteIDs[0],
                     &client0);
  RunAllPendingInMessageLoop();

  mojom::SharedWorkerFactoryRequest factory_request;
  EXPECT_TRUE(CheckReceivedFactoryRequest(&factory_request));
  MockSharedWorkerFactory factory(std::move(factory_request));
  RunAllPendingInMessageLoop();

  mojom::SharedWorkerHostPtr worker_host;
  mojom::SharedWorkerRequest worker_request;
  EXPECT_TRUE(factory.CheckReceivedCreateSharedWorker(
      "http://example.com/w.js", "name",
      blink::kWebContentSecurityPolicyTypeReport, &worker_host,
      &worker_request));
  MockSharedWorker worker(std::move(worker_request));
  RunAllPendingInMessageLoop();

  int connection_request_id0;
  MessagePortChannel port0;
  EXPECT_TRUE(worker.CheckReceivedConnect(&connection_request_id0, &port0));

  client0.CheckReceivedOnCreated();

  // Simulate events the shared worker would send.
  worker_host->OnReadyForInspection();
  worker_host->OnScriptLoaded();
  worker_host->OnConnected(connection_request_id0);

  RunAllPendingInMessageLoop();

  EXPECT_TRUE(
      client0.CheckReceivedOnConnected(std::set<blink::mojom::WebFeature>()));

  // Verify that |port0| corresponds to |connector0->local_port()|.
  std::vector<uint8_t> expected_message0(StringPieceToVector("test1"));
  connector0->local_port().PostMessage(expected_message0.data(),
                                       expected_message0.size(),
                                       std::vector<MessagePortChannel>());
  std::vector<uint8_t> received_message0;
  BlockingReadFromMessagePort(port0, &received_message0);
  EXPECT_EQ(expected_message0, received_message0);

  auto feature1 = static_cast<blink::mojom::WebFeature>(124);
  worker_host->OnFeatureUsed(feature1);
  RunAllPendingInMessageLoop();
  EXPECT_TRUE(client0.CheckReceivedOnFeatureUsed(feature1));
  auto feature2 = static_cast<blink::mojom::WebFeature>(901);
  worker_host->OnFeatureUsed(feature2);
  RunAllPendingInMessageLoop();
  EXPECT_TRUE(client0.CheckReceivedOnFeatureUsed(feature2));

  // Only a single worker instance in process 0.
  EXPECT_EQ(1u, renderer_host0->GetKeepAliveRefCount());

  // The second renderer host.
  std::unique_ptr<MockRenderProcessHostWrapper> renderer_host1(
      new MockRenderProcessHostWrapper(browser_context_.get(),
                                       *partition_.get()));
  std::unique_ptr<MockSharedWorkerConnector> connector1(
      new MockSharedWorkerConnector(renderer_host1.get()));
  MockSharedWorkerClient client1;

  connector1->Create("http://example.com/w.js", "name", kRenderFrameRouteIDs[1],
                     &client1);
  RunAllPendingInMessageLoop();

  // Should not have tried to create a new shared worker.
  EXPECT_TRUE(CheckNotReceivedFactoryRequest());

  int connection_request_id1;
  MessagePortChannel port1;
  EXPECT_TRUE(worker.CheckReceivedConnect(&connection_request_id1, &port1));

  client1.CheckReceivedOnCreated();

  // Only a single worker instance in process 0.
  EXPECT_EQ(1u, renderer_host0->GetKeepAliveRefCount());
  EXPECT_EQ(0u, renderer_host1->GetKeepAliveRefCount());

  worker_host->OnConnected(connection_request_id1);

  RunAllPendingInMessageLoop();

  EXPECT_TRUE(client1.CheckReceivedOnConnected({feature1, feature2}));

  // Verify that |worker_msg_port2| corresponds to |connector1->local_port()|.
  std::vector<uint8_t> expected_message1(StringPieceToVector("test2"));
  connector1->local_port().PostMessage(expected_message1.data(),
                                       expected_message1.size(),
                                       std::vector<MessagePortChannel>());
  std::vector<uint8_t> received_message1;
  BlockingReadFromMessagePort(port1, &received_message1);
  EXPECT_EQ(expected_message1, received_message1);

  worker_host->OnFeatureUsed(feature1);
  RunAllPendingInMessageLoop();
  EXPECT_TRUE(client0.CheckNotReceivedOnFeatureUsed());
  EXPECT_TRUE(client1.CheckNotReceivedOnFeatureUsed());

  auto feature3 = static_cast<blink::mojom::WebFeature>(1019);
  worker_host->OnFeatureUsed(feature3);
  RunAllPendingInMessageLoop();
  EXPECT_TRUE(client0.CheckReceivedOnFeatureUsed(feature3));
  EXPECT_TRUE(client1.CheckReceivedOnFeatureUsed(feature3));

  renderer_host1.reset();
  renderer_host0.reset();
}

TEST_F(SharedWorkerServiceImplTest, CreateWorkerTest_NormalCase) {
  const char kURL[] = "http://example.com/w.js";
  const char kName[] = "name";

  // The first renderer host.
  std::unique_ptr<MockRenderProcessHostWrapper> renderer_host0(
      new MockRenderProcessHostWrapper(browser_context_.get(),
                                       *partition_.get()));
  // The second renderer host.
  std::unique_ptr<MockRenderProcessHostWrapper> renderer_host1(
      new MockRenderProcessHostWrapper(browser_context_.get(),
                                       *partition_.get()));

  // First client, creates worker.

  std::unique_ptr<MockSharedWorkerConnector> connector0(
      new MockSharedWorkerConnector(renderer_host0.get()));
  MockSharedWorkerClient client0;
  connector0->Create(kURL, kName, kRenderFrameRouteIDs[0], &client0);
  RunAllPendingInMessageLoop();

  mojom::SharedWorkerFactoryRequest factory_request;
  EXPECT_TRUE(CheckReceivedFactoryRequest(&factory_request));
  MockSharedWorkerFactory factory(std::move(factory_request));
  RunAllPendingInMessageLoop();

  mojom::SharedWorkerHostPtr worker_host;
  mojom::SharedWorkerRequest worker_request;
  EXPECT_TRUE(factory.CheckReceivedCreateSharedWorker(
      kURL, kName, blink::kWebContentSecurityPolicyTypeReport, &worker_host,
      &worker_request));
  MockSharedWorker worker(std::move(worker_request));
  RunAllPendingInMessageLoop();

  EXPECT_TRUE(worker.CheckReceivedConnect(nullptr, nullptr));
  client0.CheckReceivedOnCreated();

  // Second client, same worker.

  std::unique_ptr<MockSharedWorkerConnector> connector1(
      new MockSharedWorkerConnector(renderer_host1.get()));
  MockSharedWorkerClient client1;
  connector1->Create(kURL, kName, kRenderFrameRouteIDs[1], &client1);
  RunAllPendingInMessageLoop();

  EXPECT_TRUE(CheckNotReceivedFactoryRequest());

  EXPECT_TRUE(worker.CheckReceivedConnect(nullptr, nullptr));
  client1.CheckReceivedOnCreated();

  // Cleanup

  client0.Close();
  client1.Close();
  RunAllPendingInMessageLoop();

  EXPECT_TRUE(worker.CheckReceivedTerminate());
}

TEST_F(SharedWorkerServiceImplTest, CreateWorkerTest_NormalCase_URLMismatch) {
  const char kURL0[] = "http://example.com/w0.js";
  const char kURL1[] = "http://example.com/w1.js";
  const char kName[] = "name";

  // The first renderer host.
  std::unique_ptr<MockRenderProcessHostWrapper> renderer_host0(
      new MockRenderProcessHostWrapper(browser_context_.get(),
                                       *partition_.get()));
  // The second renderer host.
  std::unique_ptr<MockRenderProcessHostWrapper> renderer_host1(
      new MockRenderProcessHostWrapper(browser_context_.get(),
                                       *partition_.get()));

  // First client, creates worker.

  std::unique_ptr<MockSharedWorkerConnector> connector0(
      new MockSharedWorkerConnector(renderer_host0.get()));
  MockSharedWorkerClient client0;
  connector0->Create(kURL0, kName, kRenderFrameRouteIDs[0], &client0);
  RunAllPendingInMessageLoop();

  mojom::SharedWorkerFactoryRequest factory_request0;
  EXPECT_TRUE(CheckReceivedFactoryRequest(&factory_request0));
  MockSharedWorkerFactory factory0(std::move(factory_request0));
  RunAllPendingInMessageLoop();

  mojom::SharedWorkerHostPtr worker_host0;
  mojom::SharedWorkerRequest worker_request0;
  EXPECT_TRUE(factory0.CheckReceivedCreateSharedWorker(
      kURL0, kName, blink::kWebContentSecurityPolicyTypeReport, &worker_host0,
      &worker_request0));
  MockSharedWorker worker0(std::move(worker_request0));
  RunAllPendingInMessageLoop();

  EXPECT_TRUE(worker0.CheckReceivedConnect(nullptr, nullptr));
  client0.CheckReceivedOnCreated();

  // Second client, creates worker.

  std::unique_ptr<MockSharedWorkerConnector> connector1(
      new MockSharedWorkerConnector(renderer_host1.get()));
  MockSharedWorkerClient client1;
  connector1->Create(kURL1, kName, kRenderFrameRouteIDs[1], &client1);
  RunAllPendingInMessageLoop();

  mojom::SharedWorkerFactoryRequest factory_request1;
  EXPECT_TRUE(CheckReceivedFactoryRequest(&factory_request1));
  MockSharedWorkerFactory factory1(std::move(factory_request1));
  RunAllPendingInMessageLoop();

  mojom::SharedWorkerHostPtr worker_host1;
  mojom::SharedWorkerRequest worker_request1;
  EXPECT_TRUE(factory1.CheckReceivedCreateSharedWorker(
      kURL1, kName, blink::kWebContentSecurityPolicyTypeReport, &worker_host1,
      &worker_request1));
  MockSharedWorker worker1(std::move(worker_request1));
  RunAllPendingInMessageLoop();

  EXPECT_TRUE(worker1.CheckReceivedConnect(nullptr, nullptr));
  client1.CheckReceivedOnCreated();

  // Cleanup

  client0.Close();
  client1.Close();
  RunAllPendingInMessageLoop();

  EXPECT_TRUE(worker0.CheckReceivedTerminate());
  EXPECT_TRUE(worker1.CheckReceivedTerminate());
}

TEST_F(SharedWorkerServiceImplTest, CreateWorkerTest_NormalCase_NameMismatch) {
  const char kURL[] = "http://example.com/w.js";
  const char kName0[] = "name0";
  const char kName1[] = "name1";

  // The first renderer host.
  std::unique_ptr<MockRenderProcessHostWrapper> renderer_host0(
      new MockRenderProcessHostWrapper(browser_context_.get(),
                                       *partition_.get()));
  // The second renderer host.
  std::unique_ptr<MockRenderProcessHostWrapper> renderer_host1(
      new MockRenderProcessHostWrapper(browser_context_.get(),
                                       *partition_.get()));

  // First client, creates worker.

  std::unique_ptr<MockSharedWorkerConnector> connector0(
      new MockSharedWorkerConnector(renderer_host0.get()));
  MockSharedWorkerClient client0;
  connector0->Create(kURL, kName0, kRenderFrameRouteIDs[0], &client0);
  RunAllPendingInMessageLoop();

  mojom::SharedWorkerFactoryRequest factory_request0;
  EXPECT_TRUE(CheckReceivedFactoryRequest(&factory_request0));
  MockSharedWorkerFactory factory0(std::move(factory_request0));
  RunAllPendingInMessageLoop();

  mojom::SharedWorkerHostPtr worker_host0;
  mojom::SharedWorkerRequest worker_request0;
  EXPECT_TRUE(factory0.CheckReceivedCreateSharedWorker(
      kURL, kName0, blink::kWebContentSecurityPolicyTypeReport, &worker_host0,
      &worker_request0));
  MockSharedWorker worker0(std::move(worker_request0));
  RunAllPendingInMessageLoop();

  EXPECT_TRUE(worker0.CheckReceivedConnect(nullptr, nullptr));
  client0.CheckReceivedOnCreated();

  // Second client, creates worker.

  std::unique_ptr<MockSharedWorkerConnector> connector1(
      new MockSharedWorkerConnector(renderer_host1.get()));
  MockSharedWorkerClient client1;
  connector1->Create(kURL, kName1, kRenderFrameRouteIDs[1], &client1);
  RunAllPendingInMessageLoop();

  mojom::SharedWorkerFactoryRequest factory_request1;
  EXPECT_TRUE(CheckReceivedFactoryRequest(&factory_request1));
  MockSharedWorkerFactory factory1(std::move(factory_request1));
  RunAllPendingInMessageLoop();

  mojom::SharedWorkerHostPtr worker_host1;
  mojom::SharedWorkerRequest worker_request1;
  EXPECT_TRUE(factory1.CheckReceivedCreateSharedWorker(
      kURL, kName1, blink::kWebContentSecurityPolicyTypeReport, &worker_host1,
      &worker_request1));
  MockSharedWorker worker1(std::move(worker_request1));
  RunAllPendingInMessageLoop();

  EXPECT_TRUE(worker1.CheckReceivedConnect(nullptr, nullptr));
  client1.CheckReceivedOnCreated();

  // Cleanup

  client0.Close();
  client1.Close();
  RunAllPendingInMessageLoop();

  EXPECT_TRUE(worker0.CheckReceivedTerminate());
  EXPECT_TRUE(worker1.CheckReceivedTerminate());
}

TEST_F(SharedWorkerServiceImplTest, CreateWorkerTest_PendingCase) {
  const char kURL[] = "http://example.com/w.js";
  const char kName[] = "name";

  // The first renderer host.
  std::unique_ptr<MockRenderProcessHostWrapper> renderer_host0(
      new MockRenderProcessHostWrapper(browser_context_.get(),
                                       *partition_.get()));
  // The second renderer host.
  std::unique_ptr<MockRenderProcessHostWrapper> renderer_host1(
      new MockRenderProcessHostWrapper(browser_context_.get(),
                                       *partition_.get()));

  // First client and second client are created before the worker starts.

  std::unique_ptr<MockSharedWorkerConnector> connector0(
      new MockSharedWorkerConnector(renderer_host0.get()));
  MockSharedWorkerClient client0;
  connector0->Create(kURL, kName, kRenderFrameRouteIDs[0], &client0);

  std::unique_ptr<MockSharedWorkerConnector> connector1(
      new MockSharedWorkerConnector(renderer_host1.get()));
  MockSharedWorkerClient client1;
  connector1->Create(kURL, kName, kRenderFrameRouteIDs[1], &client1);

  RunAllPendingInMessageLoop();

  // Check that the worker was created.

  mojom::SharedWorkerFactoryRequest factory_request;
  EXPECT_TRUE(CheckReceivedFactoryRequest(&factory_request));
  MockSharedWorkerFactory factory(std::move(factory_request));

  RunAllPendingInMessageLoop();

  mojom::SharedWorkerHostPtr worker_host;
  mojom::SharedWorkerRequest worker_request;
  EXPECT_TRUE(factory.CheckReceivedCreateSharedWorker(
      kURL, kName, blink::kWebContentSecurityPolicyTypeReport, &worker_host,
      &worker_request));
  MockSharedWorker worker(std::move(worker_request));

  RunAllPendingInMessageLoop();

  // Check that the worker received two connections.

  EXPECT_TRUE(worker.CheckReceivedConnect(nullptr, nullptr));
  client0.CheckReceivedOnCreated();

  EXPECT_TRUE(worker.CheckReceivedConnect(nullptr, nullptr));
  client1.CheckReceivedOnCreated();

  // Cleanup

  client0.Close();
  client1.Close();
  RunAllPendingInMessageLoop();

  EXPECT_TRUE(worker.CheckReceivedTerminate());
}

TEST_F(SharedWorkerServiceImplTest, CreateWorkerTest_PendingCase_URLMismatch) {
  const char kURL0[] = "http://example.com/w0.js";
  const char kURL1[] = "http://example.com/w1.js";
  const char kName[] = "name";

  // The first renderer host.
  std::unique_ptr<MockRenderProcessHostWrapper> renderer_host0(
      new MockRenderProcessHostWrapper(browser_context_.get(),
                                       *partition_.get()));
  // The second renderer host.
  std::unique_ptr<MockRenderProcessHostWrapper> renderer_host1(
      new MockRenderProcessHostWrapper(browser_context_.get(),
                                       *partition_.get()));

  // First client and second client are created before the workers start.

  std::unique_ptr<MockSharedWorkerConnector> connector0(
      new MockSharedWorkerConnector(renderer_host0.get()));
  MockSharedWorkerClient client0;
  connector0->Create(kURL0, kName, kRenderFrameRouteIDs[0], &client0);

  std::unique_ptr<MockSharedWorkerConnector> connector1(
      new MockSharedWorkerConnector(renderer_host1.get()));
  MockSharedWorkerClient client1;
  connector1->Create(kURL1, kName, kRenderFrameRouteIDs[1], &client1);

  RunAllPendingInMessageLoop();

  // Check that both workers were created.

  mojom::SharedWorkerFactoryRequest factory_request0;
  EXPECT_TRUE(CheckReceivedFactoryRequest(&factory_request0));
  MockSharedWorkerFactory factory0(std::move(factory_request0));

  mojom::SharedWorkerFactoryRequest factory_request1;
  EXPECT_TRUE(CheckReceivedFactoryRequest(&factory_request1));
  MockSharedWorkerFactory factory1(std::move(factory_request1));

  RunAllPendingInMessageLoop();

  mojom::SharedWorkerHostPtr worker_host0;
  mojom::SharedWorkerRequest worker_request0;
  EXPECT_TRUE(factory0.CheckReceivedCreateSharedWorker(
      kURL0, kName, blink::kWebContentSecurityPolicyTypeReport, &worker_host0,
      &worker_request0));
  MockSharedWorker worker0(std::move(worker_request0));

  mojom::SharedWorkerHostPtr worker_host1;
  mojom::SharedWorkerRequest worker_request1;
  EXPECT_TRUE(factory1.CheckReceivedCreateSharedWorker(
      kURL1, kName, blink::kWebContentSecurityPolicyTypeReport, &worker_host1,
      &worker_request1));
  MockSharedWorker worker1(std::move(worker_request1));

  RunAllPendingInMessageLoop();

  // Check that the workers each received a connection.

  EXPECT_TRUE(worker0.CheckReceivedConnect(nullptr, nullptr));
  EXPECT_TRUE(worker0.CheckNotReceivedConnect());
  client0.CheckReceivedOnCreated();

  EXPECT_TRUE(worker1.CheckReceivedConnect(nullptr, nullptr));
  EXPECT_TRUE(worker1.CheckNotReceivedConnect());
  client1.CheckReceivedOnCreated();

  // Cleanup

  client0.Close();
  client1.Close();
  RunAllPendingInMessageLoop();

  EXPECT_TRUE(worker0.CheckReceivedTerminate());
  EXPECT_TRUE(worker1.CheckReceivedTerminate());
}

TEST_F(SharedWorkerServiceImplTest, CreateWorkerTest_PendingCase_NameMismatch) {
  const char kURL[] = "http://example.com/w.js";
  const char kName0[] = "name0";
  const char kName1[] = "name1";

  // The first renderer host.
  std::unique_ptr<MockRenderProcessHostWrapper> renderer_host0(
      new MockRenderProcessHostWrapper(browser_context_.get(),
                                       *partition_.get()));
  // The second renderer host.
  std::unique_ptr<MockRenderProcessHostWrapper> renderer_host1(
      new MockRenderProcessHostWrapper(browser_context_.get(),
                                       *partition_.get()));

  // First client and second client are created before the workers start.

  std::unique_ptr<MockSharedWorkerConnector> connector0(
      new MockSharedWorkerConnector(renderer_host0.get()));
  MockSharedWorkerClient client0;
  connector0->Create(kURL, kName0, kRenderFrameRouteIDs[0], &client0);

  std::unique_ptr<MockSharedWorkerConnector> connector1(
      new MockSharedWorkerConnector(renderer_host1.get()));
  MockSharedWorkerClient client1;
  connector1->Create(kURL, kName1, kRenderFrameRouteIDs[1], &client1);

  RunAllPendingInMessageLoop();

  // Check that both workers were created.

  mojom::SharedWorkerFactoryRequest factory_request0;
  EXPECT_TRUE(CheckReceivedFactoryRequest(&factory_request0));
  MockSharedWorkerFactory factory0(std::move(factory_request0));

  mojom::SharedWorkerFactoryRequest factory_request1;
  EXPECT_TRUE(CheckReceivedFactoryRequest(&factory_request1));
  MockSharedWorkerFactory factory1(std::move(factory_request1));

  RunAllPendingInMessageLoop();

  mojom::SharedWorkerHostPtr worker_host0;
  mojom::SharedWorkerRequest worker_request0;
  EXPECT_TRUE(factory0.CheckReceivedCreateSharedWorker(
      kURL, kName0, blink::kWebContentSecurityPolicyTypeReport, &worker_host0,
      &worker_request0));
  MockSharedWorker worker0(std::move(worker_request0));

  mojom::SharedWorkerHostPtr worker_host1;
  mojom::SharedWorkerRequest worker_request1;
  EXPECT_TRUE(factory1.CheckReceivedCreateSharedWorker(
      kURL, kName1, blink::kWebContentSecurityPolicyTypeReport, &worker_host1,
      &worker_request1));
  MockSharedWorker worker1(std::move(worker_request1));

  RunAllPendingInMessageLoop();

  // Check that the workers each received a connection.

  EXPECT_TRUE(worker0.CheckReceivedConnect(nullptr, nullptr));
  EXPECT_TRUE(worker0.CheckNotReceivedConnect());
  client0.CheckReceivedOnCreated();

  EXPECT_TRUE(worker1.CheckReceivedConnect(nullptr, nullptr));
  EXPECT_TRUE(worker1.CheckNotReceivedConnect());
  client1.CheckReceivedOnCreated();

  // Cleanup

  client0.Close();
  client1.Close();
  RunAllPendingInMessageLoop();

  EXPECT_TRUE(worker0.CheckReceivedTerminate());
  EXPECT_TRUE(worker1.CheckReceivedTerminate());
}

TEST_F(SharedWorkerServiceImplTest, CreateWorkerRaceTest) {
  const char kURL[] = "http://example.com/w.js";
  const char kName[] = "name";

  // Create three renderer hosts.
  std::unique_ptr<MockRenderProcessHostWrapper> renderer_host0(
      new MockRenderProcessHostWrapper(browser_context_.get(),
                                       *partition_.get()));
  std::unique_ptr<MockRenderProcessHostWrapper> renderer_host1(
      new MockRenderProcessHostWrapper(browser_context_.get(),
                                       *partition_.get()));
  std::unique_ptr<MockRenderProcessHostWrapper> renderer_host2(
      new MockRenderProcessHostWrapper(browser_context_.get(),
                                       *partition_.get()));

  std::unique_ptr<MockSharedWorkerConnector> connector0(
      new MockSharedWorkerConnector(renderer_host0.get()));
  std::unique_ptr<MockSharedWorkerConnector> connector1(
      new MockSharedWorkerConnector(renderer_host1.get()));
  std::unique_ptr<MockSharedWorkerConnector> connector2(
      new MockSharedWorkerConnector(renderer_host2.get()));

  MockSharedWorkerClient client0;
  connector0->Create(kURL, kName, kRenderFrameRouteIDs[0], &client0);

  RunAllPendingInMessageLoop();

  // Starts a worker.

  mojom::SharedWorkerFactoryRequest factory_request0;
  EXPECT_TRUE(CheckReceivedFactoryRequest(&factory_request0));
  MockSharedWorkerFactory factory0(std::move(factory_request0));

  RunAllPendingInMessageLoop();

  mojom::SharedWorkerHostPtr worker_host0;
  mojom::SharedWorkerRequest worker_request0;
  EXPECT_TRUE(factory0.CheckReceivedCreateSharedWorker(
      kURL, kName, blink::kWebContentSecurityPolicyTypeReport, &worker_host0,
      &worker_request0));
  MockSharedWorker worker0(std::move(worker_request0));

  RunAllPendingInMessageLoop();

  EXPECT_TRUE(worker0.CheckReceivedConnect(nullptr, nullptr));
  client0.CheckReceivedOnCreated();

  // Kill this process, which should make worker0 unavailable.
  renderer_host0->FastShutdownIfPossible();

  // Start a new client, attemping to connect to the same worker.
  MockSharedWorkerClient client1;
  connector1->Create(kURL, kName, kRenderFrameRouteIDs[1], &client1);

  RunAllPendingInMessageLoop();

  // The previous worker is unavailable, so a new worker is created.

  mojom::SharedWorkerFactoryRequest factory_request1;
  EXPECT_TRUE(CheckReceivedFactoryRequest(&factory_request1));
  MockSharedWorkerFactory factory1(std::move(factory_request1));

  RunAllPendingInMessageLoop();

  mojom::SharedWorkerHostPtr worker_host1;
  mojom::SharedWorkerRequest worker_request1;
  EXPECT_TRUE(factory1.CheckReceivedCreateSharedWorker(
      kURL, kName, blink::kWebContentSecurityPolicyTypeReport, &worker_host1,
      &worker_request1));
  MockSharedWorker worker1(std::move(worker_request1));

  RunAllPendingInMessageLoop();

  EXPECT_TRUE(worker0.CheckNotReceivedConnect());
  EXPECT_TRUE(worker1.CheckReceivedConnect(nullptr, nullptr));
  client1.CheckReceivedOnCreated();

  // Start another client to confirm that it can connect to the same worker.
  MockSharedWorkerClient client2;
  connector2->Create(kURL, kName, kRenderFrameRouteIDs[2], &client2);

  RunAllPendingInMessageLoop();

  EXPECT_TRUE(CheckNotReceivedFactoryRequest());

  EXPECT_TRUE(worker1.CheckReceivedConnect(nullptr, nullptr));
  client2.CheckReceivedOnCreated();
}

TEST_F(SharedWorkerServiceImplTest, CreateWorkerRaceTest2) {
  const char kURL[] = "http://example.com/w.js";
  const char kName[] = "name";

  // Create three renderer hosts.
  std::unique_ptr<MockRenderProcessHostWrapper> renderer_host0(
      new MockRenderProcessHostWrapper(browser_context_.get(),
                                       *partition_.get()));
  std::unique_ptr<MockRenderProcessHostWrapper> renderer_host1(
      new MockRenderProcessHostWrapper(browser_context_.get(),
                                       *partition_.get()));
  std::unique_ptr<MockRenderProcessHostWrapper> renderer_host2(
      new MockRenderProcessHostWrapper(browser_context_.get(),
                                       *partition_.get()));

  std::unique_ptr<MockSharedWorkerConnector> connector0(
      new MockSharedWorkerConnector(renderer_host0.get()));
  std::unique_ptr<MockSharedWorkerConnector> connector1(
      new MockSharedWorkerConnector(renderer_host1.get()));
  std::unique_ptr<MockSharedWorkerConnector> connector2(
      new MockSharedWorkerConnector(renderer_host2.get()));

  MockSharedWorkerClient client0;
  connector0->Create(kURL, kName, kRenderFrameRouteIDs[0], &client0);

  // Kill this process, which should make worker0 unavailable.
  renderer_host0->FastShutdownIfPossible();

  // Start a new client, attemping to connect to the same worker.
  MockSharedWorkerClient client1;
  connector1->Create(kURL, kName, kRenderFrameRouteIDs[1], &client1);

  RunAllPendingInMessageLoop();

  // The previous worker is unavailable, so a new worker is created.

  mojom::SharedWorkerFactoryRequest factory_request1;
  EXPECT_TRUE(CheckReceivedFactoryRequest(&factory_request1));
  MockSharedWorkerFactory factory1(std::move(factory_request1));

  EXPECT_TRUE(CheckNotReceivedFactoryRequest());

  RunAllPendingInMessageLoop();

  mojom::SharedWorkerHostPtr worker_host1;
  mojom::SharedWorkerRequest worker_request1;
  EXPECT_TRUE(factory1.CheckReceivedCreateSharedWorker(
      kURL, kName, blink::kWebContentSecurityPolicyTypeReport, &worker_host1,
      &worker_request1));
  MockSharedWorker worker1(std::move(worker_request1));

  RunAllPendingInMessageLoop();

  EXPECT_TRUE(worker1.CheckReceivedConnect(nullptr, nullptr));
  client1.CheckReceivedOnCreated();

  // Start another client to confirm that it can connect to the same worker.
  MockSharedWorkerClient client2;
  connector2->Create(kURL, kName, kRenderFrameRouteIDs[2], &client2);

  RunAllPendingInMessageLoop();

  EXPECT_TRUE(CheckNotReceivedFactoryRequest());

  EXPECT_TRUE(worker1.CheckReceivedConnect(nullptr, nullptr));
  client2.CheckReceivedOnCreated();
}

}  // namespace content
