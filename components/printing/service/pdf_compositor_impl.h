// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_PRINTING_SERVICE_PDF_COMPOSITOR_IMPL_H_
#define COMPONENTS_PRINTING_SERVICE_PDF_COMPOSITOR_IMPL_H_

#include <memory>
#include <string>
#include <vector>

#include "base/containers/flat_map.h"
#include "base/containers/flat_set.h"
#include "base/macros.h"
#include "base/memory/ref_counted_memory.h"
#include "base/memory/shared_memory.h"
#include "base/optional.h"
#include "components/printing/service/public/interfaces/pdf_compositor.mojom.h"
#include "mojo/public/cpp/system/buffer.h"
#include "services/service_manager/public/cpp/service_context_ref.h"
#include "third_party/skia/include/core/SkPicture.h"
#include "third_party/skia/include/core/SkRefCnt.h"

namespace printing {

using mojom::ContentToFrameInfo;
using mojom::ContentToFrameInfoPtr;

class PdfCompositorImpl : public mojom::PdfCompositor {
 public:
  PdfCompositorImpl(
      const std::string& creator,
      std::unique_ptr<service_manager::ServiceContextRef> service_ref);
  ~PdfCompositorImpl() override;

  // mojom::PdfCompositor
  void AddSubframeContent(
      uint64_t frame_guid,
      mojo::ScopedSharedBufferHandle serialized_content,
      std::vector<ContentToFrameInfoPtr> subframe_content_info) override;
  void CompositePageToPdf(
      uint64_t frame_guid,
      uint32_t page_num,
      mojo::ScopedSharedBufferHandle serialized_content,
      std::vector<ContentToFrameInfoPtr> subframe_content_info,
      mojom::PdfCompositor::CompositePageToPdfCallback callback) override;
  void CompositeDocumentToPdf(
      uint64_t frame_guid,
      mojo::ScopedSharedBufferHandle serialized_content,
      std::vector<ContentToFrameInfoPtr> subframe_content_info,
      mojom::PdfCompositor::CompositeDocumentToPdfCallback callback) override;

 private:
  FRIEND_TEST_ALL_PREFIXES(PdfCompositorImplTest, IsReadyToComposite);
  FRIEND_TEST_ALL_PREFIXES(PdfCompositorImplTest, MultiLayerDependency);
  FRIEND_TEST_ALL_PREFIXES(PdfCompositorImplTest, DependencyLoop);

  // The map needed during content deserialization. It stores the mapping
  // between content id and its actual content.
  using DeserializationContext = base::flat_map<uint32_t, sk_sp<SkPicture>>;

  // This is the uniform underlying type for both
  // mojom::PdfCompositor::CompositePageToPdfCallback and
  // mojom::PdfCompositor::CompositeDocumentToPdfCallback.
  using CompositeToPdfCallback =
      base::OnceCallback<void(PdfCompositor::Status,
                              mojo::ScopedSharedBufferHandle)>;

  // Base structure to store a frame's content and its subframe
  // content information.
  struct FrameContentInfo {
    FrameContentInfo(std::unique_ptr<base::SharedMemory> content);
    FrameContentInfo();
    ~FrameContentInfo();

    // Serialized SkPicture content of this frame.
    std::unique_ptr<base::SharedMemory> serialized_content;

    // Frame content after composition with subframe content.
    sk_sp<SkPicture> content;

    // Subframe content id and its corresponding frame guid.
    std::vector<ContentToFrameInfoPtr> subframe_content_info;
  };

  // Other than content, it also stores the mapping for all the subframe content
  // to the frames they refer to, and status during frame composition.
  struct FrameInfo : public FrameContentInfo {
    FrameInfo();
    ~FrameInfo();

    // The following fields are used for storing composition status.
    // Set to true when this frame's |serialized_content| is composed with
    // subframe content and the final result is stored in |content|.
    bool composited = false;
  };

  // Stores the mapping between frame's global unique ids and their
  // corresponding frame information.
  using FrameMap = base::flat_map<uint64_t, std::unique_ptr<FrameInfo>>;

  // Stores the page or document's request information.
  struct RequestInfo : public FrameContentInfo {
    RequestInfo(uint64_t frame_guid,
                base::Optional<uint32_t> page_num,
                std::unique_ptr<base::SharedMemory> content,
                const std::vector<ContentToFrameInfoPtr>& content_info,
                const base::flat_set<uint64_t>& pending_subframes,
                CompositeToPdfCallback callback);
    ~RequestInfo();

    uint64_t frame_guid;
    base::Optional<uint32_t> page_number;

    // All pending frame ids whose content is not available but needed
    // for composition.
    base::flat_set<uint64_t> pending_subframes;

    CompositeToPdfCallback callback;
  };

  // Check whether the frame with a list of subframe content is ready to
  // composite. If not, return all unavailable frames' ids in
  // |pending_subframes|.
  bool IsReadyToComposite(
      uint64_t frame_guid,
      const std::vector<ContentToFrameInfoPtr>& subframe_content_info,
      base::flat_set<uint64_t>* pending_subframes);

  // Recursive check all the frames |frame_guid| depends on and put those
  // not ready in |pending_subframes|.
  void CheckFramesForReadiness(
      uint64_t frame_guid,
      const std::vector<ContentToFrameInfoPtr>& subframe_content_info,
      base::flat_set<uint64_t>* pending_subframes,
      base::flat_set<uint64_t>* visited);

  // The internal implementation for handling page and documentation composition
  // requests.
  void HandleCompositionRequest(
      uint64_t frame_guid,
      base::Optional<uint32_t> page_num,
      mojo::ScopedSharedBufferHandle serialized_content,
      const std::vector<ContentToFrameInfoPtr>& subframe_content_ids,
      CompositeToPdfCallback callback);

  // The core function for content composition and conversion to a pdf file.
  mojom::PdfCompositor::Status CompositeToPdf(
      uint64_t frame_guid,
      base::Optional<uint32_t> page_num,
      std::unique_ptr<base::SharedMemory> shared_mem,
      const std::vector<ContentToFrameInfoPtr>& subframe_content_info,
      mojo::ScopedSharedBufferHandle* handle);

  // Composite the content of a subframe.
  sk_sp<SkPicture> CompositeSubframe(uint64_t frame_guid);

  bool CheckForPendingFrame(uint64_t frame_guid,
                            base::flat_set<uint64_t> visited_frames);

  void AddToDeserializationContext(uint64_t frame_guid,
                                   uint32_t content_id,
                                   DeserializationContext* subframes);

  void FullfillRequest(
      uint64_t frame_guid,
      base::Optional<uint32_t> page_num,
      std::unique_ptr<base::SharedMemory> serialized_content,
      const std::vector<ContentToFrameInfoPtr>& subframe_content_info,
      CompositeToPdfCallback callback);

  const std::unique_ptr<service_manager::ServiceContextRef> service_ref_;
  const std::string creator_;

  // Keep track of all frames' information indexed by frame id.
  FrameMap frame_info_map_;

  std::vector<std::unique_ptr<RequestInfo>> requests_;

  DISALLOW_COPY_AND_ASSIGN(PdfCompositorImpl);
};

}  // namespace printing

#endif  // COMPONENTS_PRINTING_SERVICE_PDF_COMPOSITOR_IMPL_H_
