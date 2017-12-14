// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/printing/service/pdf_compositor_impl.h"

#include "base/logging.h"
#include "base/memory/shared_memory_handle.h"
#include "components/printing/service/public/cpp/pdf_service_mojo_utils.h"
#include "mojo/public/cpp/system/platform_handle.h"
#include "printing/common/pdf_metafile_utils.h"
#include "third_party/skia/include/core/SkCanvas.h"
#include "third_party/skia/include/core/SkDocument.h"
#include "third_party/skia/src/utils/SkMultiPictureDocument.h"

namespace printing {

static constexpr int kMinFrameId = std::numeric_limits<int>::min();

PdfCompositorImpl::PdfCompositorImpl(
    const std::string& creator,
    std::unique_ptr<service_manager::ServiceContextRef> service_ref)
    : service_ref_(std::move(service_ref)), creator_(creator) {}

PdfCompositorImpl::~PdfCompositorImpl() = default;

void PdfCompositorImpl::AddSubframeMap(int frame_id, uint32_t content_id) {
  auto iter = content_to_frame_id_map_.find(content_id);
  DCHECK(iter == content_to_frame_id_map_.end() ||
         iter->second.first == kMinFrameId);

  if (iter == content_to_frame_id_map_.end()) {
    content_to_frame_id_map_[content_id] =
        std::make_pair(frame_id, kMinFrameId);
  } else {
    content_to_frame_id_map_[content_id].first = frame_id;
  }
  frame_to_content_id_map_[frame_id].push_back(content_id);
}

void PdfCompositorImpl::AddSubframeContent(
    int frame_id,
    mojo::ScopedSharedBufferHandle sk_handle,
    const std::vector<uint32_t>& subframe_content_ids) {
  // Add any unpainted subframe into the pending set.
  std::set<uint32_t> pending_content_ids;
  for (auto content_id : subframe_content_ids) {
    if (GetFrameEntryForContent(content_id) == frame_info_map_.end()) {
      pending_content_ids.insert(content_id);
      // Record the pending subframe's parent frame id.
      auto iter = content_to_frame_id_map_.find(content_id);
      if (iter == content_to_frame_id_map_.end()) {
        content_to_frame_id_map_[content_id] =
            std::make_pair(kMinFrameId, frame_id);
      } else {
        // Don't overwrite parent frame id if it is already set.
        iter->second.second = frame_id;
      }
    }
  }

  // Add this frame, its serialized content and all pending content ids.
  std::unique_ptr<base::SharedMemory> shm =
      GetShmFromMojoHandle(std::move(sk_handle));
  DCHECK(frame_info_map_.find(frame_id) == frame_info_map_.end());
  frame_info_map_[frame_id] = std::make_unique<FrameInfo>(
      std::move(shm), subframe_content_ids, pending_content_ids);

  if (pending_content_ids.empty())
    UpdateForCompleteFrame(frame_id);
}

void PdfCompositorImpl::IsReadyToComposite(
    int frame_id,
    uint32_t page_num,
    const std::vector<uint32_t>& subframe_content_ids,
    mojom::PdfCompositor::IsReadyToCompositeCallback callback) {
  // If there is any reference to this frame, no need to wait.
  UpdateForCompleteFrame(frame_id);

  // First, check whether it is already requested.
  std::pair<int, uint32_t> frame_pair(frame_id, page_num);
  auto req_iter = pending_requests_.find(frame_pair);
  if (req_iter != pending_requests_.end() &&
      req_iter->second.Elapsed() > Timeout()) {
    // If the timeout is reached, no long wait for the pending subframes.
    pending_requests_.erase(req_iter);
    std::move(callback).Run(true);
    return;
  }

  // Then, check whether all subframes are ready.
  for (auto content_id : subframe_content_ids) {
    auto frame_iter = GetFrameEntryForContent(content_id);
    if (frame_iter == frame_info_map_.end() ||
        !frame_iter->second->pending_subframe_content_ids.empty()) {
      pending_requests_[frame_pair] = base::ElapsedTimer();
      std::move(callback).Run(false);
      return;
    }
  }
  if (req_iter != pending_requests_.end())
    pending_requests_.erase(req_iter);
  std::move(callback).Run(true);
}

void PdfCompositorImpl::CompositeToPdf(
    int frame_id,
    uint32_t page_num,
    mojo::ScopedSharedBufferHandle sk_handle,
    const std::vector<uint32_t>& subframe_content_ids,
    mojom::PdfCompositor::CompositeToPdfCallback callback) {
  DCHECK(!base::ContainsKey(pending_requests_,
                            std::make_pair(frame_id, page_num)));

  // Recursively composite all subframes.
  std::vector<sk_sp<SkPicture>> subframes;
  for (auto content_id : subframe_content_ids) {
    auto frame_iter = content_to_frame_id_map_.find(content_id);
    if (frame_iter == content_to_frame_id_map_.end()) {
      // Each subframe content is ordered in the vector.
      // So it should either has a SkPicture for its content, or nullptr.
      subframes.push_back(nullptr);
      continue;
    }

    int id = frame_iter->second.first;
    auto info_iter = frame_info_map_.find(id);
    if (info_iter != frame_info_map_.end() && info_iter->second->composited)
      subframes.push_back(info_iter->second->content);
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
  bool result;
#if defined(EXPERIMENTAL_SKIA)
  result = SkMultiPictureContainerDocumentRead(&stream, &subframes,
                                               pages.data(), page_count);
#else
  result = SkMultiPictureDocumentRead(&stream, pages.data(), page_count);
#endif
  if (!result) {
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

base::TimeDelta PdfCompositorImpl::Timeout() const {
  // The time limit for subframe painting and delivering.
  static constexpr base::TimeDelta kRequestTimeoutSec =
      base::TimeDelta::FromSeconds(15);
  return kRequestTimeoutSec;
}

void PdfCompositorImpl::UpdateForCompleteFrame(int frame_id) {
  // Find all outer frames that need this frame, and update them.
  auto content_ids_iter = frame_to_content_id_map_.find(frame_id);
  if (content_ids_iter == frame_to_content_id_map_.end())
    return;

  for (auto content_id : content_ids_iter->second) {
    auto iter = content_to_frame_id_map_.find(content_id);
    DCHECK(iter != content_to_frame_id_map_.end());

    int parent_frame_id = iter->second.second;
    auto parent_iter = frame_info_map_.find(parent_frame_id);
    if (parent_iter != frame_info_map_.end()) {
      parent_iter->second->pending_subframe_content_ids.erase(content_id);
      if (parent_iter->second->pending_subframe_content_ids.empty())
        UpdateForCompleteFrame(parent_frame_id);
    }
  }
}

sk_sp<SkPicture> PdfCompositorImpl::CompositeSubframe(int frame_id) {
  // Check whether the content of this frame is available.
  auto iter = frame_info_map_.find(frame_id);
  if (iter == frame_info_map_.end())
    return nullptr;
  std::unique_ptr<FrameInfo>& frame_info = iter->second;
  frame_info->composited = true;

  // Composite subframes first.
  std::vector<sk_sp<SkPicture>> subframes;
  for (auto content_id : frame_info->subframe_content_ids) {
    auto subframe_iter = GetFrameEntryForContent(content_id);
    if (subframe_iter == frame_info_map_.end()) {
      // When its content is unavailable, set the result as nullptr.
      subframes.push_back(nullptr);
      continue;
    }

    if (subframe_iter->second->composited) {
      subframes.push_back(subframe_iter->second->content);
    } else {
      subframes.push_back(CompositeSubframe(subframe_iter->first));
    }
  }

  // Composite the entire frame.
  SkMemoryStream stream(iter->second->serialized_content->memory(),
                        iter->second->serialized_content->mapped_size());
#if defined(EXPERIMENTAL_SKIA)
  iter->second->content = SkReadEmbeddedPicture(&stream, &subframes);
#else
  iter->second->content = nullptr;
#endif
  return iter->second->content;
}

PdfCompositorImpl::FrameMap::iterator
PdfCompositorImpl::GetFrameEntryForContent(uint32_t content_id) {
  auto iter = content_to_frame_id_map_.find(content_id);
  if (iter == content_to_frame_id_map_.end())
    return frame_info_map_.end();

  return frame_info_map_.find(iter->second.first);
}

PdfCompositorImpl::FrameInfo::FrameInfo(
    std::unique_ptr<base::SharedMemory> content,
    const std::vector<uint32_t>& content_ids,
    const std::set<uint32_t>& pending_content_ids)
    : composited(false),
      serialized_content(std::move(content)),
      subframe_content_ids(content_ids),
      pending_subframe_content_ids(pending_content_ids) {}

PdfCompositorImpl::FrameInfo::~FrameInfo() {}

}  // namespace printing
