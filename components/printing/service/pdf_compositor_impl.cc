// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/printing/service/pdf_compositor_impl.h"

#include <tuple>

#include "base/logging.h"
#include "base/memory/shared_memory_handle.h"
#include "components/printing/service/public/cpp/pdf_service_mojo_utils.h"
#include "mojo/public/cpp/system/platform_handle.h"
#include "printing/common/pdf_metafile_utils.h"
#include "third_party/skia/include/core/SkCanvas.h"
#include "third_party/skia/include/core/SkDocument.h"
#include "third_party/skia/include/core/SkSerialProcs.h"
#include "third_party/skia/src/utils/SkMultiPictureDocument.h"

namespace printing {

PdfCompositorImpl::PdfCompositorImpl(
    const std::string& creator,
    std::unique_ptr<service_manager::ServiceContextRef> service_ref)
    : service_ref_(std::move(service_ref)), creator_(creator) {}

PdfCompositorImpl::~PdfCompositorImpl() = default;

void PdfCompositorImpl::AddSubframeMap(uint64_t frame_guid,
                                       uint32_t content_id,
                                       uint64_t content_frame_guid) {
  // Add frame and its content info into |frame_info_map_|.
  auto iter = frame_info_map_.find(frame_guid);
  if (iter == frame_info_map_.end()) {
    frame_info_map_[frame_guid] = std::make_unique<FrameInfo>();
    frame_info_map_[frame_guid]->subframe_content_map[content_id] =
        content_frame_guid;
  } else {
    auto& content_map = iter->second->subframe_content_map;
    DCHECK(!base::ContainsKey(content_map, content_id));
    content_map[content_id] = content_frame_guid;
  }

  // Add the mapping between content and its corresponding frame.
  frame_to_content_map_[content_frame_guid].emplace(frame_guid, content_id);
}

void PdfCompositorImpl::AddSubframeContent(
    uint64_t frame_guid,
    mojo::ScopedSharedBufferHandle sk_handle,
    const std::vector<uint32_t>& subframe_content_ids) {
  // Add this frame, its serialized content and all pending content ids.
  if (!base::ContainsKey(frame_info_map_, frame_guid))
    frame_info_map_[frame_guid] = std::make_unique<FrameInfo>();
  std::unique_ptr<FrameInfo>& info = frame_info_map_[frame_guid];

  // The frame's rendered content is only set in this call.
  DCHECK(!info->serialized_content);
  DCHECK(info->pending_subframe_content_ids.empty());
  info->serialized_content = GetShmFromMojoHandle(std::move(sk_handle));
  GetPendingContentIds(frame_guid, subframe_content_ids,
                       &info->pending_subframe_content_ids);

  // When the content is complete, update all the frames which depend on
  // this frame.
  if (info->pending_subframe_content_ids.empty())
    UpdateForCompleteFrame(frame_guid);
}

void PdfCompositorImpl::CompositePageToPdf(
    uint64_t frame_guid,
    uint32_t page_num,
    mojo::ScopedSharedBufferHandle sk_handle,
    const std::vector<uint32_t>& subframe_content_ids,
    mojom::PdfCompositor::CompositePageToPdfCallback callback) {
  HandleCompositionRequest(frame_guid, page_num, std::move(sk_handle),
                           subframe_content_ids, std::move(callback));
}

void PdfCompositorImpl::CompositeDocumentToPdf(
    uint64_t frame_guid,
    mojo::ScopedSharedBufferHandle sk_handle,
    const std::vector<uint32_t>& subframe_content_ids,
    mojom::PdfCompositor::CompositeDocumentToPdfCallback callback) {
  HandleCompositionRequest(frame_guid, base::nullopt, std::move(sk_handle),
                           subframe_content_ids, std::move(callback));
}

bool PdfCompositorImpl::IsReadyToComposite(
    uint64_t frame_guid,
    const std::vector<uint32_t>& subframe_content_ids,
    base::flat_set<uint32_t>* pending_content_ids) {
  // For the frame we are compositing, set it as being ready to
  // prevent potiential circular references.
  UpdateForCompleteFrame(frame_guid);

  GetPendingContentIds(frame_guid, subframe_content_ids, pending_content_ids);
  if (pending_content_ids->empty())
    return true;
  return false;
}

void PdfCompositorImpl::HandleCompositionRequest(
    uint64_t frame_guid,
    base::Optional<uint32_t> page_num,
    mojo::ScopedSharedBufferHandle sk_handle,
    const std::vector<uint32_t>& subframe_content_ids,
    CompositeToPdfCallback callback) {
  // Record top frame's global unique id.
  top_frame_guid_ = frame_guid;

  base::flat_set<uint32_t> pending_content_ids;
  if (IsReadyToComposite(frame_guid, subframe_content_ids,
                         &pending_content_ids)) {
    mojo::ScopedSharedBufferHandle handle;
    auto status = CompositeToPdf(frame_guid, page_num,
                                 GetShmFromMojoHandle(std::move(sk_handle)),
                                 subframe_content_ids, &handle);
    std::move(callback).Run(status, std::move(handle));
    return;
  }

  // When it is not ready yet, keep its information and
  // wait until all dependent subframes are ready.
  if (!base::ContainsKey(frame_info_map_, frame_guid))
    frame_info_map_[frame_guid] = std::make_unique<FrameInfo>();
  requests_.emplace_back(new RequestInfo(
      page_num, GetShmFromMojoHandle(std::move(sk_handle)),
      subframe_content_ids, pending_content_ids, std::move(callback)));
}

mojom::PdfCompositor::Status PdfCompositorImpl::CompositeToPdf(
    uint64_t frame_guid,
    base::Optional<uint32_t> page_num,
    std::unique_ptr<base::SharedMemory> shared_mem,
    const std::vector<uint32_t>& subframe_content_ids,
    mojo::ScopedSharedBufferHandle* handle) {
  DeserializationContext subframes;
  GetDeserializationContext(frame_guid, subframe_content_ids, &subframes);

  // Read in content and convert it into pdf.
  SkMemoryStream stream(shared_mem->memory(), shared_mem->mapped_size());
  int page_count = SkMultiPictureDocumentReadPageCount(&stream);
  if (!page_count) {
    DLOG(ERROR) << "CompositeToPdf: No page is read.";
    return mojom::PdfCompositor::Status::CONTENT_FORMAT_ERROR;
  }

  std::vector<SkDocumentPage> pages(page_count);
  // TODO(weili): Change the default functions to actual implementation.
  SkDeserialProcs procs;
  if (!SkMultiPictureDocumentRead(&stream, pages.data(), page_count, &procs)) {
    DLOG(ERROR) << "CompositeToPdf: Page reading failed.";
    return mojom::PdfCompositor::Status::CONTENT_FORMAT_ERROR;
  }

  SkDynamicMemoryWStream wstream;
  sk_sp<SkDocument> doc = MakePdfDocument(creator_, &wstream);

  for (const auto& page : pages) {
    SkCanvas* canvas = doc->beginPage(page.fSize.width(), page.fSize.height());
    canvas->drawPicture(page.fPicture);
    doc->endPage();
  }
  doc->close();

  *handle = mojo::SharedBufferHandle::Create(wstream.bytesWritten());
  DCHECK((*handle).is_valid());

  mojo::ScopedSharedBufferMapping mapping =
      (*handle)->Map(wstream.bytesWritten());
  DCHECK(mapping);
  wstream.copyToAndReset(mapping.get());

  return mojom::PdfCompositor::Status::SUCCESS;
}

void PdfCompositorImpl::UpdateForCompleteFrame(uint64_t frame_guid) {
  // Find all frames that depends on this frame, and update them.
  auto content_iter = frame_to_content_map_.find(frame_guid);
  if (content_iter == frame_to_content_map_.end())
    return;

  for (const ContentGlobalId& id : content_iter->second) {
    uint64_t parent_frame_guid = id.frame_guid;
    auto parent_iter = frame_info_map_.find(parent_frame_guid);
    if (parent_iter == frame_info_map_.end())
      continue;

    std::unique_ptr<FrameInfo>& parent_info = parent_iter->second;
    // If parent frame is the top frame, need to check for pages' dependencies
    // as well.
    if (parent_frame_guid == top_frame_guid_) {
      for (auto& req_info : requests_) {
        if (req_info->pending_subframe_content_ids.erase(id.content_id) &&
            req_info->pending_subframe_content_ids.empty()) {
          // Page is ready for composition.
          mojo::ScopedSharedBufferHandle handle;
          auto status = CompositeToPdf(parent_frame_guid, req_info->page_number,
                                       std::move(req_info->serialized_content),
                                       req_info->subframe_content_ids, &handle);
          std::move(req_info->callback).Run(status, std::move(handle));
        }
      }
    } else {
      if (parent_info->pending_subframe_content_ids.erase(id.content_id) &&
          parent_info->pending_subframe_content_ids.empty()) {
        UpdateForCompleteFrame(parent_frame_guid);
      }
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
  DeserializationContext subframes;
  GetDeserializationContext(
      frame_guid, GetSubframeContentIds(frame_info->subframe_content_map),
      &subframes);

  // Composite the entire frame.
  SkMemoryStream stream(iter->second->serialized_content->memory(),
                        iter->second->serialized_content->mapped_size());
  // TODO(weili): Change the default functions to actual implementation.
  SkDeserialProcs procs;
  iter->second->content = SkPicture::MakeFromStream(&stream, &procs);
  return iter->second->content;
}

PdfCompositorImpl::FrameMap::iterator
PdfCompositorImpl::GetFrameEntryForContent(uint64_t frame_guid,
                                           uint32_t content_id) {
  auto iter = frame_info_map_.find(frame_guid);
  if (iter == frame_info_map_.end())
    return frame_info_map_.end();

  auto& content_map = iter->second->subframe_content_map;
  auto content_iter = content_map.find(content_id);
  return content_iter == content_map.end()
             ? frame_info_map_.end()
             : frame_info_map_.find(content_iter->second);
}

void PdfCompositorImpl::GetPendingContentIds(
    uint64_t frame_guid,
    const base::flat_set<uint32_t>& content_ids,
    base::flat_set<uint32_t>* pending_content_ids) {
  pending_content_ids->clear();
  for (auto content_id : content_ids) {
    auto frame_iter = GetFrameEntryForContent(frame_guid, content_id);
    if (frame_iter == frame_info_map_.end() ||
        !frame_iter->second->pending_subframe_content_ids.empty()) {
      pending_content_ids->insert(content_id);
    }
  }
  return;
}

void PdfCompositorImpl::GetDeserializationContext(
    uint64_t frame_guid,
    const base::flat_set<uint32_t>& subframe_content_ids,
    DeserializationContext* subframes) {
  // Recursively composite all subframes.
  for (auto content_id : subframe_content_ids) {
    auto frame_iter = GetFrameEntryForContent(frame_guid, content_id);
    if (frame_iter == frame_info_map_.end())
      continue;

    if (frame_iter->second->composited)
      (*subframes)[content_id] = frame_iter->second->content;
    else
      (*subframes)[content_id] = CompositeSubframe(frame_iter->first);
  }
}

std::vector<uint32_t> PdfCompositorImpl::GetSubframeContentIds(
    const SubframeContentMap& content_map) {
  std::vector<uint32_t> ids;
  for (auto& item : content_map)
    ids.push_back(item.first);
  return ids;
}

PdfCompositorImpl::FrameContentInfo::FrameContentInfo(
    std::unique_ptr<base::SharedMemory> content,
    const base::flat_set<uint32_t>& pending_content_ids)
    : serialized_content(std::move(content)),
      pending_subframe_content_ids(pending_content_ids) {}

PdfCompositorImpl::FrameContentInfo::FrameContentInfo() {}

PdfCompositorImpl::FrameContentInfo::~FrameContentInfo() {}

PdfCompositorImpl::FrameInfo::FrameInfo() {}

PdfCompositorImpl::FrameInfo::~FrameInfo() {}

PdfCompositorImpl::RequestInfo::RequestInfo(
    base::Optional<uint32_t> page_num,
    std::unique_ptr<base::SharedMemory> content,
    const std::vector<uint32_t>& content_ids,
    const base::flat_set<uint32_t>& pending_content_ids,
    mojom::PdfCompositor::CompositePageToPdfCallback callback)
    : FrameContentInfo(std::move(content), pending_content_ids),
      page_number(page_num),
      subframe_content_ids(content_ids),
      callback(std::move(callback)) {}

PdfCompositorImpl::RequestInfo::~RequestInfo() {}

PdfCompositorImpl::ContentGlobalId::ContentGlobalId(uint64_t frame_guid,
                                                    uint32_t content_id)
    : frame_guid(frame_guid), content_id(content_id) {}

PdfCompositorImpl::ContentGlobalId::~ContentGlobalId() {}

bool PdfCompositorImpl::ContentGlobalId::operator==(
    const ContentGlobalId& other) const {
  return frame_guid == other.frame_guid && content_id == other.content_id;
}

bool PdfCompositorImpl::ContentGlobalId::operator<(
    const ContentGlobalId& other) const {
  return std::tie(frame_guid, content_id) <
         std::tie(other.frame_guid, other.content_id);
}

}  // namespace printing
