// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <memory>

#include "base/callback.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/memory/shared_memory.h"
#include "base/path_service.h"
#include "base/run_loop.h"
#include "base/test/test_discardable_memory_allocator.h"
#include "components/printing/service/pdf_compositor_service.h"
#include "components/printing/service/public/interfaces/pdf_compositor.mojom.h"
#include "mojo/public/cpp/bindings/binding_set.h"
#include "mojo/public/cpp/system/platform_handle.h"
#include "services/service_manager/public/cpp/binder_registry.h"
#include "services/service_manager/public/cpp/service_context.h"
#include "services/service_manager/public/cpp/service_test.h"
#include "services/service_manager/public/interfaces/service_factory.mojom.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace printing {

// In order to test PdfCompositorService, this class overrides PrepareToStart()
// to do nothing. So the test discardable memory allocator set up by
// PdfCompositorServiceTest will be used.
class PdfCompositorTestService : public printing::PdfCompositorService {
 public:
  explicit PdfCompositorTestService(const std::string& creator)
      : PdfCompositorService(creator) {}
  ~PdfCompositorTestService() override {}

  // PdfCompositorService:
  void PrepareToStart() override {}
};

class PdfServiceTestClient : public service_manager::test::ServiceTestClient,
                             public service_manager::mojom::ServiceFactory {
 public:
  explicit PdfServiceTestClient(service_manager::test::ServiceTest* test)
      : service_manager::test::ServiceTestClient(test) {
    registry_.AddInterface<service_manager::mojom::ServiceFactory>(
        base::Bind(&PdfServiceTestClient::Create, base::Unretained(this)));
  }
  ~PdfServiceTestClient() override {}

  // service_manager::Service
  void OnBindInterface(const service_manager::BindSourceInfo& source_info,
                       const std::string& interface_name,
                       mojo::ScopedMessagePipeHandle interface_pipe) override {
    registry_.BindInterface(interface_name, std::move(interface_pipe));
  }

  // service_manager::mojom::ServiceFactory
  void CreateService(service_manager::mojom::ServiceRequest request,
                     const std::string& name) override {
    if (!name.compare(mojom::kServiceName)) {
      service_context_ = std::make_unique<service_manager::ServiceContext>(
          std::make_unique<PdfCompositorTestService>("pdf_compositor_unittest"),
          std::move(request));
    }
  }

  void Create(service_manager::mojom::ServiceFactoryRequest request) {
    service_factory_bindings_.AddBinding(this, std::move(request));
  }

 private:
  service_manager::BinderRegistry registry_;
  mojo::BindingSet<service_manager::mojom::ServiceFactory>
      service_factory_bindings_;
  std::unique_ptr<service_manager::ServiceContext> service_context_;
};

class PdfCompositorServiceTest : public service_manager::test::ServiceTest {
 public:
  PdfCompositorServiceTest() : ServiceTest("pdf_compositor_service_unittest") {}
  ~PdfCompositorServiceTest() override {}

  MOCK_METHOD1(CallbackOnCompositeSuccess, void(mojo::SharedBufferHandle));
  MOCK_METHOD1(CallbackOnCompositeStatus, void(mojom::PdfCompositor::Status));
  MOCK_METHOD1(CallbackOnIsReadyStatus, void(bool));
  void OnCompositeToPdfCallback(mojom::PdfCompositor::Status status,
                                mojo::ScopedSharedBufferHandle handle) {
    if (status == mojom::PdfCompositor::Status::SUCCESS)
      CallbackOnCompositeSuccess(handle.get());
    else
      CallbackOnCompositeStatus(status);
    run_loop_->Quit();
  }
  void OnIsReadyToCompositeCallback(bool status) {
    CallbackOnIsReadyStatus(status);
    run_loop_->Quit();
  }

  MOCK_METHOD0(ConnectionClosed, void());

 protected:
  // service_manager::test::ServiceTest:
  void SetUp() override {
    base::DiscardableMemoryAllocator::SetInstance(
        &discardable_memory_allocator_);
    ServiceTest::SetUp();

    ASSERT_FALSE(compositor_);
    connector()->BindInterface(mojom::kServiceName, &compositor_);
    ASSERT_TRUE(compositor_);

    run_loop_ = std::make_unique<base::RunLoop>();
  }

  void TearDown() override {
    // Clean up
    compositor_.reset();
    base::DiscardableMemoryAllocator::SetInstance(nullptr);
  }

  std::unique_ptr<service_manager::Service> CreateService() override {
    return std::make_unique<PdfServiceTestClient>(this);
  }

  base::SharedMemoryHandle LoadFileInSharedMemory() {
    base::FilePath path;
    PathService::Get(base::DIR_SOURCE_ROOT, &path);
    base::FilePath test_file =
        path.AppendASCII("components/test/data/printing/google.mskp");
    std::string content;
    base::SharedMemoryHandle invalid_handle;
    if (!base::ReadFileToString(test_file, &content))
      return invalid_handle;
    size_t len = content.size();
    base::SharedMemoryCreateOptions options;
    options.size = len;
    options.share_read_only = true;
    base::SharedMemory shared_memory;
    if (shared_memory.Create(options) && shared_memory.Map(len)) {
      memcpy(shared_memory.memory(), content.data(), len);
      return base::SharedMemory::DuplicateHandle(shared_memory.handle());
    }
    return invalid_handle;
  }

  void CallCompositorWithSuccess(mojom::PdfCompositorPtr ptr) {
    static constexpr int kFrameId = 1234;
    auto handle = LoadFileInSharedMemory();
    ASSERT_TRUE(handle.IsValid());
    mojo::ScopedSharedBufferHandle buffer_handle =
        mojo::WrapSharedMemoryHandle(handle, handle.GetSize(), true);
    ASSERT_TRUE(buffer_handle->is_valid());
    EXPECT_CALL(*this, CallbackOnCompositeSuccess(testing::_)).Times(1);
    ptr->CompositeToPdf(
        kFrameId, 0, std::move(buffer_handle), std::vector<uint32_t>(),
        base::BindOnce(&PdfCompositorServiceTest::OnCompositeToPdfCallback,
                       base::Unretained(this)));
    run_loop_->Run();
  }

  std::unique_ptr<base::RunLoop> run_loop_;
  mojom::PdfCompositorPtr compositor_;
  base::TestDiscardableMemoryAllocator discardable_memory_allocator_;

 private:
  DISALLOW_COPY_AND_ASSIGN(PdfCompositorServiceTest);
};

// Test callback is called on error conditions in service.
TEST_F(PdfCompositorServiceTest, InvokeCallbackOnContentError) {
  EXPECT_CALL(*this, CallbackOnCompositeStatus(
                         mojom::PdfCompositor::Status::CONTENT_FORMAT_ERROR))
      .Times(1);
  compositor_->CompositeToPdf(
      5, 0, mojo::SharedBufferHandle::Create(10), std::vector<uint32_t>(),
      base::BindOnce(&PdfCompositorServiceTest::OnCompositeToPdfCallback,
                     base::Unretained(this)));
  run_loop_->Run();
}

TEST_F(PdfCompositorServiceTest, InvokeCallbackOnSuccess) {
  CallCompositorWithSuccess(std::move(compositor_));
}

TEST_F(PdfCompositorServiceTest, MultipleServiceInstances) {
  // One service can bind multiple interfaces.
  mojom::PdfCompositorPtr another_compositor;
  ASSERT_FALSE(another_compositor);
  connector()->BindInterface(mojom::kServiceName, &another_compositor);
  ASSERT_TRUE(another_compositor);
  ASSERT_NE(compositor_.get(), another_compositor.get());

  // Terminating one interface won't affect another.
  compositor_.reset();
  CallCompositorWithSuccess(std::move(another_compositor));
}

TEST_F(PdfCompositorServiceTest, IndependentServiceInstances) {
  // Create a new connection 2.
  mojom::PdfCompositorPtr compositor2;
  ASSERT_FALSE(compositor2);
  connector()->BindInterface(mojom::kServiceName, &compositor2);
  ASSERT_TRUE(compositor2);

  // Original connection add subframe 1.
  auto handle = LoadFileInSharedMemory();
  ASSERT_TRUE(handle.IsValid());
  mojo::ScopedSharedBufferHandle buffer_handle =
      mojo::WrapSharedMemoryHandle(handle, handle.GetSize(), true);
  compositor_->AddSubframeMap(1u, 2u);
  compositor_->AddSubframeContent(1u, std::move(buffer_handle),
                                  std::vector<uint32_t>());

  // Original connection can use this subframe 1.
  EXPECT_CALL(*this, CallbackOnIsReadyStatus(true)).Times(1);
  std::vector<uint32_t> subframe_content_ids({2u});
  compositor_->IsReadyToComposite(
      4u, 0u, subframe_content_ids,
      base::BindOnce(&PdfCompositorServiceTest::OnIsReadyToCompositeCallback,
                     base::Unretained(this)));
  run_loop_->Run();

  // Connection 2 doesn't know about subframe 2.
  EXPECT_CALL(*this, CallbackOnIsReadyStatus(false)).Times(1);
  compositor2->IsReadyToComposite(
      5u, 0u, subframe_content_ids,
      base::BindOnce(&PdfCompositorServiceTest::OnIsReadyToCompositeCallback,
                     base::Unretained(this)));
  run_loop_ = std::make_unique<base::RunLoop>();
  run_loop_->Run();

  // Add info about subframe 2 to connection 2 so it can use it.
  EXPECT_CALL(*this, CallbackOnIsReadyStatus(true)).Times(1);
  auto handle2 = LoadFileInSharedMemory();
  ASSERT_TRUE(handle2.IsValid());
  mojo::ScopedSharedBufferHandle buffer_handle2 =
      mojo::WrapSharedMemoryHandle(handle2, handle2.GetSize(), true);
  compositor2->AddSubframeMap(3u, 2u);
  compositor2->AddSubframeContent(3u, std::move(buffer_handle2),
                                  std::vector<uint32_t>());
  compositor2->IsReadyToComposite(
      4u, 0u, subframe_content_ids,
      base::BindOnce(&PdfCompositorServiceTest::OnIsReadyToCompositeCallback,
                     base::Unretained(this)));
  run_loop_ = std::make_unique<base::RunLoop>();
  run_loop_->Run();
}

}  // namespace printing
