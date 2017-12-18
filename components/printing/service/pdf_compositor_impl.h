// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_PRINTING_SERVICE_PDF_COMPOSITOR_IMPL_H_
#define COMPONENTS_PRINTING_SERVICE_PDF_COMPOSITOR_IMPL_H_

#include <map>
#include <memory>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "base/containers/flat_map.h"
#include "base/macros.h"
#include "base/memory/ref_counted_memory.h"
#include "base/memory/shared_memory.h"
#include "base/time/time.h"
#include "base/timer/elapsed_timer.h"
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
  void AddSubframeMap(int frame_id, uint32_t content_id) override;
  void AddSubframeContent(
      int frame_id,
      mojo::ScopedSharedBufferHandle sk_handle,
      const std::vector<uint32_t>& subframe_content_ids) override;
  void IsReadyToComposite(
      int frame_id,
      uint32_t page_num,
      const std::vector<uint32_t>& subframe_content_ids,
      mojom::PdfCompositor::IsReadyToCompositeCallback callback) override;
  void CompositeToPdf(
      int frame_id,
      uint32_t page_num,
      mojo::ScopedSharedBufferHandle sk_handle,
      const std::vector<uint32_t>& subframe_content_ids,
      mojom::PdfCompositor::CompositeToPdfCallback callback) override;

 protected:
  // Defined as virtual so unit tests can override it.
  virtual base::TimeDelta Timeout() const;

 private:
  // Stores out-of-process child frame's content and its own subframe
  // information. It also stores status info during composition.
  struct FrameInfo {
    FrameInfo(std::unique_ptr<base::SharedMemory> content,
              const std::vector<uint32_t>& content_ids,
              const std::set<uint32_t>& pending_content_ids);
    ~FrameInfo();

    // Set to true when this frame's |serialized_content| is composed with
    // subframe content, and the final result is stored in |content|.
    bool composited;

    // Serialized SkPicture content of this frame.
    std::unique_ptr<base::SharedMemory> serialized_content;

    // Frame content after composition with subframe content.
    sk_sp<SkPicture> content;

    std::vector<uint32_t> subframe_content_ids;

    // All subframe content that is still unavailable for composition.
    std::set<uint32_t> pending_subframe_content_ids;
  };

  // Keeps the mapping between frame's id and its information.
  using FrameMap = base::flat_map<int, std::unique_ptr<FrameInfo>>;

  // When a frame is ready for composition, update its parent(s)
  // to no longer wait for it.
  void UpdateForCompleteFrame(int frame_id);

  // Composite the content of a subframe.
  sk_sp<SkPicture> CompositeSubframe(int frame_id);

  // Get the entry in |frame_info_map_| for content.
  FrameMap::iterator GetFrameEntryForContent(uint32_t content_id);

  const std::unique_ptr<service_manager::ServiceContextRef> service_ref_;
  const std::string creator_;

  // The following three maps store information about non-root subframes.
  // Keep track of all frames' information indexed by frame id.
  FrameMap frame_info_map_;
  // Map to track each content id with its own frame id and parent frame id.
  std::map<uint32_t, std::pair<int, int>> content_to_frame_id_map_;
  // Map to track each frame id with all content id associated with it.
  std::map<int, std::vector<uint32_t>> frame_to_content_id_map_;

  // Pending request map for compositing a frame or its page.
  // It maps <frame_id, page_number> to its timer.
  // The frames here are root frames for composition.
  std::map<std::pair<int, uint32_t>, base::ElapsedTimer> pending_requests_;

  DISALLOW_COPY_AND_ASSIGN(PdfCompositorImpl);
};

}  // namespace printing

#endif  // COMPONENTS_PRINTING_SERVICE_PDF_COMPOSITOR_IMPL_H_
