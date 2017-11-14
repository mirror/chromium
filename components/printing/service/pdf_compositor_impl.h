// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_PRINTING_SERVICE_PDF_COMPOSITOR_IMPL_H_
#define COMPONENTS_PRINTING_SERVICE_PDF_COMPOSITOR_IMPL_H_

#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/macros.h"
#include "base/memory/ref_counted_memory.h"
#include "base/memory/shared_memory.h"
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

  // Add the map between a frame's id and its content uid.
  void AddSubframeMap(uint32_t frame_id, uint32_t uid) override;

  // Add frame content and its subframe info.
  // |sk_handle| points to its content in Skia picture format.
  void AddSubframeContent(uint32_t frame_id,
                          mojo::ScopedSharedBufferHandle sk_handle,
                          const std::vector<uint32_t>& subframe_uids) override;

  // Check whether it is ready to composite this frame.
  void IsReadyToComposite(
      uint32_t frame_id,
      uint32_t page_num,
      const std::vector<uint32_t>& subframe_uids,
      mojom::PdfCompositor::IsReadyToCompositeCallback callback) override;

  // Composite the final content and convert it into a PDF file.
  void CompositeToPdf(
      uint32_t frame_id,
      uint32_t page_num,
      mojo::ScopedSharedBufferHandle sk_handle,
      const std::vector<uint32_t>& subframe_uids,
      mojom::PdfCompositor::CompositeToPdfCallback callback) override;

  // Utility function.
  static std::unique_ptr<base::SharedMemory> GetShmFromMojoHandle(
      mojo::ScopedSharedBufferHandle handle);

 private:
  friend class PdfCompositorImplTest;

  // The time limit on waiting for subframe painting and delivering.
  static constexpr int kRequestTimeoutSec = 15;

  struct FrameInfo {
    FrameInfo(std::unique_ptr<base::SharedMemory> content,
              std::vector<uint32_t> uids);
    ~FrameInfo();

    bool composited;
    std::unique_ptr<base::SharedMemory> serialized_content;
    sk_sp<SkPicture> content;
    std::vector<uint32_t> subframe_uids;
  };
  struct RequestInfo {
    RequestInfo();
    ~RequestInfo();

    std::vector<uint32_t> subframe_uids;
    base::ElapsedTimer timer;
  };

  // Composite the content of a subframe.
  sk_sp<SkPicture> CompositeSubframe(uint32_t frame_id);

  const std::unique_ptr<service_manager::ServiceContextRef> service_ref_;
  const std::string creator_;

  // Keep track of all the painted frames' information indexed by frame id.
  std::map<uint32_t, std::unique_ptr<FrameInfo>> frame_info_map_;
  // Content unique ids of frames to be painted.
  std::set<uint32_t> pending_frame_uids_;
  // Maps to track frames' ids and their content unique ids.
  std::map<uint32_t, uint32_t> uid_to_id_map_;
  std::map<uint32_t, std::vector<uint32_t>> id_to_uid_map_;

  // Pending request info for compositing a frame or its page.
  // It maps <frame_id, page_num> to its request.
  std::map<std::pair<uint32_t, uint32_t>, RequestInfo> pending_requests_;

  // Time out value for testing only.
  int test_timeout_;

  DISALLOW_COPY_AND_ASSIGN(PdfCompositorImpl);
};

}  // namespace printing

#endif  // COMPONENTS_PRINTING_SERVICE_PDF_COMPOSITOR_IMPL_H_
