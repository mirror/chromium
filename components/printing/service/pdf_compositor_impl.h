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

class PdfCompositorImpl : public mojom::PdfCompositor {
 public:
  PdfCompositorImpl(
      const std::string& creator,
      std::unique_ptr<service_manager::ServiceContextRef> service_ref);
  ~PdfCompositorImpl() override;

  // mojom::PdfCompositor
  void AddSubframeMap(uint64_t frame_guid,
                      uint32_t content_id,
                      uint64_t content_frame_guid) override;
  void AddSubframeContent(
      uint64_t frame_guid,
      mojo::ScopedSharedBufferHandle sk_handle,
      const std::vector<uint32_t>& subframe_content_ids) override;
  void CompositePageToPdf(
      uint64_t frame_guid,
      uint32_t page_num,
      mojo::ScopedSharedBufferHandle sk_handle,
      const std::vector<uint32_t>& subframe_content_ids,
      mojom::PdfCompositor::CompositePageToPdfCallback callback) override;
  void CompositeDocumentToPdf(
      uint64_t frame_guid,
      mojo::ScopedSharedBufferHandle sk_handle,
      const std::vector<uint32_t>& subframe_content_ids,
      mojom::PdfCompositor::CompositeDocumentToPdfCallback callback) override;

 private:
  FRIEND_TEST_ALL_PREFIXES(PdfCompositorImplTest, IsReadyToComposite);
  FRIEND_TEST_ALL_PREFIXES(PdfCompositorImplTest, MultiLayerDependency);
  FRIEND_TEST_ALL_PREFIXES(PdfCompositorImplTest, DependencyLoop);

  // Stores the mapping between content ids and their corresponding frames'
  // global unique ids.
  using SubframeContentMap = base::flat_map<uint32_t, uint64_t>;

  // The map needed during content deserialization. It stores the mapping
  // between content id and its actual content.
  using DeserializationContext = std::map<uint32_t, sk_sp<SkPicture>>;

  // This is the uniform underlying type for both
  // mojom::PdfCompositor::CompositePageToPdfCallback and
  // mojom::PdfCompositor::CompositeDocumentToPdfCallback.
  using CompositeToPdfCallback =
      base::OnceCallback<void(PdfCompositor::Status,
                              mojo::ScopedSharedBufferHandle)>;

  // Base structure to store a frame's content and its subframe
  // content information.
  struct FrameContentInfo {
    FrameContentInfo(std::unique_ptr<base::SharedMemory> content,
                     const base::flat_set<uint32_t>& pending_content_ids);
    FrameContentInfo();
    ~FrameContentInfo();

    // Serialized SkPicture content of this frame.
    std::unique_ptr<base::SharedMemory> serialized_content;

    // Frame content after composition with subframe content.
    sk_sp<SkPicture> content;

    // All subframe content ids whose content is not available for composition.
    base::flat_set<uint32_t> pending_subframe_content_ids;
  };

  // Other than content, it also stores the mapping for all the subframe content
  // to the frames they refer to, and status during frame composition.
  struct FrameInfo : public FrameContentInfo {
    FrameInfo();
    ~FrameInfo();

    // All subframe content this frame refers to during printing.
    SubframeContentMap subframe_content_map;

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
    RequestInfo(base::Optional<uint32_t> page_num,
                std::unique_ptr<base::SharedMemory> content,
                const std::vector<uint32_t>& content_ids,
                const base::flat_set<uint32_t>& pending_content_ids,
                CompositeToPdfCallback callback);
    ~RequestInfo();

    base::Optional<uint32_t> page_number;
    std::vector<uint32_t> subframe_content_ids;
    CompositeToPdfCallback callback;
  };

  // The id used to globally identify a piece of content.
  // |frame_guid|: the global unique id of the host frame where the
  // content was painted;
  // |content_id|: the content id which is a unique id within its
  // painting process.
  struct ContentGlobalId {
    ContentGlobalId(uint64_t frame_guid, uint32_t content_id);
    ~ContentGlobalId();

    bool operator==(const ContentGlobalId& other) const;
    bool operator<(const ContentGlobalId& other) const;

    uint64_t frame_guid;
    uint32_t content_id;
  };

  // Check whether the frame with a list of subframe content is ready to
  // composite. If not, return all unavailable content ids in
  // |pending_content_ids|.
  bool IsReadyToComposite(uint64_t frame_guid,
                          const std::vector<uint32_t>& subframe_content_ids,
                          base::flat_set<uint32_t>* pending_content_ids);

  // The internal implementation for handling page and documentation composition
  // requests.
  void HandleCompositionRequest(
      uint64_t frame_guid,
      base::Optional<uint32_t> page_num,
      mojo::ScopedSharedBufferHandle sk_handle,
      const std::vector<uint32_t>& subframe_content_ids,
      CompositeToPdfCallback callback);

  // The core function for content composition and conversion to a pdf file.
  mojom::PdfCompositor::Status CompositeToPdf(
      uint64_t frame_guid,
      base::Optional<uint32_t> page_num,
      std::unique_ptr<base::SharedMemory> shared_mem,
      const std::vector<uint32_t>& subframe_content_ids,
      mojo::ScopedSharedBufferHandle* handle);

  // When a frame is ready for composition, update its parent(s)
  // to no longer wait for it.
  void UpdateForCompleteFrame(uint64_t frame_guid);

  // Composite the content of a subframe.
  sk_sp<SkPicture> CompositeSubframe(uint64_t frame_guid);

  // Get the entry in |frame_info_map_| for content.
  FrameMap::iterator GetFrameEntryForContent(uint64_t frame_guid,
                                             uint32_t content_id);

  void GetPendingContentIds(uint64_t frame_guid,
                            const base::flat_set<uint32_t>& content_ids,
                            base::flat_set<uint32_t>* pending_content_ids);

  void GetDeserializationContext(
      uint64_t frame_guid,
      const base::flat_set<uint32_t>& subframe_content_ids,
      DeserializationContext* subframes);

  std::vector<uint32_t> GetSubframeContentIds(
      const SubframeContentMap& content_map);

  const std::unique_ptr<service_manager::ServiceContextRef> service_ref_;
  const std::string creator_;

  // Keep track of all frames' information indexed by frame id.
  FrameMap frame_info_map_;

  // Map to track each frame's global unique id with all content ids that
  // associated with it.
  base::flat_map<uint64_t, base::flat_set<ContentGlobalId>>
      frame_to_content_map_;

  uint64_t top_frame_guid_ = 0;

  std::vector<std::unique_ptr<RequestInfo>> requests_;

  DISALLOW_COPY_AND_ASSIGN(PdfCompositorImpl);
};

}  // namespace printing

#endif  // COMPONENTS_PRINTING_SERVICE_PDF_COMPOSITOR_IMPL_H_
