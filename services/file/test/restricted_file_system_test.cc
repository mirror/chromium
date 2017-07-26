// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/files/scoped_temp_dir.h"
#include "base/test/mock_callback.h"
#include "services/file/file_service.h"
#include "services/file/public/interfaces/constants.mojom.h"
#include "services/file/public/interfaces/restricted_file_system.mojom.h"
#include "services/file/user_id_map.h"
#include "services/service_manager/public/cpp/service_context.h"
#include "services/service_manager/public/cpp/service_test.h"
#include "services/service_manager/public/interfaces/constants.mojom.h"
#include "services/service_manager/public/interfaces/service_manager.mojom.h"
#include "services/service_manager/public/interfaces/service_factory.mojom.h"
#include "testing/gmock/include/gmock/gmock.h"

using testing::_;
using testing::Invoke;

namespace {

class ServiceTestClient : public service_manager::test::ServiceTestClient,
                          public service_manager::mojom::ServiceFactory {
 public:
  explicit ServiceTestClient(service_manager::test::ServiceTest* test,
    base::OnceClosure service_created_cb)
      : service_manager::test::ServiceTestClient(test),
        service_created_cb_(std::move(service_created_cb)) {
    registry_.AddInterface<service_manager::mojom::ServiceFactory>(base::Bind(
        &ServiceTestClient::BindServiceFactoryRequest, base::Unretained(this)));
  }
  ~ServiceTestClient() override {}

 protected:
  void OnBindInterface(const service_manager::BindSourceInfo& source_info,
                       const std::string& interface_name,
                       mojo::ScopedMessagePipeHandle interface_pipe) override {
    LOG(ERROR) << "Binding to interface " << interface_name;
    registry_.BindInterface(interface_name, std::move(interface_pipe));
  }

  void CreateService(service_manager::mojom::ServiceRequest request,
                     const std::string& name) override {
    if (name == file::mojom::kServiceName) {
      LOG(ERROR) << "Creating FileService";
      file_service_context_.reset(new service_manager::ServiceContext(
          file::CreateFileService(), std::move(request)));
      base::ResetAndReturn(&service_created_cb_).Run();
    }
  }

  void BindServiceFactoryRequest(
      service_manager::mojom::ServiceFactoryRequest request) {
    service_factory_bindings_.AddBinding(this, std::move(request));
  }

 private:
  service_manager::BinderRegistry registry_;
  mojo::BindingSet<service_manager::mojom::ServiceFactory>
      service_factory_bindings_;
  std::unique_ptr<service_manager::ServiceContext> file_service_context_;
  base::OnceClosure service_created_cb_;
};

}  // namespace


namespace file {

// Sets up a connection to a restricted file system instance.
class RestrictedFileSystemTest : public service_manager::test::ServiceTest {
 public:
  RestrictedFileSystemTest()
      : service_manager::test::ServiceTest("file_unittests") {}
  ~RestrictedFileSystemTest() override = default;

  void SetUp() override {
    ServiceTest::SetUp();
    ASSERT_TRUE(temp_path_.CreateUniqueTempDir());
    file::AssociateServiceUserIdWithUserDir(test_userid(),
                                            temp_path_.GetPath());
    connector()->BindInterface(mojom::kServiceName, &file_system_);
    service_created_.Run();
  }

  void TearDown() override {
    service_manager::ServiceContext::ClearGlobalBindersForTesting(
        file::mojom::kServiceName);
    ServiceTest::TearDown();
  }

  std::unique_ptr<service_manager::Service> CreateService() override {
    LOG(ERROR) << "Creating ServiceTestClient";
    return base::MakeUnique<ServiceTestClient>(
        this,
        base::BindOnce(
            [](base::RunLoop* service_created) { service_created->Quit(); },
            &service_created_));
  }

 protected:
  mojom::RestrictedFileSystemPtr file_system_;
  base::MockCallback<mojom::RestrictedFileSystem::CreateTempFileCallback>
      create_temp_file_callback_;
  base::RunLoop service_created_;

 private:
  base::ScopedTempDir temp_path_;
};

TEST_F(RestrictedFileSystemTest, CreateAndReleaseTempFile) {
  base::RunLoop wait_loop;
  EXPECT_CALL(create_temp_file_callback_, Run(_)).WillOnce(Invoke([&wait_loop](
    filesystem::mojom::FileError result_code){
      EXPECT_EQ(filesystem::mojom::FileError::OK, result_code);
      wait_loop.Quit();
  }));

  filesystem::mojom::FilePtr file;
  file_system_->CreateTempFile(mojo::MakeRequest(&file),
                               create_temp_file_callback_.Get());
  wait_loop.Run();
}

TEST_F(RestrictedFileSystemTest, WriteAndReadBackTempFile) {
  base::RunLoop wait_loop;
  EXPECT_CALL(create_temp_file_callback_, Run(_)).WillOnce(Invoke([&wait_loop](
    filesystem::mojom::FileError result_code){
      LOG(INFO) << "Callback arrived";
      EXPECT_EQ(filesystem::mojom::FileError::OK, result_code);
      wait_loop.Quit();
  }));

  filesystem::mojom::FilePtr file;
  file_system_->CreateTempFile(mojo::MakeRequest(&file),
                               create_temp_file_callback_.Get());
  // We must wait for the callback before calling any (synchronous) method of
  // |file|, because such a call will not return before the callback has been
  // received, leading to a deadlock.
  wait_loop.Run();

  // Initialize a buffer with a test pattern
  static const int kTestByteCount = 10;
  std::vector<uint8_t> bytes_to_write;
  for (int i = 0; i < kTestByteCount; i++) {
    bytes_to_write.push_back(i);
  }
  filesystem::mojom::FileError result_code;
  uint32_t num_bytes_written = 0;
  LOG(INFO) << "Waiting";
  ASSERT_TRUE(file->Write(bytes_to_write, 0 /*offset*/,
                          filesystem::mojom::Whence::FROM_BEGIN, &result_code,
                          &num_bytes_written));
}

}  // namespace file
