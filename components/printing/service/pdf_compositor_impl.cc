// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/printing/service/pdf_compositor_impl.h"

#include "base/logging.h"
#include "base/memory/shared_memory_handle.h"
#include "mojo/public/cpp/system/platform_handle.h"
#include "printing/common/pdf_metafile_utils.h"
#include "third_party/skia/include/core/SkCanvas.h"
#include "third_party/skia/include/core/SkDocument.h"
#include "third_party/skia/src/utils/SkMultiPictureDocument.h"

namespace printing {

PdfCompositorImpl::PdfCompositorImpl(
    const std::string& creator,
    std::unique_ptr<service_manager::ServiceContextRef> service_ref)
    : service_ref_(std::move(service_ref)), creator_(creator) {}

PdfCompositorImpl::~PdfCompositorImpl() = default;

void PdfCompositorImpl::AddSubframeMap(uint32_t frame_id, uint32_t uid) {
  uid_to_id_map_[uid] = frame_id;
  id_to_uid_map_[frame_id].push_back(uid);
}

void PdfCompositorImpl::AddSubframeContent(
    uint32_t frame_id,
    mojo::ScopedSharedBufferHandle sk_handle,
    const std::vector<uint32_t>& subframe_uids) {
  // Add this frame and its information.
  std::unique_ptr<base::SharedMemory> shm =
      GetShmFromMojoHandle(std::move(sk_handle));
  frame_info_map_[frame_id] =
      std::make_unique<FrameInfo>(std::move(shm), subframe_uids);
  for (auto uid : id_to_uid_map_[frame_id])
    pending_frame_uids_.erase(uid);

  // Add any unpainted subframe into the pending set.
  for (auto uid : subframe_uids) {
    auto iter = uid_to_id_map_.find(uid);
    if (iter == uid_to_id_map_.end() ||
        frame_info_map_.find(uid_to_id_map_[uid]) == frame_info_map_.end()) {
      pending_frame_uids_.insert(uid);
    }
  }
}

void PdfCompositorImpl::IsReadyToComposite(
    uint32_t frame_id,
    uint32_t page_num,
    const std::vector<uint32_t>& subframe_uids,
    mojom::PdfCompositor::IsReadyToCompositeCallback callback) {
  // First, check whether it is already requested.
  std::pair<uint32_t, uint32_t> frame_key(frame_id, page_num);
  if (pending_requests_.find(frame_key) != pending_requests_.end()) {
    // Check whether the timeout is reached.
    if (pending_requests_[frame_key].timer.Elapsed() >
        base::TimeDelta::FromSeconds(kRequestTimeoutSec)) {
      // No longer waiting for the pending subframes.
      for (auto subframe_uid : subframe_uids)
        pending_frame_uids_.erase(subframe_uid);
      std::move(callback).Run(true);
    }
  }

  // Then, check whether all subframes are ready.
  for (auto uid : subframe_uids) {
    if (uid_to_id_map_.find(uid) == uid_to_id_map_.end() ||
        frame_info_map_.find(uid_to_id_map_[uid]) == frame_info_map_.end()) {
      std::move(callback).Run(false);
      return;
    }
  }
  std::move(callback).Run(true);
}

void PdfCompositorImpl::CompositeToPdf(
    uint32_t frame_id,
    uint32_t page_num,
    mojo::ScopedSharedBufferHandle sk_handle,
    const std::vector<uint32_t>& subframe_uids,
    mojom::PdfCompositor::CompositeToPdfCallback callback) {
  DCHECK(pending_requests_.find(std::pair<uint32_t, uint32_t>(
             frame_id, page_num)) == pending_requests_.end());

  // Recursively composite all subframes.
  std::vector<sk_sp<SkPicture>> subframes;
  for (auto uid : subframe_uids) {
    if (uid_to_id_map_.find(uid) == uid_to_id_map_.end()) {
      subframes.push_back(nullptr);
      continue;
    }

    auto id = uid_to_id_map_[uid];
    if (frame_info_map_.find(id) != frame_info_map_.end() &&
        frame_info_map_[id]->composited)
      subframes.push_back(frame_info_map_[id]->content);
    else
      subframes.push_back(CompositeSubframe(id));
  }

  // Read in content and convert it into pdf.
  std::unique_ptr<base::SharedMemory> shm =
      GetShmFromMojoHandle(std::move(sk_handle));
  SkMemoryStream stream(shm->memory(), shm->mapped_size());
  int page_count = SkMultiPictureDocumentReadPageCount(&stream);
  if (!page_count) {
    DLOG(ERROR) << "CompositePdf: No page is read.";
    std::move(callback).Run(mojom::PdfCompositor::Status::CONTENT_FORMAT_ERROR,
                            mojo::ScopedSharedBufferHandle());
    return;
  }

  std::vector<SkDocumentPage> pages(page_count);
#if defined(EXPERIMENTAL_SKIA)
  if (!SkMultiPictureContainerDocumentRead(&stream, &subframes, pages.data(),
                                           page_count)) {
#else
  if (!SkMultiPictureDocumentRead(&stream, pages.data(), page_count)) {
#endif
    DLOG(ERROR) << "CompositePdf: Page reading failed.";
    std::move(callback).Run(mojom::PdfCompositor::Status::CONTENT_FORMAT_ERROR,
                            mojo::ScopedSharedBufferHandle());
    return;
  }

  SkDynamicMemoryWStream wstream;
  sk_sp<SkDocument> doc = MakePdfDocument(creator_, &wstream);

  for (const auto& page : pages) {
    SkCanvas* canvas = doc->beginPage(page.fSize.width(), page.fSize.height());
    canvas->drawPicture(page.fPicture);
    doc->endPage();
  }
  doc->close();

  mojo::ScopedSharedBufferHandle buffer =
      mojo::SharedBufferHandle::Create(wstream.bytesWritten());
  DCHECK(buffer.is_valid());

  mojo::ScopedSharedBufferMapping mapping = buffer->Map(wstream.bytesWritten());
  DCHECK(mapping);
  wstream.copyToAndReset(mapping.get());

  std::move(callback).Run(mojom::PdfCompositor::Status::SUCCESS,
                          std::move(buffer));
}

std::unique_ptr<base::SharedMemory> PdfCompositorImpl::GetShmFromMojoHandle(
    mojo::ScopedSharedBufferHandle handle) {
  base::SharedMemoryHandle memory_handle;
  size_t memory_size = 0;
  bool read_only_flag = false;

  const MojoResult result = mojo::UnwrapSharedMemoryHandle(
      std::move(handle), &memory_handle, &memory_size, &read_only_flag);
  if (result != MOJO_RESULT_OK)
    return nullptr;
  DCHECK_GT(memory_size, 0u);

  std::unique_ptr<base::SharedMemory> shm =
      base::MakeUnique<base::SharedMemory>(memory_handle, true /* read_only */);
  if (!shm->Map(memory_size)) {
    DLOG(ERROR) << "Map shared memory failed.";
    return nullptr;
  }
  return shm;
}

sk_sp<SkPicture> PdfCompositorImpl::CompositeSubframe(uint32_t frame_id) {
  // Check whether the content of this frame is available.
  if (frame_info_map_.find(frame_id) == frame_info_map_.end())
    return nullptr;
  frame_info_map_[frame_id]->composited = true;

  // Composite subframes first.
  std::vector<sk_sp<SkPicture>> subframes;
  for (auto uid : frame_info_map_[frame_id]->subframe_uids) {
    if (uid_to_id_map_.find(uid) == uid_to_id_map_.end()) {
      subframes.push_back(nullptr);
      continue;
    }

    auto id = uid_to_id_map_[uid];
    if (frame_info_map_.find(id) != frame_info_map_.end() &&
        frame_info_map_[id]->composited)
      subframes.push_back(frame_info_map_[id]->content);
    else
      subframes.push_back(CompositeSubframe(id));
  }

  // Composite the entire frame.
  SkMemoryStream stream(
      frame_info_map_[frame_id]->serialized_content->memory(),
      frame_info_map_[frame_id]->serialized_content->mapped_size());
#if defined(EXPERIMENTAL_SKIA)
  frame_info_map_[frame_id]->content =
      SkReadEmbeddedPicture(&stream, &subframes);
#else
  frame_info_map_[frame_id]->content = nullptr;
#endif
  return frame_info_map_[frame_id]->content;
}

PdfCompositorImpl::FrameInfo::FrameInfo(
    std::unique_ptr<base::SharedMemory> content,
    std::vector<uint32_t> uids)
    : composited(false),
      serialized_content(std::move(content)),
      content(nullptr),
      subframe_uids(uids) {}

PdfCompositorImpl::FrameInfo::~FrameInfo() {}

PdfCompositorImpl::RequestInfo::RequestInfo() {}

PdfCompositorImpl::RequestInfo::~RequestInfo() {}

}  // namespace printing
