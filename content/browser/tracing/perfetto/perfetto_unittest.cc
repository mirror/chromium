// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <memory>

#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/message_loop/message_loop.h"
#include "base/threading/sequenced_task_runner_handle.h"

#include "base/values.h"
#include "content/public/test/test_browser_thread.h"
#include "mojo/public/cpp/system/buffer.h"
#include "testing/gtest/include/gtest/gtest.h"

#include "content/browser/tracing/perfetto/producer.mojom.h"
#include "content/browser/tracing/perfetto/producer_service.mojom.h"
#include "content/browser/tracing/perfetto/remote_producer.mojom.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "mojo/public/cpp/bindings/binding_set.h"
#include "mojo/public/cpp/bindings/interface_ptr_set.h"

#undef OS_ANDROID

#ifdef BUILDFLAG_CAT_INDIRECT
#undef BUILDFLAG_CAT_INDIRECT
#endif

#ifdef BUILDFLAG
#undef BUILDFLAG
#endif

#include "perfetto/base/task_runner.h"
#include "perfetto/tracing/core/consumer.h"
#include "perfetto/tracing/core/data_source_descriptor.h"
#include "perfetto/tracing/core/producer.h"
#include "perfetto/tracing/core/service.h"
#include "perfetto/tracing/core/shared_memory.h"
#include "perfetto/tracing/core/shared_memory_arbiter.h"
#include "perfetto/tracing/core/trace_config.h"
#include "perfetto/tracing/core/trace_packet.h"
#include "perfetto/tracing/core/trace_writer.h"

#include "perfetto/trace/test_event.pbzero.h"
#include "perfetto/trace/trace_packet.pb.h"
#include "perfetto/trace/trace_packet.pbzero.h"

namespace {
const char kPerfettoDataSourceName[] = "perfetto_chrome_test";
}  // namespace

class MojoSharedMemory : public perfetto::SharedMemory {
 public:
  class Factory : public perfetto::SharedMemory::Factory {
   public:
    std::unique_ptr<perfetto::SharedMemory> CreateSharedMemory(
        size_t size) override {
      LOG(ERROR) << "MojoSharedMemory::CreateSharedMemory(" << size << ");";

      return std::make_unique<MojoSharedMemory>(size);
    }
  };

  MojoSharedMemory(size_t size) : size_(size) {
    shared_buffer_ = mojo::SharedBufferHandle::Create(size_);
    mapping_ = shared_buffer_->Map(size_);
    DCHECK(mapping_);
  }

  MojoSharedMemory(mojo::ScopedSharedBufferHandle shared_memory, size_t size)
      : shared_buffer_(std::move(shared_memory)), size_(size) {
    mapping_ = shared_buffer_->Map(size_);
    DCHECK(mapping_);
    LOG(ERROR) << "MojoSharedMemory::MojoSharedMemory: Duped with size "
               << size;
  }

  ~MojoSharedMemory() override = default;

  mojo::ScopedSharedBufferHandle Clone() {
    return shared_buffer_->Clone(
        mojo::SharedBufferHandle::AccessMode::READ_WRITE);
  }

  void* start() const override { return mapping_.get(); }

  size_t size() const override { return size_; }

 private:
  mojo::ScopedSharedBufferHandle shared_buffer_;
  mojo::ScopedSharedBufferMapping mapping_;
  size_t size_;
};

class PerfettoTaskRunner : public perfetto::base::TaskRunner {
 public:
  void PostTask(std::function<void()> task) override {
    base::SequencedTaskRunnerHandle::Get()->PostTask(
        FROM_HERE, base::BindOnce(&StdFunctionRunner::Run,
                                  std::make_unique<StdFunctionRunner>(task)));
  }

  void PostDelayedTask(std::function<void()> task, int delay_ms) override {
    base::SequencedTaskRunnerHandle::Get()->PostDelayedTask(
        FROM_HERE,
        base::BindOnce(&StdFunctionRunner::Run,
                       std::make_unique<StdFunctionRunner>(task)),
        base::TimeDelta::FromMilliseconds(delay_ms));
  }

  void AddFileDescriptorWatch(int fd, std::function<void()>) override {
    NOTREACHED();
  }

  void RemoveFileDescriptorWatch(int fd) override { NOTREACHED(); }

 private:
  // This seems like it should be unnecessary.
  class StdFunctionRunner {
   public:
    StdFunctionRunner(std::function<void()> task) : task_(task) {}

    void Run() { task_(); }

   private:
    std::function<void()> task_;
  };
};

class ProducerClientBase : public perfetto::mojom::ProducerClient {
 public:
  ProducerClientBase() = default;
  ~ProducerClientBase() override = default;

  void Initialize(perfetto::mojom::ProducerServicePtr producer_service) {
    perfetto::mojom::ProducerClientPtr producer_client;
    binding_ = std::make_unique<mojo::Binding<perfetto::mojom::ProducerClient>>(
        this, mojo::MakeRequest(&producer_client));

    ConnectToProducer(std::move(producer_client), std::move(producer_service));
  }

  // perfetto::mojom::ProducerClient implementation
  void ConnectProducer(perfetto::mojom::ProducerPtr producer) override {
    LOG(ERROR) << "ProducerBase::ConnectProducer();";
    producer_ = std::move(producer);
  }

  void EnableDataSource(mojo::ScopedSharedBufferHandle shared_memory,
                        uint64_t size) override {
    LOG(ERROR) << "ProducerClientBase::EnableDataSource();";
    shared_memory_ =
        std::make_unique<MojoSharedMemory>(std::move(shared_memory), size);

    auto on_pages_complete =
        [this](const std::vector<uint32_t>& changed_pages) {
          OnPagesComplete(changed_pages);
        };

    shared_memory_arbiter_ = perfetto::SharedMemoryArbiter::CreateInstance(
        shared_memory_.get(), 4096 /* TODO where does this come from? */,
        on_pages_complete, &perfetto_task_runner_);

    OnEnabled();
  }

  void DisableDataSource() override {
    LOG(ERROR) << "ProducerClientBase::DisableDataSource();";
  }

 protected:
  virtual void ConnectToProducer(
      perfetto::mojom::ProducerClientPtr producer_client,
      perfetto::mojom::ProducerServicePtr producer_service) = 0;
  virtual void OnEnabled() = 0;

  std::unique_ptr<mojo::Binding<perfetto::mojom::ProducerClient>> binding_;
  std::unique_ptr<perfetto::SharedMemoryArbiter> shared_memory_arbiter_;

 private:
  void OnPagesComplete(const std::vector<uint32_t>& changed_pages) {
    producer_->NotifySharedMemoryUpdate(changed_pages);
  }

  PerfettoTaskRunner perfetto_task_runner_;
  perfetto::mojom::ProducerPtr producer_;
  std::unique_ptr<MojoSharedMemory> shared_memory_;
};

class ChromeTracingProducerClient : public ProducerClientBase {
 public:
  ~ChromeTracingProducerClient() override = default;

 protected:
  void ConnectToProducer(
      perfetto::mojom::ProducerClientPtr producer_client,
      perfetto::mojom::ProducerServicePtr producer_service) override {
    producer_service->ConnectToChromeTracingProducer(
        std::move(producer_client));
  }

  void OnEnabled() override {
    std::unique_ptr<perfetto::TraceWriter> writer =
        shared_memory_arbiter_->CreateTraceWriter(1 /* target_buffer */);
    ASSERT_TRUE(writer);

    const size_t kNumPackets = 5;
    for (size_t i = 0; i < kNumPackets; i++) {
      char buf[8];
      sprintf(buf, "evt_%zu", i);
      writer->NewTracePacket()->set_test_event()->set_str(buf, strlen(buf));
    }
  }
};

class ProducerBase : public perfetto::mojom::Producer,
                     public perfetto::Producer {
 public:
  ~ProducerBase() override = default;

  void Initialize(perfetto::mojom::ProducerClientPtr producer_client, perfetto::Service* service) {
    producer_client_ = std::move(producer_client);

    perfetto::mojom::ProducerPtr producer;
    binding_ = std::make_unique<mojo::Binding<perfetto::mojom::Producer>>(this, mojo::MakeRequest(&producer));
    //binding_->set_connection_error_handler();
    producer_client->ConnectProducer(std::move(producer));

    producer_endpoint_ = service->ConnectProducer(this);
  }

  virtual void InitializeDataSource() = 0;

  // perfetto::Producer implementation
  void OnConnect() override { InitializeDataSource(); }

  void OnDisconnect() override {
    LOG(ERROR) << "ProducerBase::OnDisconnect();";
  }

  void CreateDataSourceInstance(
      perfetto::DataSourceInstanceID id,
      const perfetto::DataSourceConfig& config) override {
    CHECK_EQ(config.name(), kPerfettoDataSourceName);
    LOG(ERROR) << "ProducerBase::CreateDataSourceInstance(), target buffer: "
               << config.target_buffer();

    MojoSharedMemory* shared_memory =
        static_cast<MojoSharedMemory*>(producer_endpoint_->shared_memory());
    DCHECK(shared_memory);
    DCHECK(producer_client_);
    mojo::ScopedSharedBufferHandle shm = shared_memory->Clone();
    DCHECK(shm.is_valid());
    producer_client_->EnableDataSource(std::move(shm), shared_memory->size());

  }

  void TearDownDataSourceInstance(perfetto::DataSourceInstanceID) override {
    DCHECK(producer_client_);
    producer_client_->DisableDataSource();
  }

  // perfetto::mojom::Producer implementation
  void NotifySharedMemoryUpdate(
      const std::vector<uint32_t>& changed_pages) override {
    DCHECK(producer_endpoint_);

    LOG(ERROR) << "ProducerBase::NotifySharedMemoryUpdate();";
    for (auto page : changed_pages)
      LOG(ERROR) << "\tPage complete: " << page;

    producer_endpoint_->NotifySharedMemoryUpdate(changed_pages);
  }

 protected:
  std::unique_ptr<perfetto::Service::ProducerEndpoint> producer_endpoint_;

 private:
  perfetto::mojom::ProducerClientPtr producer_client_;
  std::unique_ptr<mojo::Binding<perfetto::mojom::Producer>> binding_;
};

class ChromeTracingProducer : public ProducerBase {
 public:
  ~ChromeTracingProducer() override = default;

  void InitializeDataSource() override {
    perfetto::DataSourceDescriptor descriptor;
    descriptor.set_name(kPerfettoDataSourceName);
    producer_endpoint_->RegisterDataSource(
        descriptor, [](perfetto::DataSourceID id) {
          LOG(ERROR) << "Data source registered, new ID: " << id;
        });
  }
};

class Consumer : public perfetto::Consumer {
 public:
  Consumer(perfetto::Service* service) {
    consumer_endpoint_ = service->ConnectConsumer(this);
  }

  void ReadBuffers() { consumer_endpoint_->ReadBuffers(); }

  void DisableTracing() { consumer_endpoint_->DisableTracing(); }

  void OnConnect() override {
    LOG(ERROR) << "Consumer::OnConnect();";

    // Start tracing.
    perfetto::TraceConfig trace_config;
    trace_config.add_buffers()->set_size_kb(4096 * 10);
    auto* ds_config = trace_config.add_data_sources()->mutable_config();
    ds_config->set_name(kPerfettoDataSourceName);
    ds_config->set_target_buffer(0);
    //ds_config->set_trace_category_filters("foo,bar");

    consumer_endpoint_->EnableTracing(trace_config);
  }

  void OnDisconnect() override { LOG(ERROR) << "Consumer::OnDisconnect();"; }

  void OnTraceData(std::vector<perfetto::TracePacket> packets,
                   bool has_more) override {
    for (auto& packet : packets) {
      packet.Decode();
      LOG(ERROR) << "Consumer::OnTraceData: " << packet->test_event().str();
    }
  }

 private:
  std::unique_ptr<perfetto::Service::ConsumerEndpoint> consumer_endpoint_;
};

class PerfettoService : public perfetto::mojom::ProducerService {
 public:
  PerfettoService() {
    service_ = perfetto::Service::CreateInstance(
        std::make_unique<MojoSharedMemory::Factory>(), &perfetto_task_runner_);
    DCHECK(service_);

    //chrome_tracing_producer_ = std::make_unique<ChromeTracingProducer>();
    //chrome_tracing_producer_->Initialize(service_.get());
  }

  void ConnectToChromeTracingProducer(
      perfetto::mojom::ProducerClientPtr producer_client) override {
    auto new_producer = std::make_unique<ChromeTracingProducer>();
    new_producer->Initialize(std::move(producer_client), service_.get());
    chrome_tracing_producers_.insert(std::move(new_producer));
  }

  perfetto::Service* service() const { return service_.get(); }

  PerfettoTaskRunner perfetto_task_runner_;
  std::unique_ptr<perfetto::Service> service_;
  std::set<std::unique_ptr<ChromeTracingProducer>> chrome_tracing_producers_;
};

class PerfettoTest : public testing::Test {
 public:
  PerfettoTest() : ui_thread_(content::BrowserThread::UI, &message_loop_) {}

 private:
  base::MessageLoop message_loop_;
  content::TestBrowserThread ui_thread_;
};

TEST_F(PerfettoTest, Testing) {
  PerfettoService service;
  mojo::BindingSet<perfetto::mojom::ProducerService> service_bindings_;

  // Child process 1
  ChromeTracingProducerClient producer_client_1;
  {
    perfetto::mojom::ProducerServicePtr producer_service_ptr_1;
    service_bindings_.AddBinding(&service,
                                 mojo::MakeRequest(&producer_service_ptr_1));

    producer_client_1.Initialize(std::move(producer_service_ptr_1));
  }

  // Child process 2
  ChromeTracingProducerClient producer_client_2;
  {
    perfetto::mojom::ProducerServicePtr producer_service_ptr_2;
    service_bindings_.AddBinding(&service,
                                 mojo::MakeRequest(&producer_service_ptr_2));

    producer_client_2.Initialize(std::move(producer_service_ptr_2));
  }

  base::RunLoop().RunUntilIdle();

  Consumer consumer(service.service());
  base::RunLoop().RunUntilIdle();

  consumer.DisableTracing();
  base::RunLoop().RunUntilIdle();

  consumer.ReadBuffers();
  base::RunLoop().RunUntilIdle();
}
