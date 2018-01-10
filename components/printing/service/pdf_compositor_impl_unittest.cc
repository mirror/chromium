// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <memory>

#include "base/callback.h"
#include "base/run_loop.h"
#include "base/test/scoped_task_environment.h"
#include "components/printing/service/pdf_compositor_impl.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace printing {

class PdfCompositorImplTest : public testing::Test {
 public:
  PdfCompositorImplTest()
      : task_environment_(
            base::test::ScopedTaskEnvironment::MainThreadType::IO),
        run_loop_(std::make_unique<base::RunLoop>()),
        is_ready_(false) {}

  void OnIsReadyToCompositeCallback(bool is_ready) {
    is_ready_ = is_ready;
    run_loop_->Quit();
  }

  bool ResultFromCallback() {
    run_loop_->Run();
    run_loop_ = std::make_unique<base::RunLoop>();
    return is_ready_;
  }

 protected:
  base::test::ScopedTaskEnvironment task_environment_;
  std::unique_ptr<base::RunLoop> run_loop_;
  bool is_ready_;
};

TEST_F(PdfCompositorImplTest, IsReadyToComposite) {
  PdfCompositorImpl impl("unittest", nullptr);
  // Frame 1 contains content 3 which corresponds to frame 2.
  // Frame 1, 2 and 3 are all painted.
  impl.AddSubframeMap(1u, 3u, 2u);
  impl.AddSubframeContent(2u, mojo::SharedBufferHandle::Create(10),
                          std::vector<uint32_t>());
  impl.AddSubframeContent(1u, mojo::SharedBufferHandle::Create(10),
                          std::vector<uint32_t>{3u});
  impl.AddSubframeContent(3u, mojo::SharedBufferHandle::Create(10),
                          std::vector<uint32_t>());

  // Frame 1 with content 3 should be ready as frame 2 is ready.
  base::flat_set<uint32_t> pending_content_ids;
  bool is_ready = impl.IsReadyToComposite(1u, std::vector<uint32_t>{3u},
                                          &pending_content_ids);
  ASSERT_TRUE(is_ready);
  ASSERT_TRUE(pending_content_ids.empty());

  // If another page of frame 1 needs content 2, that is not ready
  // since content 2 is not known as being painted.
  is_ready = impl.IsReadyToComposite(1u, std::vector<uint32_t>{2u},
                                     &pending_content_ids);
  ASSERT_FALSE(is_ready);
  ASSERT_EQ(pending_content_ids.size(), 1u);
  ASSERT_EQ(*pending_content_ids.begin(), 2u);

  // Content 2 corresponds to frame 3. Now this page is ready since content 2
  // was painted also.
  impl.AddSubframeMap(1u, 2u, 3u);
  is_ready = impl.IsReadyToComposite(1u, std::vector<uint32_t>{2u},
                                     &pending_content_ids);
  ASSERT_TRUE(is_ready);
  ASSERT_TRUE(pending_content_ids.empty());

  // Frame 4 with content 1, 2 and 3 should not be ready since content 1 is
  // not known as painted yet.
  is_ready = impl.IsReadyToComposite(1u, std::vector<uint32_t>{1u, 2u, 3u},
                                     &pending_content_ids);
  ASSERT_FALSE(is_ready);
  ASSERT_EQ(pending_content_ids.size(), 1u);
  ASSERT_EQ(*pending_content_ids.begin(), 1u);

  // Add content of frame 4. Now it is ready for composition.
  impl.AddSubframeMap(1u, 1u, 3u);
  is_ready = impl.IsReadyToComposite(1u, std::vector<uint32_t>{1u, 2u, 3u},
                                     &pending_content_ids);
  ASSERT_TRUE(is_ready);
  ASSERT_TRUE(pending_content_ids.empty());
}

TEST_F(PdfCompositorImplTest, MultiLayerDependency) {
  PdfCompositorImpl impl("unittest", nullptr);
  // Frame 3 has content 1 which refers to subframe 1.
  // Frame 5 has content 3 which refers to subframe 3.
  impl.AddSubframeMap(3u, 1u, 1u);
  impl.AddSubframeMap(5u, 3u, 3u);
  impl.AddSubframeContent(3u, mojo::SharedBufferHandle::Create(10),
                          std::vector<uint32_t>{1u});

  // Although frame 3's content is added, its subframe 1's content is not.
  // So frame 5 is not ready.
  base::flat_set<uint32_t> pending_content_ids;
  bool is_ready = impl.IsReadyToComposite(5u, std::vector<uint32_t>{3u},
                                          &pending_content_ids);
  ASSERT_FALSE(is_ready);
  ASSERT_EQ(pending_content_ids.size(), 1u);
  ASSERT_EQ(*pending_content_ids.begin(), 3u);

  // When frame 1's content is added, frame 5 is ready.
  impl.AddSubframeContent(1u, mojo::SharedBufferHandle::Create(10),
                          std::vector<uint32_t>());
  is_ready = impl.IsReadyToComposite(5u, std::vector<uint32_t>{3u},
                                     &pending_content_ids);
  ASSERT_TRUE(is_ready);
  ASSERT_TRUE(pending_content_ids.empty());

  // Frame 6 is also ready since it only needs frame 5 which is ready.
  impl.AddSubframeMap(6u, 1u, 5u);
  is_ready = impl.IsReadyToComposite(6u, std::vector<uint32_t>{1u},
                                     &pending_content_ids);
  ASSERT_TRUE(is_ready);
  ASSERT_TRUE(pending_content_ids.empty());
}

TEST_F(PdfCompositorImplTest, DependencyLoop) {
  PdfCompositorImpl impl("unittest", nullptr);
  // Frame 3 has content 1, which refers to frame 1.
  // Frame 5 has content 3, which refers to frame 3.
  impl.AddSubframeMap(3u, 1u, 1u);
  impl.AddSubframeMap(1u, 3u, 3u);
  impl.AddSubframeMap(5u, 3u, 3u);
  impl.AddSubframeContent(1u, mojo::SharedBufferHandle::Create(10),
                          std::vector<uint32_t>{3u});
  impl.AddSubframeContent(3u, mojo::SharedBufferHandle::Create(10),
                          std::vector<uint32_t>{1u});

  // Both frame 1 and 3 are painted, frame 5 should be ready.
  base::flat_set<uint32_t> pending_content_ids;
  bool is_ready = impl.IsReadyToComposite(5u, std::vector<uint32_t>{3u},
                                          &pending_content_ids);
  ASSERT_TRUE(is_ready);
  ASSERT_TRUE(pending_content_ids.empty());

  impl.AddSubframeMap(7u, 6u, 6u);
  impl.AddSubframeMap(6u, 7u, 7u);
  impl.AddSubframeContent(6, mojo::SharedBufferHandle::Create(10),
                          std::vector<uint32_t>{7u});
  // Frame 7 should be ready since frame 6's own content is added and it only
  // depends on frame 7.
  is_ready = impl.IsReadyToComposite(7u, std::vector<uint32_t>{6u},
                                     &pending_content_ids);
  ASSERT_TRUE(is_ready);
  ASSERT_TRUE(pending_content_ids.empty());
}

}  // namespace printing
