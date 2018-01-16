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

#undef OS_ANDROID

#ifdef BUILDFLAG_CAT_INDIRECT
#undef BUILDFLAG_CAT_INDIRECT
#endif

#ifdef BUILDFLAG
#undef BUILDFLAG
#endif

#include "third_party/perfetto/include/perfetto/base/task_runner.h"
#include "third_party/perfetto/include/perfetto/tracing/core/consumer.h"
#include "third_party/perfetto/include/perfetto/tracing/core/data_source_descriptor.h"
#include "third_party/perfetto/include/perfetto/tracing/core/producer.h"
#include "third_party/perfetto/include/perfetto/tracing/core/service.h"
#include "third_party/perfetto/include/perfetto/tracing/core/shared_memory.h"
#include "third_party/perfetto/include/perfetto/tracing/core/shared_memory_arbiter.h"
#include "third_party/perfetto/include/perfetto/tracing/core/trace_config.h"
#include "third_party/perfetto/include/perfetto/tracing/core/trace_writer.h"

//#include "third_party/perfetto/protos/trace_packet.pbzero.h"
#include "protos_zero/protos/test_event.pbzero.h"
#include "protos_zero/protos/trace_packet.pbzero.h"

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

  void PostDelayedTask(std::function<void()>, int delay_ms) override {
    // base::SequencedTaskRunnerHandle::Get()->PostDelayedTask(
    //      FROM_HERE, base::Bind(&RecordSqliteMemory10Min),
    //      base::TimeDelta::FromMinutes(10));
    NOTREACHED();
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

class Producer : public perfetto::mojom::Producer {
 public:
  Producer(perfetto::mojom::ProducerServicePtr producer_service_client,
           PerfettoTaskRunner* task_runner)
      : producer_service_client_(std::move(producer_service_client)),
        task_runner_(task_runner) {
    perfetto::mojom::ProducerPtr producer;
    binding_ = std::make_unique<mojo::Binding<perfetto::mojom::Producer>>(
        this, mojo::MakeRequest(&producer));
    producer_service_client_->ConnectProducer(std::move(producer));
  }

  void OnConnect() override {
    LOG(ERROR) << "Producer::OnConnect();";

    perfetto::mojom::DataSourceDescriptorPtr data_source_descriptor(
        perfetto::mojom::DataSourceDescriptor::New());
    data_source_descriptor->name = kPerfettoDataSourceName;
    producer_service_client_->RegisterDataSource(
        std::move(data_source_descriptor));
  }

  void SetTraceBuffer(mojo::ScopedSharedBufferHandle shared_memory,
                      uint64_t size) override {
    LOG(ERROR) << "Producer::SetTraceBuffer();";
    shared_memory_ =
        std::make_unique<MojoSharedMemory>(std::move(shared_memory), size);

    auto on_pages_complete =
        [this](const std::vector<uint32_t>& changed_pages) {
          OnPagesComplete(changed_pages);
        };

    shared_memory_arbiter_ = perfetto::SharedMemoryArbiter::CreateInstance(
        shared_memory_.get(), 4096 /* TODO where does this come from? */,
        on_pages_complete, task_runner_);
  }

  void OnDisconnect() override { LOG(ERROR) << "Producer::OnDisconnect();"; }

  void CreateDataSourceInstance(
      int64_t id,
      perfetto::mojom::DataSourceConfigPtr config) override {
    CHECK_EQ(config->name, kPerfettoDataSourceName);
    LOG(ERROR) << "Producer::CreateDataSourceInstance(), target buffer: "
               << config->target_buffer;

    DCHECK(shared_memory_arbiter_);

    std::unique_ptr<perfetto::TraceWriter> writer =
        shared_memory_arbiter_->CreateTraceWriter(1 /* target_buffer */);
    ASSERT_TRUE(writer);

    const size_t kNumPackets = 10;
    for (size_t i = 0; i < kNumPackets; i++) {
      char buf[8];
      sprintf(buf, "evt_%zu", i);
      writer->NewTracePacket()->set_test_event()->set_str(buf, strlen(buf));
      base::RunLoop().RunUntilIdle();
    }
  }

  void OnPagesComplete(const std::vector<uint32_t>& changed_pages) {
    producer_service_client_->NotifySharedMemoryUpdate(changed_pages);
  }

 private:
  std::unique_ptr<mojo::Binding<perfetto::mojom::Producer>> binding_;
  perfetto::mojom::ProducerServicePtr producer_service_client_;
  std::unique_ptr<MojoSharedMemory> shared_memory_;
  std::unique_ptr<perfetto::SharedMemoryArbiter> shared_memory_arbiter_;
  PerfettoTaskRunner* task_runner_;
};

class RemoteProducer : public perfetto::Producer {
 public:
  RemoteProducer(perfetto::mojom::ProducerPtr producer)
      : producer_ptr_(std::move(producer)) {}

  void SetTraceBuffer(mojo::ScopedSharedBufferHandle shared_memory,
                      uint64_t size) {
    producer_ptr_->SetTraceBuffer(std::move(shared_memory), size);
  }

  void OnConnect() override { producer_ptr_->OnConnect(); }

  void OnDisconnect() override { producer_ptr_->OnDisconnect(); }

  void CreateDataSourceInstance(
      perfetto::DataSourceInstanceID id,
      const perfetto::DataSourceConfig& config) override {
    perfetto::mojom::DataSourceConfigPtr data_source_config(
        perfetto::mojom::DataSourceConfig::New());

    data_source_config->name = config.name();
    data_source_config->target_buffer = config.target_buffer();
    data_source_config->trace_category_filters =
        config.trace_category_filters();

    producer_ptr_->CreateDataSourceInstance(id, std::move(data_source_config));
  }

  void TearDownDataSourceInstance(perfetto::DataSourceInstanceID) override {
    NOTREACHED();
  }

 private:
  perfetto::mojom::ProducerPtr producer_ptr_;
};

class ProducerService : public perfetto::mojom::ProducerService {
 public:
  ProducerService(perfetto::mojom::ProducerServiceRequest request,
                  perfetto::Service* service)
      : binding_(this, std::move(request)), service_(service) {}

  void ConnectProducer(perfetto::mojom::ProducerPtr producer) override {
    remote_producer_ = std::make_unique<RemoteProducer>(std::move(producer));
    producer_endpoint_ = service_->ConnectProducer(remote_producer_.get());

    MojoSharedMemory* shared_memory =
        static_cast<MojoSharedMemory*>(producer_endpoint_->shared_memory());
    DCHECK(shared_memory);
    mojo::ScopedSharedBufferHandle shm = shared_memory->Clone();
    DCHECK(shm.is_valid());
    // TODO: Ideally we'd do this as part of the OnConnect().
    remote_producer_->SetTraceBuffer(std::move(shm), shared_memory->size());
  };

  void RegisterDataSource(
      perfetto::mojom::DataSourceDescriptorPtr mojo_descriptor) override {
    perfetto::DataSourceDescriptor descriptor;
    descriptor.set_name(mojo_descriptor->name);
    producer_endpoint_->RegisterDataSource(
        descriptor, [](perfetto::DataSourceID id) {
          LOG(ERROR) << "Data source registered, new ID: " << id;
        });
  }

  void NotifySharedMemoryUpdate(
      const std::vector<uint32_t>& changed_pages) override {
    LOG(ERROR) << "PAGES COMPLETE!!!:";
    for (auto page : changed_pages)
      LOG(ERROR) << "Page complete: " << page;

    producer_endpoint_->NotifySharedMemoryUpdate(changed_pages);
  }

  std::unique_ptr<perfetto::Service::ProducerEndpoint> producer_endpoint_;

 private:
  mojo::Binding<perfetto::mojom::ProducerService> binding_;
  std::unique_ptr<RemoteProducer> remote_producer_;
  perfetto::Service* service_;
};

class Consumer : public perfetto::Consumer {
 public:
  Consumer(perfetto::Service* service) {
    consumer_endpoint_ = service->ConnectConsumer(this);
  }

  void OnConnect() override {
    LOG(ERROR) << "Consumer::OnConnect();";

    // Start tracing.
    perfetto::TraceConfig trace_config;
    trace_config.add_buffers()->set_size_kb(4096 * 10);
    auto* ds_config = trace_config.add_data_sources()->mutable_config();
    ds_config->set_name(kPerfettoDataSourceName);
    ds_config->set_target_buffer(0);
    ds_config->set_trace_category_filters("foo,bar");

    consumer_endpoint_->EnableTracing(trace_config);
  }

  void OnDisconnect() override { LOG(ERROR) << "Consumer::OnDisconnect();"; }

  void OnTraceData(std::vector<perfetto::TracePacket>, bool has_more) override {
    NOTREACHED();
  }

 private:
  std::unique_ptr<perfetto::Service::ConsumerEndpoint> consumer_endpoint_;
};

namespace content {

class PerfettoTest : public testing::Test {
 public:
  PerfettoTest() : ui_thread_(BrowserThread::UI, &message_loop_) {
    service_ = perfetto::Service::CreateInstance(
        std::make_unique<MojoSharedMemory::Factory>(), &perfetto_task_runner_);
    EXPECT_TRUE(service_);
  }

  void BindToProducerService(perfetto::mojom::ProducerServiceRequest request) {
    producer_service_ =
        std::make_unique<ProducerService>(std::move(request), service_.get());
    // impl.set_connection_error_handler(loop.QuitClosure());
  }

 protected:
  base::MessageLoop message_loop_;
  TestBrowserThread ui_thread_;
  PerfettoTaskRunner perfetto_task_runner_;
  std::unique_ptr<Producer> producer_;
  std::unique_ptr<perfetto::Service> service_;
  std::unique_ptr<ProducerService> producer_service_;
};

TEST_F(PerfettoTest, Testing) {
  perfetto::mojom::ProducerServicePtr producer_service_client;
  BindToProducerService(mojo::MakeRequest(&producer_service_client));
  producer_ = std::make_unique<Producer>(std::move(producer_service_client),
                                         &perfetto_task_runner_);
  Consumer consumer(service_.get());

  base::RunLoop().RunUntilIdle();
  producer_service_->producer_endpoint_.reset();
}

}  // namespace content
