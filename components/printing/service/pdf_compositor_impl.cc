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

// Use this as the default value when a frame's global unique id is unknown.
static constexpr uint64_t kMaxFrameGuid = std::numeric_limits<uint64_t>::max();

PdfCompositorImpl::PdfCompositorImpl(
    const std::string& creator,
    std::unique_ptr<service_manager::ServiceContextRef> service_ref)
    : service_ref_(std::move(service_ref)), creator_(creator) {}

PdfCompositorImpl::~PdfCompositorImpl() = default;

void PdfCompositorImpl::AddSubframeMap(uint64_t frame_guid,
                                       uint64_t content_id) {
  auto iter = content_to_frame_node_map_.find(content_id);
  DCHECK(iter == content_to_frame_node_map_.end() ||
         iter->second.frame_guid == kMaxFrameGuid);

  if (iter == content_to_frame_node_map_.end()) {
    content_to_frame_node_map_.emplace(
        content_id, FrameNodeInfo(frame_guid, kMaxFrameGuid));
  } else {
    iter->second.frame_guid = frame_guid;
  }
  frame_to_content_id_map_[frame_guid].push_back(content_id);
}

void PdfCompositorImpl::AddSubframeContent(
    uint64_t frame_guid,
    mojo::ScopedSharedBufferHandle sk_handle,
    const std::vector<uint64_t>& subframe_content_ids) {
  // Add any unpainted subframe into the pending set.
  base::flat_set<uint64_t> pending_content_ids;
  for (auto content_id : subframe_content_ids) {
    // Content and its corresponding frame is added by AddSubframeMap() which
    // may be called before or after this call. Check whether the map entry is
    // already there.
    if (GetFrameEntryForContent(content_id) == frame_info_map_.end()) {
      pending_content_ids.insert(content_id);
      // Record the pending subframe's parent frame id.
      auto iter = content_to_frame_node_map_.find(content_id);
      if (iter == content_to_frame_node_map_.end()) {
        content_to_frame_node_map_.emplace(
            content_id, FrameNodeInfo(kMaxFrameGuid, frame_guid));
      } else {
        // Don't overwrite parent frame id if it is already set.
        iter->second.parent_frame_guid = frame_guid;
      }
    }
  }

  // Add this frame, its serialized content and all pending content ids.
  std::unique_ptr<base::SharedMemory> shm =
      GetShmFromMojoHandle(std::move(sk_handle));
  DCHECK(!base::ContainsKey(frame_info_map_, frame_guid));
  frame_info_map_[frame_guid] = std::make_unique<FrameInfo>(
      std::move(shm), subframe_content_ids, pending_content_ids);

  if (pending_content_ids.empty())
    UpdateForCompleteFrame(frame_guid);
}

void PdfCompositorImpl::IsReadyToComposite(
    uint64_t frame_guid,
    uint32_t page_num,
    const std::vector<uint64_t>& subframe_content_ids,
    mojom::PdfCompositor::IsReadyToCompositeCallback callback) {
  // For the root frame we are compositing, set it as ready to
  // prevent potiential circular references.
  UpdateForCompleteFrame(frame_guid);

  // First, check whether it is already requested.
  RequestId req(frame_guid, page_num);
  auto req_iter = pending_requests_.find(req);
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
      pending_requests_[req] = base::ElapsedTimer();
      std::move(callback).Run(false);
      return;
    }
  }
  if (req_iter != pending_requests_.end())
    pending_requests_.erase(req_iter);
  std::move(callback).Run(true);
}

void PdfCompositorImpl::CompositeToPdf(
    uint64_t frame_guid,
    uint32_t page_num,
    mojo::ScopedSharedBufferHandle sk_handle,
    const std::vector<uint64_t>& subframe_content_ids,
    mojom::PdfCompositor::CompositeToPdfCallback callback) {
  DCHECK(
      !base::ContainsKey(pending_requests_, RequestId(frame_guid, page_num)));

  // Recursively composite all subframes.
  std::map<uint64_t, sk_sp<SkPicture>> subframes;
  for (auto content_id : subframe_content_ids) {
    auto frame_iter = content_to_frame_node_map_.find(content_id);
    if (frame_iter == content_to_frame_node_map_.end())
      continue;

    uint64_t id = frame_iter->second.frame_guid;
    auto info_iter = frame_info_map_.find(id);
    if (info_iter != frame_info_map_.end() && info_iter->second->composited)
      subframes[content_id] = info_iter->second->content;
    else
      subframes[content_id] = CompositeSubframe(id);
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
  result =
      SkMultiPictureDocumentRead(&stream, pages.data(), page_count, &subframes);
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
  // TODO(weili): check whether we need to adjust this, or add more
  //              monitoring code in browser.
  static constexpr base::TimeDelta kRequestTimeoutSec =
      base::TimeDelta::FromSeconds(60);
  return kRequestTimeoutSec;
}

void PdfCompositorImpl::UpdateForCompleteFrame(uint64_t frame_guid) {
  // Find all outer frames that need this frame, and update them.
  auto content_ids_iter = frame_to_content_id_map_.find(frame_guid);
  if (content_ids_iter == frame_to_content_id_map_.end())
    return;

  for (auto content_id : content_ids_iter->second) {
    auto iter = content_to_frame_node_map_.find(content_id);
    DCHECK(iter != content_to_frame_node_map_.end());

    uint64_t parent_frame_guid = iter->second.parent_frame_guid;
    auto parent_iter = frame_info_map_.find(parent_frame_guid);
    if (parent_iter != frame_info_map_.end()) {
      parent_iter->second->pending_subframe_content_ids.erase(content_id);
      if (parent_iter->second->pending_subframe_content_ids.empty())
        UpdateForCompleteFrame(parent_frame_guid);
    }
  }
}

sk_sp<SkPicture> PdfCompositorImpl::CompositeSubframe(uint64_t frame_guid) {
  // Check whether the content of this frame is available.
  auto iter = frame_info_map_.find(frame_guid);
  if (iter == frame_info_map_.end())
    return nullptr;
  std::unique_ptr<FrameInfo>& frame_info = iter->second;
  frame_info->composited = true;

  // Composite subframes first.
  std::map<uint64_t, sk_sp<SkPicture>> subframes;
  for (auto content_id : frame_info->subframe_content_ids) {
    auto subframe_iter = GetFrameEntryForContent(content_id);
    if (subframe_iter == frame_info_map_.end())
      continue;

    if (subframe_iter->second->composited) {
      subframes[content_id] = subframe_iter->second->content;
    } else {
      subframes[content_id] = CompositeSubframe(subframe_iter->first);
    }
  }

  // Composite the entire frame.
  SkMemoryStream stream(iter->second->serialized_content->memory(),
                        iter->second->serialized_content->mapped_size());
#if defined(EXPERIMENTAL_SKIA)
  iter->second->content =
      SkDeserializePictureWithOopContent(&stream, &subframes);
#else
  iter->second->content = nullptr;
#endif
  return iter->second->content;
}

PdfCompositorImpl::FrameMap::iterator
PdfCompositorImpl::GetFrameEntryForContent(uint64_t content_id) {
  auto iter = content_to_frame_node_map_.find(content_id);
  if (iter == content_to_frame_node_map_.end())
    return frame_info_map_.end();

  return frame_info_map_.find(iter->second.frame_guid);
}

PdfCompositorImpl::FrameInfo::FrameInfo(
    std::unique_ptr<base::SharedMemory> content,
    const std::vector<uint64_t>& content_ids,
    const base::flat_set<uint64_t>& pending_content_ids)
    : composited(false),
      serialized_content(std::move(content)),
      subframe_content_ids(content_ids),
      pending_subframe_content_ids(pending_content_ids) {}

PdfCompositorImpl::FrameInfo::~FrameInfo() {}

}  // namespace printing
