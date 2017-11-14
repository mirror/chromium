// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/renderer_host/clipboard_host_impl.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "base/memory/ref_counted.h"
#include "base/process/process_handle.h"
#include "base/run_loop.h"
#include "content/browser/blob_storage/chrome_blob_storage_context.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "mojo/public/cpp/system/platform_handle.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "ui/base/test/test_clipboard.h"
#include "ui/gfx/geometry/size.h"

namespace content {

class ClipboardHostImplTest : public ::testing::Test {
 protected:
  ClipboardHostImplTest()
      : filter_(new ClipboardHostImpl(nullptr)),
        clipboard_(ui::TestClipboard::CreateForCurrentThread()) {}

  ~ClipboardHostImplTest() override {
    ui::Clipboard::DestroyClipboardForCurrentThread();
  }

  std::unique_ptr<base::SharedMemory> CreateAndMapReadOnlySharedMemory(
      size_t size) {
    std::unique_ptr<base::SharedMemory> m = CreateReadOnlySharedMemory(size);
    if (!m->Map(size))
      return nullptr;
    return m;
  }

  std::unique_ptr<base::SharedMemory> CreateReadOnlySharedMemory(size_t size) {
    std::unique_ptr<base::SharedMemory> m(new base::SharedMemory());
    base::SharedMemoryCreateOptions options;
    options.size = size;
    options.share_read_only = true;
    if (!m->Create(options))
      return nullptr;
    return m;
  }

  void CallWriteImage(const gfx::Size& size,
                      base::SharedMemory* shared_memory,
                      size_t shared_memory_size) {
    base::SharedMemoryHandle handle = shared_memory->GetReadOnlyHandle();
    shared_memory->Unmap();
    shared_memory->Close();
    ASSERT_TRUE(handle.IsValid());
    CallWriteImageDirectly(size, handle, shared_memory_size);
  }

  // Prefer to use CallWriteImage() in tests.
  void CallWriteImageDirectly(const gfx::Size& size,
                              base::SharedMemoryHandle handle,
                              size_t shared_memory_size) {
    mojo::ScopedSharedBufferHandle shared_buffer_handle =
        mojo::WrapSharedMemoryHandle(handle, shared_memory_size, false);
    filter_->WriteImage(ui::CLIPBOARD_TYPE_COPY_PASTE, size,
                        std::move(shared_buffer_handle));
  }

  void CallCommitWrite() {
    filter_->CommitWrite(ui::CLIPBOARD_TYPE_COPY_PASTE);
    base::RunLoop().RunUntilIdle();
  }

  ui::Clipboard* clipboard() { return clipboard_; }

 private:
  const TestBrowserThreadBundle thread_bundle_;
  const std::unique_ptr<ClipboardHostImpl> filter_;
  ui::Clipboard* const clipboard_;
};

// Test that it actually works.
TEST_F(ClipboardHostImplTest, SimpleImage) {
  static const uint32_t bitmap_data[] = {
      0x33333333, 0xdddddddd, 0xeeeeeeee, 0x00000000, 0x88888888, 0x66666666,
      0x55555555, 0xbbbbbbbb, 0x44444444, 0xaaaaaaaa, 0x99999999, 0x77777777,
      0xffffffff, 0x11111111, 0x22222222, 0xcccccccc,
  };

  std::unique_ptr<base::SharedMemory> shared_memory =
      CreateAndMapReadOnlySharedMemory(sizeof(bitmap_data));
  memcpy(shared_memory->memory(), bitmap_data, sizeof(bitmap_data));

  CallWriteImage(gfx::Size(4, 4), shared_memory.get(), sizeof(bitmap_data));
  uint64_t sequence_number =
      clipboard()->GetSequenceNumber(ui::CLIPBOARD_TYPE_COPY_PASTE);
  CallCommitWrite();

  EXPECT_NE(sequence_number,
            clipboard()->GetSequenceNumber(ui::CLIPBOARD_TYPE_COPY_PASTE));
  EXPECT_FALSE(clipboard()->IsFormatAvailable(
      ui::Clipboard::GetPlainTextFormatType(), ui::CLIPBOARD_TYPE_COPY_PASTE));
  EXPECT_TRUE(clipboard()->IsFormatAvailable(
      ui::Clipboard::GetBitmapFormatType(), ui::CLIPBOARD_TYPE_COPY_PASTE));

  SkBitmap actual = clipboard()->ReadImage(ui::CLIPBOARD_TYPE_COPY_PASTE);
  EXPECT_EQ(sizeof(bitmap_data), actual.computeByteSize());
  EXPECT_EQ(0,
            memcmp(bitmap_data, actual.getAddr32(0, 0), sizeof(bitmap_data)));
}

// Test with a size that would overflow a naive 32-bit row bytes calculation.
TEST_F(ClipboardHostImplTest, ImageSizeOverflows32BitRowBytes) {
  std::unique_ptr<base::SharedMemory> shared_memory =
      CreateReadOnlySharedMemory(0x20000000);

  CallWriteImage(gfx::Size(0x20000000, 1), shared_memory.get(), 0x20000000);
  uint64_t sequence_number =
      clipboard()->GetSequenceNumber(ui::CLIPBOARD_TYPE_COPY_PASTE);
  CallCommitWrite();

  EXPECT_EQ(sequence_number,
            clipboard()->GetSequenceNumber(ui::CLIPBOARD_TYPE_COPY_PASTE));
}

TEST_F(ClipboardHostImplTest, InvalidSharedMemoryHandle) {
  CallWriteImageDirectly(gfx::Size(5, 5), base::SharedMemoryHandle(), 0);
  uint64_t sequence_number =
      clipboard()->GetSequenceNumber(ui::CLIPBOARD_TYPE_COPY_PASTE);
  CallCommitWrite();

  EXPECT_EQ(sequence_number,
            clipboard()->GetSequenceNumber(ui::CLIPBOARD_TYPE_COPY_PASTE));
}

}  // namespace content
