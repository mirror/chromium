// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/gpu/ipc/service/gpu_jpeg_decode_accelerator_mojo.h"

#include "base/bind.h"
#include "base/command_line.h"
#include "base/run_loop.h"
#include "base/test/scoped_task_environment.h"
#include "base/threading/thread.h"
#include "build/build_config.h"
#include "media/base/media_switches.h"
#include "mojo/edk/embedder/embedder.h"
#include "mojo/edk/embedder/scoped_ipc_support.h"
#include "mojo/public/cpp/system/platform_handle.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace media {
namespace {

class MojoEnabledTestEnvironment final : public testing::Environment {
 public:
  MojoEnabledTestEnvironment() : mojo_ipc_thread_("MojoIpcThread") {}

  ~MojoEnabledTestEnvironment() final {}

  void SetUp() final {
    mojo::edk::Init();
    mojo_ipc_thread_.StartWithOptions(
        base::Thread::Options(base::MessageLoop::TYPE_IO, 0));
    mojo_ipc_support_.reset(new mojo::edk::ScopedIPCSupport(
        mojo_ipc_thread_.task_runner(),
        mojo::edk::ScopedIPCSupport::ShutdownPolicy::FAST));
    VLOG(1) << "Mojo initialized";
  }

  void TearDown() final {
    mojo_ipc_support_.reset();
    VLOG(1) << "Mojo IPC tear down";
  }

 private:
  base::Thread mojo_ipc_thread_;
  std::unique_ptr<mojo::edk::ScopedIPCSupport> mojo_ipc_support_;
};

testing::Environment* const mojo_test_env =
    testing::AddGlobalTestEnvironment(new MojoEnabledTestEnvironment());

}  // namespace

static const int32_t kArbitraryBitstreamBufferId = 123;

// Test fixture for the unit that is created via the mojom interface for
// class GpuJpegDecodeAccelerator. Uses a FakeJpegDecodeAccelerator to
// simulate the actual decoding without the need for special hardware.
class GpuJpegDecodeAcceleratorMojoTest : public ::testing::Test {
 public:
  GpuJpegDecodeAcceleratorMojoTest() {}
  ~GpuJpegDecodeAcceleratorMojoTest() override {}

  void SetUp() override {
    base::CommandLine::ForCurrentProcess()->AppendSwitch(
        switches::kUseFakeJpegDecodeAccelerator);
  }

  void OnInitializeDone(const base::Closure& continuation, bool success) {
    VLOG(1) << "OnInitializeDone success " << success;
    EXPECT_TRUE(success);
    continuation.Run();
  }

  void OnDecodeAck(const base::Closure& continuation,
                   int32_t bitstream_buffer_id,
                   JpegDecodeAccelerator::Error error) {
    VLOG(1) << "OnDecodeAck id " << bitstream_buffer_id << " Error " << error;
    EXPECT_EQ(kArbitraryBitstreamBufferId, bitstream_buffer_id);
    continuation.Run();
  }

 private:
  // This is required to allow base::ThreadTaskRunnerHandle::Get() from the
  // test execution thread.
  base::test::ScopedTaskEnvironment scoped_task_environment_;
};

TEST_F(GpuJpegDecodeAcceleratorMojoTest, InitializeAndDecode) {
  mojom::GpuJpegDecodeAcceleratorPtr jpeg_decoder_;
  GpuJpegDecodeAcceleratorMojo::Create(mojo::MakeRequest(&jpeg_decoder_));
  base::RunLoop run_loop;
  jpeg_decoder_->Initialize(
      base::Bind(&GpuJpegDecodeAcceleratorMojoTest::OnInitializeDone,
                 base::Unretained(this), run_loop.QuitClosure()));
  run_loop.Run();

  const size_t kInputBufferSizeInBytes = 512;
  const size_t kOutputFrameSizeInBytes = 1024;
  const gfx::Size kDummyFrameCodedSize(10, 10);
  const char kKeyId[] = "key id";
  const char kIv[] = "0123456789abcdef";
  std::vector<SubsampleEntry> subsamples;
  subsamples.push_back(SubsampleEntry(10, 5));
  subsamples.push_back(SubsampleEntry(15, 7));

  base::RunLoop run_loop2;
  base::SharedMemory shm;
  ASSERT_TRUE(shm.CreateAndMapAnonymous(kInputBufferSizeInBytes));

  mojo::ScopedSharedBufferHandle output_frame_handle =
      mojo::SharedBufferHandle::Create(kOutputFrameSizeInBytes);

  BitstreamBuffer bitstream_buffer(
      kArbitraryBitstreamBufferId,
      base::SharedMemory::DuplicateHandle(shm.handle()),
      kInputBufferSizeInBytes);
  bitstream_buffer.SetDecryptConfig(DecryptConfig(kKeyId, kIv, subsamples));

  jpeg_decoder_->Decode(
      bitstream_buffer, kDummyFrameCodedSize, std::move(output_frame_handle),
      base::checked_cast<uint32_t>(kOutputFrameSizeInBytes),
      base::Bind(&GpuJpegDecodeAcceleratorMojoTest::OnDecodeAck,
                 base::Unretained(this), run_loop2.QuitClosure()));
  run_loop2.Run();
}

}  // namespace media
