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

void PdfCompositorImpl::AddSubframeContent(
    uint64_t frame_guid,
    mojo::ScopedSharedBufferHandle serialized_content,
    std::vector<ContentToFrameInfoPtr> subframe_content_info) {
  // Add this frame and its serialized content.
  DCHECK(!base::ContainsKey(frame_info_map_, frame_guid));
  frame_info_map_[frame_guid] = std::make_unique<FrameInfo>();
  std::unique_ptr<FrameInfo>& frame_info = frame_info_map_[frame_guid];
  frame_info->serialized_content =
      GetShmFromMojoHandle(std::move(serialized_content));

  // Add the subframe content information.
  std::vector<uint64_t> pending_subframes;
  for (auto& info : subframe_content_info) {
    auto subframe_guid = info->frame_guid;
    frame_info->subframe_content_info.push_back(info.Clone());
    if (!base::ContainsKey(frame_info_map_, subframe_guid))
      pending_subframes.push_back(subframe_guid);
  }

  // Check each request's pending list.
  // If the request is ready, fullfill it.
  for (auto& request : requests_) {
    auto& pending_list = request->pending_subframes;
    if (!pending_list.erase(frame_guid))
      continue;
    std::copy(pending_subframes.begin(), pending_subframes.end(),
              std::inserter(pending_list, pending_list.end()));
    if (pending_list.empty()) {
      FullfillRequest(request->frame_guid, request->page_number,
                      std::move(request->serialized_content),
                      request->subframe_content_info,
                      std::move(request->callback));
    }
  }
}

void PdfCompositorImpl::CompositePageToPdf(
    uint64_t frame_guid,
    uint32_t page_num,
    mojo::ScopedSharedBufferHandle serialized_content,
    std::vector<ContentToFrameInfoPtr> subframe_content_info,
    mojom::PdfCompositor::CompositePageToPdfCallback callback) {
  HandleCompositionRequest(frame_guid, page_num, std::move(serialized_content),
                           subframe_content_info, std::move(callback));
}

void PdfCompositorImpl::CompositeDocumentToPdf(
    uint64_t frame_guid,
    mojo::ScopedSharedBufferHandle serialized_content,
    std::vector<ContentToFrameInfoPtr> subframe_content_info,
    mojom::PdfCompositor::CompositeDocumentToPdfCallback callback) {
  HandleCompositionRequest(frame_guid, base::nullopt,
                           std::move(serialized_content), subframe_content_info,
                           std::move(callback));
}

bool PdfCompositorImpl::IsReadyToComposite(
    uint64_t frame_guid,
    const std::vector<ContentToFrameInfoPtr>& subframe_content_info,
    base::flat_set<uint64_t>* pending_subframes) {
  pending_subframes->clear();
  base::flat_set<uint64_t> visited_frames;
  visited_frames.insert(frame_guid);
  CheckFramesForReadiness(frame_guid, subframe_content_info, pending_subframes,
                          &visited_frames);
  return pending_subframes->empty();
}

void PdfCompositorImpl::CheckFramesForReadiness(
    uint64_t frame_guid,
    const std::vector<ContentToFrameInfoPtr>& subframe_content_info,
    base::flat_set<uint64_t>* pending_subframes,
    base::flat_set<uint64_t>* visited) {
  for (auto& info : subframe_content_info) {
    auto subframe_id = info->frame_guid;
    // If this frame has been checked, skip it.
    auto result = visited->insert(subframe_id);
    if (!result.second)
      continue;

    auto iter = frame_info_map_.find(subframe_id);
    if (iter == frame_info_map_.end()) {
      pending_subframes->insert(subframe_id);
    } else {
      CheckFramesForReadiness(subframe_id, iter->second->subframe_content_info,
                              pending_subframes, visited);
    }
  }
}

void PdfCompositorImpl::HandleCompositionRequest(
    uint64_t frame_guid,
    base::Optional<uint32_t> page_num,
    mojo::ScopedSharedBufferHandle serialized_content,
    const std::vector<ContentToFrameInfoPtr>& subframe_content_info,
    CompositeToPdfCallback callback) {
  base::flat_set<uint64_t> pending_subframes;
  if (IsReadyToComposite(frame_guid, subframe_content_info,
                         &pending_subframes)) {
    FullfillRequest(frame_guid, page_num,
                    GetShmFromMojoHandle(std::move(serialized_content)),
                    subframe_content_info, std::move(callback));
    return;
  }

  // When it is not ready yet, keep its information and
  // wait until all dependent subframes are ready.
  DCHECK(!base::ContainsKey(frame_info_map_, frame_guid));
  frame_info_map_[frame_guid] = std::make_unique<FrameInfo>();
  auto& content_info = frame_info_map_[frame_guid]->subframe_content_info;
  for (auto& info : subframe_content_info)
    content_info.push_back(info.Clone());

  requests_.emplace_back(new RequestInfo(
      frame_guid, page_num, GetShmFromMojoHandle(std::move(serialized_content)),
      content_info, std::move(pending_subframes), std::move(callback)));
}

mojom::PdfCompositor::Status PdfCompositorImpl::CompositeToPdf(
    uint64_t frame_guid,
    base::Optional<uint32_t> page_num,
    std::unique_ptr<base::SharedMemory> shared_mem,
    const std::vector<ContentToFrameInfoPtr>& subframe_content_info,
    mojo::ScopedSharedBufferHandle* handle) {
  DeserializationContext subframes;
  for (auto& info : subframe_content_info)
    AddToDeserializationContext(info->frame_guid, info->content_id, &subframes);

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

sk_sp<SkPicture> PdfCompositorImpl::CompositeSubframe(uint64_t frame_guid) {
  // The content of this frame should be available.
  auto iter = frame_info_map_.find(frame_guid);
  DCHECK(iter != frame_info_map_.end());
  std::unique_ptr<FrameInfo>& frame_info = iter->second;
  frame_info->composited = true;

  // Composite subframes first.
  DeserializationContext subframes;
  for (auto& info : frame_info->subframe_content_info) {
    AddToDeserializationContext(info->frame_guid, info->content_id, &subframes);
  }

  // Composite the entire frame.
  SkMemoryStream stream(iter->second->serialized_content->memory(),
                        iter->second->serialized_content->mapped_size());
  // TODO(weili): Change the default functions to actual implementation.
  SkDeserialProcs procs;
  iter->second->content = SkPicture::MakeFromStream(&stream, &procs);
  return iter->second->content;
}

void PdfCompositorImpl::AddToDeserializationContext(
    uint64_t frame_guid,
    uint32_t content_id,
    DeserializationContext* subframes) {
  auto frame_iter = frame_info_map_.find(frame_guid);
  if (frame_iter == frame_info_map_.end())
    return;

  if (frame_iter->second->composited)
    (*subframes)[content_id] = frame_iter->second->content;
  else
    (*subframes)[content_id] = CompositeSubframe(frame_iter->first);
}

void PdfCompositorImpl::FullfillRequest(
    uint64_t frame_guid,
    base::Optional<uint32_t> page_num,
    std::unique_ptr<base::SharedMemory> serialized_content,
    const std::vector<ContentToFrameInfoPtr>& subframe_content_info,
    CompositeToPdfCallback callback) {
  mojo::ScopedSharedBufferHandle handle;
  auto status =
      CompositeToPdf(frame_guid, page_num, std::move(serialized_content),
                     subframe_content_info, &handle);
  std::move(callback).Run(status, std::move(handle));
}

PdfCompositorImpl::FrameContentInfo::FrameContentInfo(
    std::unique_ptr<base::SharedMemory> content)
    : serialized_content(std::move(content)) {}

PdfCompositorImpl::FrameContentInfo::FrameContentInfo() {}

PdfCompositorImpl::FrameContentInfo::~FrameContentInfo() {}

PdfCompositorImpl::FrameInfo::FrameInfo() {}

PdfCompositorImpl::FrameInfo::~FrameInfo() {}

PdfCompositorImpl::RequestInfo::RequestInfo(
    uint64_t frame_guid,
    base::Optional<uint32_t> page_num,
    std::unique_ptr<base::SharedMemory> content,
    const std::vector<ContentToFrameInfoPtr>& content_info,
    const base::flat_set<uint64_t>& pending_subframes,
    mojom::PdfCompositor::CompositePageToPdfCallback callback)
    : FrameContentInfo(std::move(content)),
      frame_guid(frame_guid),
      page_number(page_num),
      pending_subframes(pending_subframes),
      callback(std::move(callback)) {
  for (auto& info : content_info)
    subframe_content_info.push_back(info.Clone());
}

PdfCompositorImpl::RequestInfo::~RequestInfo() {}

}  // namespace printing
